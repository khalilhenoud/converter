/**
 * @file materials.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <vector>
#include <string>


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;
struct aiScene;

std::vector<std::string>
populate_materials(
  scene_t *scene, 
  const aiScene *pScene,
  const allocator_t* allocator);