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
#include <library/containers/cvector.h>
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
  cvector_setup(&scene->bvh_repo, get_type_data(bvh_t), 4, allocator);
  cvector_resize(&scene->bvh_repo, 1);
  bvh_t *target = cvector_as(&scene->bvh_repo, 0, bvh_t);

  bvh_t *bvh = create_bvh_from_scene(scene, allocator);
  // the types are binary compatible.
  target->bounds = bvh->bounds;
  target->count = bvh->count;
  target->faces = bvh->faces;
  target->normals = bvh->normals;
  target->nodes = bvh->nodes;
  target->nodes_used = bvh->nodes_used;

  // the pointers have been moved
  bvh->bounds = NULL;
  bvh->faces = NULL;
  bvh->normals = NULL;
  bvh->nodes = NULL;
  bvh->nodes_used = bvh->count = 0;
  allocator->mem_free(bvh);
}