/**
 * @file skinned_meshes.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2025-12-27
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;
struct aiScene;

void
populate_skinned_meshes(
  scene_t *scene,
  const aiScene *pScene,
  const allocator_t *allocator);