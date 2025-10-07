/**
 * @file point.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <math/c/vector3f.h>


namespace topology {

struct texture_data_t;
struct texture_info_t;

point3f
get_texture_coordinates(
  const point3f& point,
  const texture_data_t& texture_data,
  const texture_info_t& texture_info,
  const vector3f& normal);

inline
float
distance_points(const point3f& p1, const point3f& p2)
{
  vector3f diff = diff_v3f(&p1, &p2);
  return length_v3f(&diff);
}

inline
float
distance_points_squared(const point3f& p1, const point3f& p2)
{
  vector3f diff = diff_v3f(&p1, &p2);
  return length_squared_v3f(&diff);
}

inline
bool
identical_points(const point3f& p1, const point3f& p2, float epsilon = 1.f/32.f)
{
  return distance_points(p1, p2) < epsilon;
}

}