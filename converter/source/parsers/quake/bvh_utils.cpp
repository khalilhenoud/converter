/**
 * @file bvh_utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <library/allocator/allocator.h>
#include <converter/parsers/quake/bvh_utils.h>
#include <entity/c/spatial/bvh.h>
#include <entity/c/spatial/bvh_utils.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/node_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <serializer/serializer_scene_data.h>

#define OPTIMIZE_UNSAFE 0


static
void
build_bvh_node_mesh_transformed_data(
  scene_t* scene,
  uint32_t mesh_index,
  matrix4f transform,
  float** vertices,
  uint32_t** indices,
  uint32_t* indices_count,
  uint32_t* data_index,
  const allocator_t* allocator)
{
  mesh_t* mesh = scene->mesh_repo.meshes + mesh_index;
  // we need to allocate the vertices since they are transformed.
  float* ws_vertices = (float*)allocator->mem_cont_alloc(
    mesh->vertices_count * 3, sizeof(float));
  memcpy(ws_vertices, mesh->vertices, mesh->vertices_count * 3 * sizeof(float));
  for (uint32_t i = 0; i < mesh->vertices_count; ++i) {
#if OPTIMIZE_UNSAFE
    mult_set_m4f_p3f(transform, (point3f*)&ws_vertices[i * 3]);
#else
    point3f pt;
    pt.data[0] = ws_vertices[i * 3 + 0];
    pt.data[1] = ws_vertices[i * 3 + 1];
    pt.data[2] = ws_vertices[i * 3 + 2];
    mult_set_m4f_p3f(&transform, &pt);
    ws_vertices[i * 3 + 0] = pt.data[0];
    ws_vertices[i * 3 + 1] = pt.data[1];
    ws_vertices[i * 3 + 2] = pt.data[2];
#endif
  }

  vertices[*data_index] = ws_vertices;
  indices[*data_index] = mesh->indices;
  indices_count[*data_index] = mesh->indices_count;
  ++*data_index;
}

static
void
build_bvh_node_transformed_data(
  scene_t* scene,
  node_t* node,
  matrix4f transform,
  float** vertices,
  uint32_t** indices,
  uint32_t* indices_count,
  uint32_t* data_index,
  const allocator_t* allocator)
{
  {
    // populate the transformed mesh data.
    for (uint32_t i = 0; i < node->meshes.count; ) {
      uint32_t mesh_index = node->meshes.indices[i];
      if (scene->mesh_repo.meshes[mesh_index].indices_count) {
        build_bvh_node_mesh_transformed_data(
          scene,
          mesh_index,
          transform,
          vertices, indices, indices_count, data_index, allocator);
      }
      ++i;
    }

    // recurively call the child nodes, after concatenating the transform.
    for (uint32_t i = 0; i < node->nodes.count; ++i) {
      uint32_t node_index = node->nodes.indices[i];
      matrix4f concat_transform = transform;
      concat_transform = mult_m4f(
        &transform, &scene->node_repo.nodes[node_index].transform);
      build_bvh_node_transformed_data(
        scene,
        scene->node_repo.nodes + node_index,
        concat_transform,
        vertices, indices, indices_count, data_index, allocator); 
    }
  }
}

static
void
build_bvh_transformed_data(
  scene_t* scene,
  float** vertices,
  uint32_t** indices,
  uint32_t* indices_count,
  const allocator_t* allocator)
{
  assert(scene && vertices && indices && indices_count && allocator);
  assert(scene->node_repo.count && "at least the root node needs to exist!");

  {
    uint32_t data_index = 0;
    build_bvh_node_transformed_data(
      scene, 
      scene->node_repo.nodes, 
      scene->node_repo.nodes[0].transform,
      vertices, indices, indices_count, &data_index, allocator);
  }
}


bvh_t* 
create_bvh_from_scene(
  scene_t* scene, 
  const allocator_t* allocator)
{
  bvh_t* bvh = NULL;
  float** vertices = NULL;
  uint32_t** indices = NULL;
  uint32_t* indices_count = NULL;
  uint32_t mesh_count = 0;

  for (uint32_t i = 0; i < scene->mesh_repo.count; ++i) {
    if (scene->mesh_repo.meshes[i].indices_count)
      ++mesh_count;
  }

  if (mesh_count) {
    vertices = (float**)allocator->mem_alloc(sizeof(float*) * mesh_count);
    assert(vertices && "allocation failed!");
    indices = (uint32_t**)allocator->mem_alloc(sizeof(uint32_t*) * mesh_count);
    assert(indices && "allocation failed!");
    indices_count = (uint32_t*)allocator->mem_alloc(
      sizeof(uint32_t) * mesh_count); 
    assert(indices && "allocation failed!");

    build_bvh_transformed_data(
      scene, vertices, indices, indices_count, allocator);

    bvh = create_bvh(
      vertices, 
      indices, 
      indices_count, 
      mesh_count, 
      allocator, 
      BVH_CONSTRUCT_NAIVE);

    for (uint32_t i = 0; i < mesh_count; ++i)
      allocator->mem_free(vertices[i]);
    allocator->mem_free(vertices);
    allocator->mem_free(indices);
    allocator->mem_free(indices_count);
  }

  return bvh;
}