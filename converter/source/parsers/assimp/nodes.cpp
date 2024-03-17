/**
 * @file nodes.cpp
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
#include <converter/parsers/assimp/nodes.h>
#include <serializer/serializer_scene_data.h>


void
populate_nodes(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator)
{
  // read the nodes.
  std::function<uint32_t(aiNode*)> count_nodes = 
  [&](aiNode* node) -> uint32_t {
    uint32_t total = node->mNumChildren;  
    for (uint32_t i = 0; i < node->mNumChildren; ++i)
      total += count_nodes(node->mChildren[i]);
    return total;
  };

  std::function<void(uint32_t&, serializer_model_data_t*, aiNode*)> 
  populate_node = [&](
    uint32_t& model_index, 
    serializer_model_data_t* target, 
    aiNode* source) {

    ++model_index;

    ::matrix4f_set_identity(&target->transform);
    aiMatrix4x4& transform = source->mTransformation;
    float data[16] = { 
      transform.a1, transform.a2, transform.a3, transform.a4, 
      transform.b1, transform.b2, transform.b3, transform.b4,
      transform.c1, transform.c2, transform.c3, transform.c4,
      transform.d1, transform.d2, transform.d3, transform.d4};
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
      serializer_model_data_t* child_target = 
        scene_bin->model_repo.data + model_index;
      target->models.indices[i] = model_index;
      populate_node(model_index, child_target, child);
    }
  };

  scene_bin->model_repo.used = count_nodes(pScene->mRootNode) + 1;
  scene_bin->model_repo.data = 
    (serializer_model_data_t *)allocator->mem_cont_alloc(
      sizeof(serializer_model_data_t), 
      scene_bin->model_repo.used);
  uint32_t start = 0;
  populate_node(start, scene_bin->model_repo.data, pScene->mRootNode);
}