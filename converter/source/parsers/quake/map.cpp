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
    // setup the scene.
    scene->texture_repo.count = tex_map.size();
    scene->texture_repo.textures = 
      (texture_t*)allocator->mem_cont_alloc(
        scene->texture_repo.count, sizeof(texture_t));

    // one material per texture.
    scene->material_repo.count = scene->texture_repo.count;
    scene->material_repo.materials = 
      (material_t*)allocator->mem_cont_alloc(
        scene->material_repo.count, sizeof(material_t));

    for (auto& entry : tex_map) {
      uint32_t i = entry.second.index;
      scene->texture_repo.textures[i].path = cstring_create(
        entry.second.path.c_str(), allocator);
      
      {
        material_def(scene->material_repo.materials + i);
        scene->material_repo.materials[i].name = cstring_create(
          NULL, allocator);
        scene->material_repo.materials[i].opacity = 1.f;
        scene->material_repo.materials[i].shininess = 1.f;
        scene->material_repo.materials[i].ambient.data[0] =
        scene->material_repo.materials[i].ambient.data[1] =
        scene->material_repo.materials[i].ambient.data[2] = 0.5f;
        scene->material_repo.materials[i].ambient.data[3] = 1.f;
        scene->material_repo.materials[i].diffuse.data[0] =
        scene->material_repo.materials[i].diffuse.data[1] =
        scene->material_repo.materials[i].diffuse.data[2] = 0.6f;
        scene->material_repo.materials[i].diffuse.data[3] = 1.f;
        scene->material_repo.materials[i].specular.data[0] =
        scene->material_repo.materials[i].specular.data[1] =
        scene->material_repo.materials[i].specular.data[2] = 0.6f;
        scene->material_repo.materials[i].specular.data[3] = 1.f;
        
        scene->material_repo.materials[i].textures.used = 1;
        memset(
          scene->material_repo.materials[i].textures.data, 
          0, 
          sizeof(scene->material_repo.materials[i].textures.data));
        scene->material_repo.materials[i].textures.data->index = i;
      }
    }
  }

  {
    // create as many mesh as there are materials.
    scene->mesh_repo.count = scene->material_repo.count;
    scene->mesh_repo.meshes = 
      (mesh_t*)allocator->mem_cont_alloc(
        scene->mesh_repo.count,
        sizeof(mesh_t));
    
    for (auto& entry : tex_map) {
      uint32_t i = entry.second.index;
      mesh_t *mesh = scene->mesh_repo.meshes + i;
      mesh_def(mesh);

      // get the faces that share this index texture-material.
      auto& face_indices = entry.second.indices;
      uint32_t face_count = face_indices.size();
      uint32_t vertices_count = face_count * 3;
      uint32_t sizef3 = sizeof(float) * 3;
      
      mesh->vertices_count = vertices_count;
      mesh->vertices = NULL;
      mesh->normals = NULL;
      mesh->uvs = NULL;
      if (mesh->vertices_count) {
        mesh->vertices = (float*)allocator->mem_alloc(sizef3 * vertices_count);
        mesh->normals = (float*)allocator->mem_alloc(sizef3 * vertices_count);
        mesh->uvs = (float*)allocator->mem_alloc(sizef3 * vertices_count);
        memset(mesh->uvs, 0, sizef3 * vertices_count);
      }

      mesh->indices_count = face_count * 3;
      if (vertices_count) {
        mesh->indices = (uint32_t*)allocator->mem_alloc(
          sizeof(uint32_t) * vertices_count);
      }
      mesh->materials.used = 1;
      mesh->materials.indices[0] = i;

      // copy the data into the mesh.
      uint32_t verti = 0, indexi = 0;
      for (uint32_t k = 0; k < face_count; ++k) {
        auto& face = map_faces[face_indices[k]];
        point3f* points = face.face.points;
        memcpy(mesh->vertices + (verti + 0) * 3, points[0].data, sizef3);
        memcpy(mesh->vertices + (verti + 1) * 3, points[1].data, sizef3);
        memcpy(mesh->vertices + (verti + 2) * 3, points[2].data, sizef3);
        memcpy(mesh->normals + (verti + 0) * 3, face.normal.data, sizef3);
        memcpy(mesh->normals + (verti + 1) * 3, face.normal.data, sizef3);
        memcpy(mesh->normals + (verti + 2) * 3, face.normal.data, sizef3);
        memcpy(mesh->uvs + (verti + 0) * 3, face.uv[0].data, sizef3);
        memcpy(mesh->uvs + (verti + 1) * 3, face.uv[1].data, sizef3);
        memcpy(mesh->uvs + (verti + 2) * 3, face.uv[2].data, sizef3);

        mesh->indices[indexi + 0] = verti + 0;
        mesh->indices[indexi + 1] = verti + 1;
        mesh->indices[indexi + 2] = verti + 2;

        indexi += 3;
        verti += 3;
      }
    }
  }

  {
    // 1 model with multiple meshes (unnamed for now), transform to fit our need
    scene->node_repo.count = 1;
    scene->node_repo.nodes = (node_t*)allocator->mem_alloc(sizeof(node_t));
    node_def(scene->node_repo.nodes);
    scene->node_repo.nodes[0].name = cstring_create(NULL, allocator);
    scene->node_repo.nodes[0].meshes.count = scene->mesh_repo.count;
    scene->node_repo.nodes[0].meshes.indices = (uint32_t *)allocator->mem_alloc(
      sizeof(uint32_t) * scene->mesh_repo.count);
    for (uint32_t i = 0; i < scene->mesh_repo.count; ++i)
      scene->node_repo.nodes[0].meshes.indices[i] = i;
    scene->node_repo.nodes[0].nodes.count = 0;
    matrix4f_rotation_x(&scene->node_repo.nodes[0].transform, -K_PI/2.f);
  }

  {
    // set the camera
    scene->camera_repo.count = 1;
    scene->camera_repo.cameras = 
      (camera_t*)allocator->mem_alloc(sizeof(camera_t));
    scene->camera_repo.cameras[0].position.data[0] = 
    scene->camera_repo.cameras[0].position.data[1] = 
    scene->camera_repo.cameras[0].position.data[2] = 0.f;

    // transform the camera to y being up.
    mult_set_m4f_p3f(
      &scene->node_repo.nodes[0].transform, 
      &scene->camera_repo.cameras[0].position);
    scene->camera_repo.cameras[0].lookat_direction.data[0] = 
    scene->camera_repo.cameras[0].lookat_direction.data[1] = 0.f;
    scene->camera_repo.cameras[0].lookat_direction.data[2] = -1.f;
    scene->camera_repo.cameras[0].up_vector.data[0] = 
    scene->camera_repo.cameras[0].up_vector.data[2] = 0.f;
    scene->camera_repo.cameras[0].up_vector.data[1] = 1.f;
  }

  {
    // set the font
    scene->font_repo.count = 1;
    scene->font_repo.fonts = (font_t *)allocator->mem_alloc(sizeof(font_t));
    scene->font_repo.fonts[0].data_file = cstring_create(
      "\\font\\FontData.csv", allocator);
    scene->font_repo.fonts[0].image_file = cstring_create(
      "\\font\\ExportedFont.png", allocator);
  }

  {
    scene->light_repo.count = map_data->lights.count;
    scene->light_repo.lights = 
      (light_t*)allocator->mem_cont_alloc(
        scene->light_repo.count, sizeof(light_t));

    {
      for (uint32_t i = 0; i < scene->light_repo.count; ++i) {
        loader_map_light_data_t* m_light = map_data->lights.lights + i;
        light_t* s_light = scene->light_repo.lights + i;
        light_def(s_light);
        s_light->name = cstring_create(NULL, allocator);
        s_light->type = LIGHT_TYPE_POINT;
        s_light->position.data[0] = (float)m_light->origin[0];
        s_light->position.data[1] = (float)m_light->origin[1];
        s_light->position.data[2] = (float)m_light->origin[2];
        mult_set_m4f_p3f(
          &scene->node_repo.nodes[0].transform, 
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

    // transform the start_position, is this required?
    mult_set_m4f_p3f(
      &scene->node_repo.nodes[0].transform, 
      &scene->metadata.player_start);
  }

  {
    // for now limit it to 1.
    scene->bvh_repo.count = 1;
    scene->bvh_repo.bvhs = 
      (bvh_t*)allocator->mem_cont_alloc(scene->bvh_repo.count, sizeof(bvh_t));

    bvh_t* bvh = create_bvh_from_scene(scene, allocator);
    // the types are binary compatible.
    scene->bvh_repo.bvhs[0].bounds = bvh->bounds;
    scene->bvh_repo.bvhs[0].count = bvh->count;
    scene->bvh_repo.bvhs[0].faces = bvh->faces;
    scene->bvh_repo.bvhs[0].normals = bvh->normals;
    scene->bvh_repo.bvhs[0].nodes = bvh->nodes;
    scene->bvh_repo.bvhs[0].nodes_used = bvh->nodes_used;

    // the pointers have been moved
    bvh->bounds = NULL;
    bvh->faces = NULL;
    bvh->normals = NULL;
    bvh->nodes = NULL;
    bvh->nodes_used = bvh->count = 0;
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

  std::vector<topology::face_t> map_faces;
  for (uint32_t i = 0; i < map_data->world.brush_count; ++i) {
    const topology::brush_t brush(map_data->world.brushes + i, textures_info);
    std::vector<topology::face_t> faces = brush.to_faces();

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