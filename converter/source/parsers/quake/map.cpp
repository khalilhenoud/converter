/**
 * @file map.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <library/allocator/allocator.h>
#include <library/containers/cvector.h>
#include <library/string/cstring.h>
#include <math/c/vector3f.h>
#include <math/c/face.h>
#include <entity/c/spatial/bvh.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/material.h>
#include <entity/c/misc/font.h>
#include <entity/c/scene/light.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/scene.h>
#include <loaders/loader_map_data.h>
#include <loaders/loader_png.h>
#include <converter/utils.h>
#include <converter/parsers/quake/topology/brush.h>
#include <converter/parsers/quake/topology/poly_brush.h>
#include <converter/parsers/quake/topology/texture_data.h>
#include <converter/parsers/quake/map.h>
#include <converter/parsers/quake/bvh_utils.h>
#include <converter/parsers/quake/string_utils.h>


struct texture_entry_t {
  std::string path;
  loader_png_data_t* png_data = nullptr;
  uint32_t index = 0;
  std::vector<uint32_t> indices;
};

using texture_map_t = std::unordered_map<std::string, texture_entry_t>;

static
void
free_texture_data(
  texture_map_t& tex_map,
  const allocator_t* allocator)
{
  for (auto& data : tex_map)
    free_png(data.second.png_data, allocator);

  tex_map.clear();
}

static
texture_vec_t
load_texture_data(
  const char* scene_file, 
  std::string wad_directory, 
  texture_map_t& tex_map,
  const allocator_t* allocator)
{
  extern std::string tools_folder;
  texture_vec_t textures;

  // this will produce the wad extraction directory.
  std::string directory = scene_file;
  auto iter = directory.find_last_of("\\/");
  directory = directory.substr(0, iter + 1);
  std::replace(wad_directory.begin(), wad_directory.end(), '/', '\\');
  directory += wad_directory;

  {
    // get the canonical wad file path.
    std::string wadfile = directory;
    wadfile += ".wad";
    auto wad_canonical = std::filesystem::canonical(wadfile).string();

    // ensure the extraction directory is created and clean. Set it as the 
    // current path so we can extract into it.
    ensure_clean_directory(directory);
    auto previous_path = std::filesystem::current_path();
    std::filesystem::current_path(directory);

    // extract the wad into the directory.
    std::string buffer = tools_folder;
    buffer += "qpakman -extract ";
    buffer += wad_canonical;
    system(buffer.c_str());

    // restore our working directory.
    std::filesystem::current_path(previous_path);
  }

  // get all the files, load all the png.
  auto files = get_all_files_in_directory(directory);
  uint32_t index = 0;
  for (auto& file : files) {
    std::string name = file.u8string();
    textures.push_back(name);
    // get the simple texture name (no extention, no path).
    name = name.substr(name.find_last_of("\\") + 1);
    name = name.substr(0, name.find_last_of("."));
    // remove '_fbr', qpakman appends '_fbr' to some textures after extraction. 
    // in addition it automatically replaces os unsafe tokens with substrings
    // e.g: '*' becomes 'star_'.
    auto path = name + ".png";
    auto sanitized = name;
    string_utils::replace(sanitized, "_fbr", "");
    loader_png_data_t* image = load_png(file.u8string().c_str(), allocator);
    tex_map[sanitized] = { path, image, index++ };
  }

  return textures;
}

static
void
populate_scene(
  scene_t *scene,
  loader_map_data_t* map_data,
  std::vector<topology::face_t>& map_faces,
  texture_map_t& tex_map,
  const allocator_t* allocator)
{
  {
    cvector_setup(
      &scene->texture_repo, 
      get_type_data(texture_t), 
      tex_map.size(), allocator);
    cvector_resize(&scene->texture_repo, tex_map.size());
    cvector_setup(
      &scene->material_repo, 
      get_type_data(material_t), 
      tex_map.size(), allocator);
    cvector_resize(&scene->material_repo, tex_map.size());

    for (auto& entry : tex_map) {
      uint32_t i = entry.second.index;
      texture_t *texture = cvector_as(&scene->texture_repo, i, texture_t);
      cstring_setup(&texture->path, entry.second.path.c_str(), allocator);
      
      {
        material_t *material = cvector_as(&scene->material_repo, i, material_t);
        material_def(material);
        cstring_setup(&material->name, "", allocator);
        material->opacity = 1.f;
        material->shininess = 1.f;
        material->ambient.data[0] =
        material->ambient.data[1] =
        material->ambient.data[2] = 0.5f;
        material->ambient.data[3] = 1.f;
        material->diffuse.data[0] =
        material->diffuse.data[1] =
        material->diffuse.data[2] = 0.6f;
        material->diffuse.data[3] = 1.f;
        material->specular.data[0] =
        material->specular.data[1] =
        material->specular.data[2] = 0.6f;
        material->specular.data[3] = 1.f;
        
        material->textures.used = 1;
        memset(material->textures.data, 0, sizeof(material->textures.data));
        material->textures.data->index = i;
      }
    }
  }

  {
    // create as many mesh as there are materials.
    cvector_setup(
      &scene->mesh_repo, 
      get_type_data(mesh_t), 
      scene->material_repo.size, 
      allocator);
    cvector_resize(&scene->mesh_repo, scene->material_repo.size);
    
    for (auto& entry : tex_map) {
      uint32_t i = entry.second.index;
      mesh_t *mesh = cvector_as(&scene->mesh_repo, i, mesh_t);
      mesh_def(mesh);

      // get the faces that share this index texture-material.
      auto& face_indices = entry.second.indices;
      uint32_t face_count = face_indices.size();
      uint32_t vertices_count = face_count * 3;
      uint32_t sizef3 = sizeof(float) * 3;
    
      cvector_setup(&mesh->vertices, get_type_data(float), 0, allocator);
      cvector_resize(&mesh->vertices, vertices_count * 3);
      cvector_setup(&mesh->normals, get_type_data(float), 0, allocator);
      cvector_resize(&mesh->normals, vertices_count * 3);
      cvector_setup(&mesh->uvs, get_type_data(float), 0, allocator);
      cvector_resize(&mesh->uvs, vertices_count * 3);
      memset(mesh->uvs.data, 0, sizeof(float) * vertices_count * 3);
      cvector_setup(&mesh->indices, get_type_data(uint32_t), 0, allocator);
      cvector_resize(&mesh->indices, vertices_count);
      mesh->materials.used = 1;
      mesh->materials.indices[0] = i;

      // copy the data into the mesh.
      uint32_t verti = 0, indexi = 0;
      float *vertices = (float *)mesh->vertices.data;
      float *normals = (float *)mesh->normals.data;
      float *uvs = (float *)mesh->uvs.data;
      uint32_t *indices = (uint32_t *)mesh->indices.data;
      for (uint32_t k = 0; k < face_count; ++k) {
        auto& face = map_faces[face_indices[k]];
        point3f* points = face.face.points;
        memcpy(vertices + (verti + 0) * 3, points[0].data, sizef3);
        memcpy(vertices + (verti + 1) * 3, points[1].data, sizef3);
        memcpy(vertices + (verti + 2) * 3, points[2].data, sizef3);
        memcpy(normals + (verti + 0) * 3, face.normal.data, sizef3);
        memcpy(normals + (verti + 1) * 3, face.normal.data, sizef3);
        memcpy(normals + (verti + 2) * 3, face.normal.data, sizef3);
        memcpy(uvs + (verti + 0) * 3, face.uv[0].data, sizef3);
        memcpy(uvs + (verti + 1) * 3, face.uv[1].data, sizef3);
        memcpy(uvs + (verti + 2) * 3, face.uv[2].data, sizef3);

        indices[indexi + 0] = verti + 0;
        indices[indexi + 1] = verti + 1;
        indices[indexi + 2] = verti + 2;

        indexi += 3;
        verti += 3;
      }
    }
  }

  {
    // 1 model with multiple meshes (unnamed for now), transform to fit our need
    cvector_setup(&scene->node_repo, get_type_data(node_t), 4, allocator);
    cvector_resize(&scene->node_repo, 1);
    node_t *node = cvector_as(&scene->node_repo, 0, node_t);
    node_def(node);
    cstring_setup(&node->name, "", allocator);
    cvector_setup(&node->meshes, get_type_data(uint32_t), 0, allocator);
    cvector_resize(&node->meshes, scene->mesh_repo.size);
    for (uint32_t i = 0; i < scene->mesh_repo.size; ++i)
      *cvector_as(&node->meshes, i, uint32_t) = i;
    cvector_setup(&node->nodes, get_type_data(uint32_t), 0, allocator);
    matrix4f_rotation_x(&node->transform, -K_PI/2.f);
  }

  {
    // set the camera
    cvector_setup(&scene->camera_repo, get_type_data(camera_t), 4, allocator);
    cvector_resize(&scene->camera_repo, 1);
    camera_t *camera = cvector_as(&scene->camera_repo, 0, camera_t);
    camera->position.data[0] = 
    camera->position.data[1] = 
    camera->position.data[2] = 0.f;

    // transform the camera to y being up.
    node_t *node = cvector_as(&scene->node_repo, 0, node_t);
    mult_set_m4f_p3f(
      &node->transform, 
      &camera->position);
    camera->lookat_direction.data[0] = 
    camera->lookat_direction.data[1] = 0.f;
    camera->lookat_direction.data[2] = -1.f;
    camera->up_vector.data[0] = 
    camera->up_vector.data[2] = 0.f;
    camera->up_vector.data[1] = 1.f;
  }

  {
    // set the font
    cvector_setup(&scene->font_repo, get_type_data(font_t), 4, allocator);
    cvector_resize(&scene->font_repo, 1);
    font_t *font = cvector_as(&scene->font_repo, 0, font_t);
    cstring_setup(&font->data_file, "\\font\\FontData.csv", allocator);
    cstring_setup(&font->image_file, "\\font\\ExportedFont.png", allocator);
  }

  {
    cvector_setup(
      &scene->light_repo, 
      get_type_data(light_t), 
      map_data->lights.count, 
      allocator);
    cvector_resize(&scene->light_repo, map_data->lights.count);
    node_t *node = cvector_as(&scene->node_repo, 0, node_t);

    {
      for (uint32_t i = 0; i < scene->light_repo.size; ++i) {
        loader_map_light_data_t* m_light = map_data->lights.lights + i;
        light_t* s_light = cvector_as(&scene->light_repo, i, light_t);
        light_def(s_light);
        cstring_setup(&s_light->name, "", allocator);
        s_light->type = LIGHT_TYPE_POINT;
        s_light->position.data[0] = (float)m_light->origin[0];
        s_light->position.data[1] = (float)m_light->origin[1];
        s_light->position.data[2] = (float)m_light->origin[2];
        mult_set_m4f_p3f(
          &node->transform, 
          &s_light->position);
        s_light->attenuation_constant = 1.f;
        s_light->attenuation_linear = 0.01f;
        s_light->attenuation_quadratic = 0.f;
        s_light->ambient.data[0] =
        s_light->ambient.data[1] =
        s_light->ambient.data[2] = 0.2f;
        s_light->ambient.data[3] = 1.f;
        s_light->diffuse.data[0] =
        s_light->diffuse.data[1] =
        s_light->diffuse.data[2] = (float)(m_light->light)/255.f;
        s_light->diffuse.data[3] = 1.f;
        s_light->specular.data[0] =
        s_light->specular.data[1] =
        s_light->specular.data[2] = 0.f;
        s_light->specular.data[3] = 1.f;
      }
    }
  }

  {
    // set the metadata.
    scene->metadata.player_start.data[0] = (float)map_data->player_start[0]; 
    scene->metadata.player_start.data[1] = (float)map_data->player_start[1]; 
    scene->metadata.player_start.data[2] = (float)map_data->player_start[2];
    scene->metadata.player_angle = (float)map_data->player_angle;

    node_t *node = cvector_as(&scene->node_repo, 0, node_t);
    // transform the start_position, is this required?
    mult_set_m4f_p3f(
      &node->transform, 
      &scene->metadata.player_start);
  }

  {
    // for now limit it to 1.
    cvector_setup(&scene->bvh_repo, get_type_data(bvh_t), 4, allocator);
    cvector_resize(&scene->bvh_repo, 1);
    bvh_t *target = cvector_as(&scene->bvh_repo, 0, bvh_t);
    bvh_def(target);
    bvh_t *bvh = create_bvh_from_scene(scene, allocator);
    // the types are binary compatible.
    cvector_fullswap(&bvh->normals, &target->normals);
    cvector_fullswap(&bvh->faces, &target->faces);
    cvector_fullswap(&bvh->bounds, &target->bounds);
    cvector_fullswap(&bvh->nodes, &target->nodes);
    allocator->mem_free(bvh);
  }
}

static
void
map_to_meshes(
  scene_t* scene,
  const char* scene_file,
  loader_map_data_t* map_data,
  texture_map_t& tex_map,
  const allocator_t* allocator)
{
  // we only need the width and height
  std::unordered_map<std::string, topology::texture_info_t> textures_info;
  for (auto& entry : tex_map)
    textures_info[entry.first] = { 
      entry.second.png_data->width, entry.second.png_data->height };

  std::vector<topology::poly_brush_t> poly_brushes;
  for (uint32_t i = 0; i < map_data->world.brush_count; ++i) {
    const topology::brush_t brush(map_data->world.brushes + i, textures_info);
    poly_brushes.emplace_back(&brush);
  }

  topology::poly_brush_t::sort_and_weld(poly_brushes);

  std::vector<topology::face_t> map_faces;
  for (uint32_t i = 0; i < poly_brushes.size(); ++i) {
    const topology::poly_brush_t& poly_brush = poly_brushes[i];
    std::vector<topology::face_t> faces = poly_brush.to_faces();

    for (uint32_t j = 0; j < faces.size(); ++j) {
      auto& face = faces[j];
      if (face.texture.size())
        tex_map[face.texture].indices.push_back(map_faces.size() + j);
    }
    
    map_faces.insert(map_faces.end(), faces.begin(), faces.end());
  }

  populate_scene(
    scene, 
    map_data, 
    map_faces, 
    tex_map,
    allocator);
}

texture_vec_t
map_to_bin(
  const char* scene_file,
  loader_map_data_t* map_data, 
  scene_t *scene,
  const allocator_t* allocator)
{
  texture_map_t tex_map;
  texture_vec_t textures = load_texture_data(
    scene_file, 
    map_data->world.wad, 
    tex_map,
    allocator);

  map_to_meshes(
    scene,
    scene_file, 
    map_data,
    tex_map,
    allocator);
  
  free_texture_data(tex_map, allocator);

  return textures;
}