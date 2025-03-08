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
#include <library/string/cstring.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/meshes.h>


void
populate_meshes(
  scene_t *scene, 
  const aiScene *pScene, 
  const allocator_t *allocator)
{
  scene->mesh_repo.count = pScene->mNumMeshes;
  scene->mesh_repo.meshes = (mesh_t *)allocator->mem_cont_alloc(
    sizeof(mesh_t), scene->mesh_repo.count);
  
  // NOTE: Currently assimp will decompose the mesh if it contains more than
  // one material, so basically a single material is specified. The rest of
  // the materials can be found on identically named meshes. 
  // Additionally no transform is assigned to the mesh, instead it uses the
  // transform attached to the parent node.
  for (uint32_t i = 0; i < scene->mesh_repo.count; ++i) {
    mesh_t *mesh = scene->mesh_repo.meshes + i;
    aiMesh* pMesh = pScene->mMeshes[i];

    mesh->materials.used = pScene->mNumMaterials == 0 ? 0 : 1;
    mesh->materials.indices[0] = pMesh->mMaterialIndex;

    mesh->indices_count = pMesh->mNumFaces * 3;
    mesh->indices = (uint32_t *)allocator->mem_cont_alloc(
      sizeof(uint32_t), mesh->indices_count);
    for (uint32_t j = 0, count = (mesh->indices_count / 3); j < count; ++j) {
      aiFace *face = &pMesh->mFaces[j];
      assert(face->mNumIndices == 3 && "We do not support non-triangle!!");
      uint32_t *target_face = mesh->indices + j * 3;
      target_face[0] = face->mIndices[0];
      target_face[1] = face->mIndices[1];
      target_face[2] = face->mIndices[2];
    }

    mesh->vertices_count = pMesh->mNumVertices;
    mesh->vertices = (float *)allocator->mem_cont_alloc(
      sizeof(float), mesh->vertices_count * 3);
    memset(mesh->vertices, 0, sizeof(float) * mesh->vertices_count * 3);
    if (pMesh->mVertices) {
      for (uint32_t j = 0; j < mesh->vertices_count; ++j) {
        aiVector3D* vertex = &pMesh->mVertices[j];
        float *target_vertex = mesh->vertices + j * 3;
        target_vertex[0] = vertex->x;
        target_vertex[1] = vertex->y;
        target_vertex[2] = vertex->z;
      }
    }
    
    mesh->normals = (float *)allocator->mem_cont_alloc(
      sizeof(float), mesh->vertices_count * 3);
    memset(
      mesh->normals, 0, sizeof(float) * mesh->vertices_count * 3);
    if (pMesh->mNormals) {
      for (uint32_t j = 0; j < mesh->vertices_count; ++j) {
        aiVector3D* normal = &pMesh->mNormals[j];
        float *target_normal = mesh->normals + j * 3;
        target_normal[0] = normal->x;
        target_normal[1] = normal->y;
        target_normal[2] = normal->z;
      }
    }
    
    // NOTE: Assimp supports 8 channels for vertices, we only consider the first
    mesh->uvs = (float *)allocator->mem_cont_alloc(
      sizeof(float), mesh->vertices_count * 3);
    memset(mesh->uvs, 0, sizeof(float) * mesh->vertices_count * 3);
    if (pMesh->mTextureCoords[0]) {
      for (uint32_t j = 0; j < mesh->vertices_count; ++j) {
        aiVector3D* uvs = &pMesh->mTextureCoords[0][j];
        float *target_uvs = mesh->uvs + j * 3;
        target_uvs[0] = uvs->x;
        target_uvs[1] = uvs->y;
        target_uvs[2] = uvs->z;
      }
    }
  }
}