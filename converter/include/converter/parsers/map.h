/**
 * @file map.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once


typedef struct allocator_t allocator_t;
typedef struct serializer_scene_data_t serializer_scene_data_t;
typedef struct loader_map_data_t loader_map_data_t;

serializer_scene_data_t*
map_to_bin(
  const char* scene_file, 
  loader_map_data_t* map, 
  const allocator_t* allocator);