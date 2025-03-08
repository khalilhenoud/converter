/**
 * @file lights.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;
struct aiScene;

void
populate_lights(
  scene_t *scene, 
  const aiScene *pScene, 
  const allocator_t *allocator);