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
#include <library/string/cstring.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/nodes.h>


void
populate_nodes(
  scene_t *scene, 
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

  std::function<void(uint32_t&, node_t*, aiNode*)> 
  populate_node = [&](
    uint32_t& model_index, 
    node_t *target, 
    aiNode *source) {

    ++model_index;

    ::matrix4f_set_identity(&target->transform);
    aiMatrix4x4& transform = source->mTransformation;
    float data[16] = { 
      transform.a1, transform.a2, transform.a3, transform.a4, 
      transform.b1, transform.b2, transform.b3, transform.b4,
      transform.c1, transform.c2, transform.c3, transform.c4,
      transform.d1, transform.d2, transform.d3, transform.d4};
    memcpy(target->transform.data, data, sizeof(float) * 16);

    target->name = cstring_create(source->mName.C_Str(), allocator);

    target->meshes.count = source->mNumMeshes;
    target->meshes.indices = NULL;
    if (target->meshes.count) {
      target->meshes.indices = (uint32_t*)allocator->mem_cont_alloc(
        target->meshes.count, sizeof(uint32_t));
      memcpy(
        target->meshes.indices,
        source->mMeshes,
        sizeof(uint32_t) * source->mNumMeshes);
    }
    
    target->nodes.count = source->mNumChildren;
    target->nodes.indices = NULL;
    if (target->nodes.count) {
      target->nodes.indices = (uint32_t*)allocator->mem_cont_alloc(
        target->nodes.count, sizeof(uint32_t));
      for (uint32_t i = 0; i < source->mNumChildren; ++i) {
        aiNode* child = source->mChildren[i];
        node_t* child_target = scene->node_repo.nodes + model_index;
        target->nodes.indices[i] = model_index;
        populate_node(model_index, child_target, child);
      }
    }
  };

  scene->node_repo.count = count_nodes(pScene->mRootNode) + 1;
  scene->node_repo.nodes = (node_t *)allocator->mem_cont_alloc(
    sizeof(node_t), scene->node_repo.count);
  uint32_t start = 0;
  populate_node(start, scene->node_repo.nodes, pScene->mRootNode);
}