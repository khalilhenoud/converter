/**
 * @file meshes.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <cassert>
#include <functional>
#include <assimp/types.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <library/allocator/allocator.h>
#include <library/string/fixed_string.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/meshes.h>
#include <serializer/serializer_scene_data.h>


void
populate_meshes(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator)
{
  // TODO: This could be greatly enhanced in terms of what we can support.
  // reading the meshes.
  scene_bin->mesh_repo.used = pScene->mNumMeshes;
  scene_bin->mesh_repo.data = 
    (serializer_mesh_data_t *)allocator->mem_cont_alloc(
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
    current->indices = (uint32_t *)allocator->mem_cont_alloc(
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
    current->vertices = (float *)allocator->mem_cont_alloc(
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
    
    current->normals = (float *)allocator->mem_cont_alloc(
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
    current->uvs = (float *)allocator->mem_cont_alloc(
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