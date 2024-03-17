/**
 * @file meshes.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once


typedef struct serializer_scene_data_t serializer_scene_data_t;
typedef struct allocator_t allocator_t;
struct aiScene;

void
populate_meshes(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator);