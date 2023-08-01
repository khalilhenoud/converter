/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <vector>
#include <cassert>
#include <malloc.h>
#include <algorithm>
#include <functional>
#include <library/allocator/allocator.h>
#include <serializer/serializer_bin.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


std::vector<uintptr_t> allocated;

void* allocate(size_t size)
{
  void* block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void free_block(void* block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(), 
      allocated.end(), 
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }), 
    allocated.end());
  free(block);
}

int main(int argc, char *argv[])
{
  assert(argc >= 3 && "provide path to mesh file!");
  const char* mesh_file = argv[1];
  const char* target_file = argv[2];

  allocator_t allocator;
  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = NULL;
  allocator.mem_realloc = NULL;

  Assimp::Importer Importer;
  const aiScene* pScene = Importer.ReadFile(
    mesh_file, 
    aiProcess_Triangulate | 
    aiProcess_GenSmoothNormals | 
    aiProcess_FlipUVs | 
    aiProcess_JoinIdenticalVertices);

  if (!pScene)
    printf("Error parsing '%s': '%s'\n", mesh_file, Importer.GetErrorString());
  else {
    printf("Parsing was successful");

    serializer_scene_data_t* scene_bin = 
      (serializer_scene_data_t*)allocator.mem_alloc(
        sizeof(serializer_scene_data_t));

    std::vector<std::string> textures;

    auto copy_str = [&](
      fixed_str_t& target, const char* source, aiReturn do_copy) {
      if (do_copy == AI_SUCCESS) {
        memset(target.data, 0, sizeof(target.data));
        memcpy(target.data, source, strlen(source));
      }
    };

    auto copy_float = [&](float* target, float* source, aiReturn do_copy) {
      if (do_copy == AI_SUCCESS)
        *target = *source;
    };

    {
      // reading the materials.
      // NOTE: if pScene AI_SCENE_FLAGS_INCOMPLETE is set pScene might have no
      // materials.
      scene_bin->material_repo.used = pScene->mNumMaterials;
      scene_bin->material_repo.data = 
        (serializer_material_data_t*)allocator.mem_cont_alloc(
          sizeof(serializer_material_data_t),
          scene_bin->material_repo.used);

      auto copy_color = [&](
        serializer_color_data_t* target, 
        aiColor4D* source, 
        aiReturn do_copy) {

        if (do_copy == AI_SUCCESS) {
          target->data[0] = source->r;
          target->data[1] = source->g;
          target->data[2] = source->b;
          target->data[3] = source->a;
        }
      };

      auto copy_texture_transform = [&](
        serializer_texture_properties_t* target, 
        aiUVTransform* source,
        aiReturn do_copy) {
        if (do_copy == AI_SUCCESS) {
          target->angle = source->mRotation;
          target->u = source->mTranslation.x;
          target->v = source->mTranslation.y;
          target->u_scale = source->mScaling.x;
          target->v_scale = source->mScaling.y;
        }
      };

      for (uint32_t i = 0; i < scene_bin->material_repo.used; ++i) {
        auto* bin_material = scene_bin->material_repo.data + i;
        aiMaterial* pMaterial = pScene->mMaterials[i];
        
        aiColor4D color;
        aiReturn value;
        value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &color);
        copy_color(&bin_material->diffuse, &color, value);
        value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &color);
        copy_color(&bin_material->ambient, &color, value);
        value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &color);
        copy_color(&bin_material->specular, &color, value);
        
        float data_float = 1.f;
        value = aiGetMaterialFloat(pMaterial, AI_MATKEY_OPACITY, &data_float);
        copy_float(&bin_material->opacity, &data_float, value);
        value = aiGetMaterialFloat(pMaterial, AI_MATKEY_SHININESS, &data_float);
        copy_float(&bin_material->shininess, &data_float, value);

        aiString data_str;
        value = aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &data_str);
        copy_str(bin_material->name, data_str.C_Str(), value);

        // Read the number of textures used by this material, stop at 8 (this is
        // max supported).
        bin_material->textures.used = 0;
        uint32_t t_type_index = 1;
        uint32_t t_type_total = aiTextureType::AI_TEXTURE_TYPE_MAX;
        for (; t_type_index < t_type_total; ++t_type_index) {
          uint32_t t_total = aiGetMaterialTextureCount(
            pMaterial, 
            (aiTextureType)t_type_index);

          for (uint32_t t_index = 0; t_index < t_total; ++t_index) {
            aiString path;
            value = aiGetMaterialTexture(
              pMaterial, 
              (aiTextureType)t_type_index, 
              t_index, 
              &path);
            
            if (value == AI_SUCCESS) {
              auto iter = std::find(
                textures.begin(), 
                textures.end(), 
                path.C_Str());

              uint32_t global_texture_index = iter - textures.begin();

              if (iter == textures.end())
                textures.push_back(path.C_Str());
              
              serializer_texture_properties_t* texture_props = 
                bin_material->textures.data + 
                bin_material->textures.used++;
              texture_props->index = global_texture_index;

              copy_str(
                texture_props->type, 
                aiTextureTypeToString((aiTextureType)t_type_index), 
                AI_SUCCESS);

              aiUVTransform transform;
              value = aiGetMaterialUVTransform(
                pMaterial, 
                AI_MATKEY_UVTRANSFORM((aiTextureType)t_type_index, t_index), 
                &transform);
              copy_texture_transform(texture_props, &transform, value);

              if (bin_material->textures.used == 8)
                break;
            }
          }

          if (bin_material->textures.used == 8)
            break;
        }
      }
    }

    {
      // adding the textures
      scene_bin->texture_repo.used = (uint32_t)textures.size();
      if (scene_bin->texture_repo.used) {
        scene_bin->texture_repo.data =
          (serializer_texture_data_t*)allocator.mem_cont_alloc(
            sizeof(serializer_texture_data_t),
            scene_bin->texture_repo.used);

        for (uint32_t i = 0; i < scene_bin->texture_repo.used; ++i) {
          memset(
            scene_bin->texture_repo.data[i].path.data, 0,
            sizeof(scene_bin->texture_repo.data[i].path.data));
          memcpy(
            scene_bin->texture_repo.data[i].path.data,
            textures[i].c_str(),
            strlen(textures[i].c_str()));
        }
      }
    }

    // TODO: This could be greatly enhanced in terms of what we can support.
    {
      // reading the meshes.
      scene_bin->mesh_repo.used = pScene->mNumMeshes;
      scene_bin->mesh_repo.data = 
        (serializer_mesh_data_t *)allocator.mem_cont_alloc(
          sizeof(serializer_mesh_data_t),
          scene_bin->mesh_repo.used);
      
      // NOTE: Currently assimp will decompose the mesh if it contains more than
      // one material, so basically a single material is specified. The rest of
      // the materials can be found on identically named meshes. 
      // Additionally no transform is assigned to the mesh, instead it uses the
      // transform attached to the parent node.
      for (uint32_t i = 0; i < scene_bin->mesh_repo.used; ++i) {
        serializer_mesh_data_t *current = scene_bin->mesh_repo.data + i;
        aiMesh* pMesh = pScene->mMeshes[i];

        copy_str(current->name, pMesh->mName.C_Str(), AI_SUCCESS);
        matrix4f_set_identity(&current->local_transform);
        current->materials.used = pScene->mNumMaterials == 0 ? 0 : 1;
        current->materials.indices[0] = pMesh->mMaterialIndex;

        current->faces_count = pMesh->mNumFaces;
        current->indices = (uint32_t *)allocator.mem_cont_alloc(
          sizeof(uint32_t), current->faces_count * 3);
        for (uint32_t j = 0; j < current->faces_count; ++j) {
          aiFace* face = &pMesh->mFaces[j];
          assert(
            face->mNumIndices == 3 && 
            "We do not support non-triangle!!");
          uint32_t *target_face = current->indices + j * 3;
          target_face[0] = face->mIndices[0];
          target_face[1] = face->mIndices[1];
          target_face[2] = face->mIndices[2];
        }

        current->vertices_count = pMesh->mNumVertices;
        current->vertices = (float *)allocator.mem_cont_alloc(
          sizeof(float), current->vertices_count * 3);
        memset(
          current->vertices, 0, sizeof(float) * current->vertices_count * 3);
        if (pMesh->mVertices) {
          for (uint32_t j = 0; j < current->vertices_count; ++j) {
            aiVector3D* vertex = &pMesh->mVertices[j];
            float *target_vertex = current->vertices + j * 3;
            target_vertex[0] = vertex->x;
            target_vertex[1] = vertex->y;
            target_vertex[2] = vertex->z;
          }
        }
        
        current->normals = (float *)allocator.mem_cont_alloc(
          sizeof(float), current->vertices_count * 3);
        memset(
          current->normals, 0, sizeof(float) * current->vertices_count * 3);
        if (pMesh->mNormals) {
          for (uint32_t j = 0; j < current->vertices_count; ++j) {
            aiVector3D* normal = &pMesh->mNormals[j];
            float *target_normal = current->normals + j * 3;
            target_normal[0] = normal->x;
            target_normal[1] = normal->y;
            target_normal[2] = normal->z;
          }
        }
        
        // NOTE: Assimp supports 8 channels for vertices, we ignore all but the
        // first.
        current->uvs = (float *)allocator.mem_cont_alloc(
          sizeof(float), current->vertices_count * 3);
        memset(
          current->uvs, 0, sizeof(float) * current->vertices_count * 3);
        if (pMesh->mTextureCoords[0]) {
          for (uint32_t j = 0; j < current->vertices_count; ++j) {
            aiVector3D* uvs = &pMesh->mTextureCoords[0][j];
            float *target_uvs = current->uvs + j * 3;
            target_uvs[0] = uvs->x;
            target_uvs[1] = uvs->y;
            target_uvs[2] = uvs->z;
          }
        }
      }
    }

    {
      // read the nodes.
      std::function<uint32_t(aiNode*)> count_nodes = 
      [&](aiNode* node) -> uint32_t {
        uint32_t total = node->mNumChildren;  
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
          total += count_nodes(node->mChildren[i]);
        return total;
      };

      std::function<void(uint32_t&, serializer_model_data_t*, aiNode*)> populate_node = 
      [&](
        uint32_t& model_index, 
        serializer_model_data_t* target, 
        aiNode* source) {

        ++model_index;

        ::matrix4f_set_identity(&target->transform);
        aiMatrix4x4& transform = source->mTransformation;
        // NOTE: This needs double checking.
        float data[16] = { 
          transform.a1, transform.a2, transform.a3, transform.a4, 
          transform.b1, transform.b2, transform.b3, transform.b4,
          transform.c1, transform.c2, transform.c3, transform.c4};
        memcpy(target->transform.data, data, sizeof(float) * 16);
        copy_str(target->name, source->mName.C_Str(), AI_SUCCESS);

        target->meshes.used = source->mNumMeshes;
        memcpy(
          target->meshes.indices, 
          source->mMeshes, 
          sizeof(uint32_t) * source->mNumMeshes); 
        
        target->models.used = source->mNumChildren;
        for (uint32_t i = 0; i < source->mNumChildren; ++i) {
          aiNode* child = source->mChildren[i];
          serializer_model_data_t* child_target = scene_bin->model_repo.data + model_index;
          target->models.indices[i] = model_index;
          populate_node(model_index, child_target, child);
        }
      };

      scene_bin->model_repo.used = count_nodes(pScene->mRootNode) + 1;
      scene_bin->model_repo.data = 
        (serializer_model_data_t *)allocator.mem_cont_alloc(
          sizeof(serializer_model_data_t), 
          scene_bin->model_repo.used);
      uint32_t start = 0;
      populate_node(start, scene_bin->model_repo.data, pScene->mRootNode);
    }

    ::serialize_bin(target_file, scene_bin);
    ::free_bin(scene_bin, &allocator);
  }
  
  assert(allocated.size() == 0);
  return 0;
}