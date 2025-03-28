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

#include <vector>
#include <string>


typedef struct allocator_t allocator_t;
typedef struct scene_t scene_t;
typedef struct loader_map_data_t loader_map_data_t;
typedef std::vector<std::string> texture_vec_t;

texture_vec_t
map_to_bin(
  const char* scene_file,
  loader_map_data_t* map_data, 
  scene_t* scene,
  const allocator_t* allocator);