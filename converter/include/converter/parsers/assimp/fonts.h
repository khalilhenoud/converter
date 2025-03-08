/**
 * @file fonts.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;

void
populate_default_font(
  scene_t *scene,
  const allocator_t *allocator);