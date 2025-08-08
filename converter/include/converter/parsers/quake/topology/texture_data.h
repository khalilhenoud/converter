/**
 * @file texture_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once


namespace topology {

struct texture_data_t {
  int32_t offset[2] = { 0, 0 };
  int32_t rotation = 0;
  float scale[2] = { 1.f, 1.f };
};

struct texture_info_t {
  uint32_t width = 0;
  uint32_t height = 0;
};

}