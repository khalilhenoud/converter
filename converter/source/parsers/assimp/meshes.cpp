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
#include <library/containers/cvector.h>
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
  cvector_setup(&scene->mesh_repo, get_type_data(mesh_t), 4, allocator);
  cvector_resize(&scene->mesh_repo, pScene->mNumMeshes);
  
  // NOTE: Currently assimp will decompose the mesh if it contains more than
  // one material, so basically a single material is specified. The rest of
  // the materials can be found on identically named meshes. 
  // Additionally no transform is assigned to the mesh, instead it uses the
  // transform attached to the parent node.
  for (uint32_t i = 0; i < scene->mesh_repo.size; ++i) {
    mesh_t *mesh = cvector_as(&scene->mesh_repo, i, mesh_t);
    aiMesh *pMesh = pScene->mMeshes[i];

    mesh->materials.used = pScene->mNumMaterials == 0 ? 0 : 1;
    mesh->materials.indices[0] = pMesh->mMaterialIndex;

    uint32_t indices_count = pMesh->mNumFaces * 3;
    cvector_setup(&mesh->indices, get_type_data(uint32_t), 0, allocator);
    cvector_resize(&mesh->indices, indices_count);
    for (uint32_t j = 0, count = (indices_count / 3); j < count; ++j) {
      aiFace *face = &pMesh->mFaces[j];
      assert(face->mNumIndices == 3 && "We do not support non-triangle!!");
      uint32_t *target_face = ((uint32_t *)mesh->indices.data) + j * 3;
      target_face[0] = face->mIndices[0];
      target_face[1] = face->mIndices[1];
      target_face[2] = face->mIndices[2];
    }

    uint32_t vertices_count = pMesh->mNumVertices;
    cvector_setup(&mesh->vertices, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->vertices, vertices_count * 3);
    memset(mesh->vertices.data, 0, sizeof(float) * vertices_count * 3);
    if (pMesh->mVertices) {
      for (uint32_t j = 0; j < vertices_count; ++j) {
        aiVector3D* vertex = &pMesh->mVertices[j];
        float *target_vertex = ((float *)mesh->vertices.data) + j * 3;
        target_vertex[0] = vertex->x;
        target_vertex[1] = vertex->y;
        target_vertex[2] = vertex->z;
      }
    }
    
    cvector_setup(&mesh->normals, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->normals, vertices_count * 3);
    memset(mesh->normals.data, 0, sizeof(float) * vertices_count * 3);
    if (pMesh->mNormals) {
      for (uint32_t j = 0; j < vertices_count; ++j) {
        aiVector3D* normal = &pMesh->mNormals[j];
        float *target_normal = ((float *)mesh->normals.data) + j * 3;
        target_normal[0] = normal->x;
        target_normal[1] = normal->y;
        target_normal[2] = normal->z;
      }
    }
    
    // NOTE: Assimp supports 8 channels for vertices, we only consider the first
    cvector_setup(&mesh->uvs, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->uvs, vertices_count * 3);
    memset(mesh->uvs.data, 0, sizeof(float) * vertices_count * 3);
    if (pMesh->mTextureCoords[0]) {
      for (uint32_t j = 0; j < vertices_count; ++j) {
        aiVector3D* uvs = &pMesh->mTextureCoords[0][j];
        float *target_uvs = ((float *)mesh->uvs.data) + j * 3;
        target_uvs[0] = uvs->x;
        target_uvs[1] = uvs->y;
        target_uvs[2] = uvs->z;
      }
    }
  }
}