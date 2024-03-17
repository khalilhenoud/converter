/**
 * @file cameras.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once


typedef struct serializer_scene_data_t serializer_scene_data_t;
typedef struct allocator_t allocator_t;
struct aiScene;

void
populate_cameras(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator);