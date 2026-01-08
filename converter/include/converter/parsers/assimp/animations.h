/**
 * @file animations.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-01-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;
struct aiScene;

void
populate_animations(
  scene_t *scene,
  const aiScene *pScene,
  const allocator_t *allocator);