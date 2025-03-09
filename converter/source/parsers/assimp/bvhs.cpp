/**
 * @file bvhs.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <cassert>
#include <functional>
#include <assimp/scene.h>
#include <assimp/camera.h>
#include <assimp/types.h>
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <entity/c/spatial/bvh.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/quake/bvh_utils.h>


void
populate_bvhs(
  scene_t *scene, 
  const allocator_t *allocator)
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