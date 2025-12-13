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
#include <entity/spatial/bvh.h>
#include <entity/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/quake/bvh_utils.h>


void
populate_bvhs(
  scene_t *scene,
  const allocator_t *allocator)
{
  // for now limit it to 1.
  cvector_setup(&scene->bvh_repo, get_type_data(bvh_t), 0, allocator);
  cvector_resize(&scene->bvh_repo, 1);
  bvh_t *target = cvector_as(&scene->bvh_repo, 0, bvh_t);
  bvh_def(target);

  bvh_t *bvh = create_bvh_from_scene(scene, allocator);
  // the types are binary compatible.
  cvector_fullswap(&bvh->normals, &target->normals);
  cvector_fullswap(&bvh->faces, &target->faces);
  cvector_fullswap(&bvh->bounds, &target->bounds);
  cvector_fullswap(&bvh->nodes, &target->nodes);
  allocator->mem_free(bvh);
}