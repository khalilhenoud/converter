/**
 * @file brush.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-31
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <list>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <converter/parsers/quake/topology/polygon.h>
#include <converter/parsers/quake/topology/texture_data.h>


typedef struct loader_map_brush_data_t loader_map_brush_data_t;

namespace topology {

std::vector<polygon_t>
make_cube(const float scale = 8192.f);

class brush_t {
public:
  brush_t(
    const loader_map_brush_data_t* brush, 
    std::unordered_map<std::string, texture_info_t>& textures_info);

  std::vector<polygon_t>
  to_polygons() const;

  static
  std::optional<polygon_t>
  make_face(
    const plane_t& plane,
    const std::list<edge_t>& edge_list);

private:
  std::vector<plane_t> planes;
};

// poly_brush_t is optimized for welding, this would be useful when welding many
// brushes together
class poly_brush_t {
public:
  poly_brush_t(const brush_t* brush, const float radius = 1.f/16.f);

  std::vector<face_t>
  to_faces() const;

private:
  std::vector<polygon_t> polygons;
  
  struct indexed_poly_t {
    std::vector<uint32_t> indices;
  }; 

  struct weld_meta_t {
    std::vector<point3f> positions;
    std::vector<indexed_poly_t> polygons; // 1 to 1 with poly_brush_t::polygons
  } meta;
};

}