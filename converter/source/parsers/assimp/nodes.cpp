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
#include <library/containers/cvector.h>
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

    cstring_setup(&target->name, source->mName.C_Str(), allocator);
    cvector_setup(&target->meshes, get_type_data(uint32_t), 0, allocator);
    cvector_resize(&target->meshes, source->mNumMeshes);

    if (source->mNumMeshes)
      memcpy(target->meshes.data, source->mMeshes,
        sizeof(uint32_t) * source->mNumMeshes);
    
    cvector_setup(&target->nodes, get_type_data(uint32_t), 0, allocator);
    cvector_resize(&target->nodes, source->mNumChildren);
    if (source->mNumChildren) {
      for (uint32_t i = 0; i < source->mNumChildren; ++i) {
        aiNode *child = source->mChildren[i];
        node_t *child_target = cvector_as(
          &scene->node_repo, model_index, node_t);
        *cvector_as(&target->nodes, i, uint32_t) = model_index;
        populate_node(model_index, child_target, child);
      }
    }
  };

  cvector_setup(&scene->node_repo, get_type_data(node_t), 4, allocator);
  cvector_resize(&scene->node_repo, count_nodes(pScene->mRootNode) + 1);
  uint32_t start = 0;
  node_t *first = cvector_as(&scene->node_repo, 0, node_t);
  populate_node(start, first, pScene->mRootNode);
}