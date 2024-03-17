/**
 * @file loader.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once


typedef struct allocator_t allocator_t;

void
load_assimp(
  const char* scene_file, 
  const allocator_t* allocator);