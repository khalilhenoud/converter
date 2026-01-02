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
#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>
#include <converter/parsers/assimp/nodes.h>
#include <converter/utils.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <library/allocator/allocator.h>
#include <library/containers/cvector.h>
#include <library/string/cstring.h>
#include <entity/mesh/mesh.h>
#include <entity/mesh/skinned_mesh.h>
#include <entity/scene/node.h>
#include <entity/scene/scene.h>


void
populate_nodes(
  scene_t *scene,
  const aiScene* pScene,
  const allocator_t* allocator)
{
  std::vector<uint32_t> meshes[2];      // static {0}, skinned {1}
  for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
    meshes[pScene->mMeshes[i]->HasBones() ? 1 : 0].push_back(i);

  std::function<node_resource_t(uint32_t)> get_resource =
  [&](uint32_t mesh_index) -> node_resource_t {
    {
      auto iter = std::find(meshes[0].begin(), meshes[0].end(), mesh_index);
      if (iter != meshes[0].end()) {
        uint32_t index = (uint32_t)std::distance(meshes[0].begin(), iter);
        return node_resource_t { get_type_id(mesh_t), index };
      }
    }
    {
      auto iter = std::find(meshes[1].begin(), meshes[1].end(), mesh_index);
      if (iter != meshes[1].end()) {
        uint32_t index = (uint32_t)std::distance(meshes[1].begin(), iter);
        return node_resource_t { get_type_id(skinned_mesh_t), index };
      }
    }

    assert(false);
    return node_resource_t { 0, 0 };
  };

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
    cvector_setup(
      &target->resources, get_type_data(node_resource_t), 0, allocator);
    cvector_resize(&target->resources, source->mNumMeshes);
    for (uint32_t i = 0; i < source->mNumMeshes; ++i) {
      node_resource_t *resource = cvector_as(
        &target->resources, i, node_resource_t);
      *resource = get_resource(source->mMeshes[i]);
    }

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