/**
 * @file polygon.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <converter/parsers/quake/topology/texture_data.h>
#include <converter/parsers/quake/topology/edge.h>
#include <math/c/vector3f.h>
#include <math/c/face.h>


namespace topology {

struct face_t {
  ::face_t face;
  vector3f normal;
  std::string texture;
  texture_data_t texture_data;
  texture_info_t texture_info;
  point3f uv[3];
};

using plane_t = face_t;

struct polygon_t {

  struct clip_data_t {
    std::shared_ptr<polygon_t> front = nullptr;
    std::shared_ptr<polygon_t> back = nullptr;
    std::shared_ptr<edge_t> edge = nullptr;
  };

  // clips the poly against a plane of the brush and return clip data containing
  // front and back splits and incident edge (whenever available)
  clip_data_t
  clip(const plane_t& plane) const;

  std::vector<face_t>
  triangulate() const;

  // removes redundant vertices and edges, returns whether we have a valid face
  bool
  sanitize();

  std::vector<point3f> points;
  vector3f normal;
  std::string texture;
  texture_data_t texture_data;
  texture_info_t texture_info;
};

}