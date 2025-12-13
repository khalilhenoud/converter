/**
 * @file aabb.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2025-10-04
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <cmath>
#include <cassert>
#include <vector>
#include <math/vector3f.h>


namespace topology {

class aabb_t {
public:
  aabb_t(
    const point3f& _min,
    const point3f& _max)
    : min{_min}
    , max{_max}
  {}

  aabb_t(const aabb_t& rhs)
    : min{rhs.min}
    , max{rhs.max}
  {}

  aabb_t(const std::vector<point3f>& points)
  {
    assert(points.size());

    min = points[0];
    max = points[0];

    for (auto& point : points) {
      min.data[0] = fmin(min.data[0], point.data[0]);
      min.data[1] = fmin(min.data[1], point.data[1]);
      min.data[2] = fmin(min.data[2], point.data[2]);

      max.data[0] = fmax(max.data[0], point.data[0]);
      max.data[1] = fmax(max.data[1], point.data[1]);
      max.data[2] = fmax(max.data[2], point.data[2]);
    }
  }

  aabb_t&
  operator=(const aabb_t& rhs)
  {
    min = rhs.min;
    max = rhs.max;
    return *this;
  }

  aabb_t
  operator+(const aabb_t& rhs) const
  {
    aabb_t copy = *this;
    copy.min.data[0] = fmin(min.data[0], rhs.min.data[0]);
    copy.min.data[1] = fmin(min.data[1], rhs.min.data[1]);
    copy.min.data[2] = fmin(min.data[2], rhs.min.data[2]);

    copy.max.data[0] = fmax(max.data[0], rhs.max.data[0]);
    copy.max.data[1] = fmax(max.data[1], rhs.max.data[1]);
    copy.max.data[2] = fmax(max.data[2], rhs.max.data[2]);

    return copy;
  }

  aabb_t&
  operator+=(const aabb_t& rhs)
  {
    *this = *this + rhs;
    return *this;
  }

  point3f
  centroid() const
  {
    vector3f diff = diff_v3f(&min, &max);
    mult_set_v3f(&diff, 0.5f);
    return add_v3f(&min, &diff);
  }

  bool
  interesect(const aabb_t& other) const
  {
    bool no_intersect_x =
      min.data[0] > other.max.data[0] || max.data[0] < other.min.data[0];
    bool no_intersect_y =
      min.data[1] > other.max.data[1] || max.data[1] < other.min.data[1];
    bool no_intersect_z =
      min.data[2] > other.max.data[2] || max.data[2] < other.min.data[2];
    return !(no_intersect_x || no_intersect_y || no_intersect_z);
  }

  point3f min;
  point3f max;
};

}