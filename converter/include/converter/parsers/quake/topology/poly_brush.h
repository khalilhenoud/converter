/**
 * @file poly_brush.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-10-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <functional>
#include <converter/parsers/quake/topology/brush.h>


namespace topology {

// poly_brush_t is optimized for welding, this would be useful when welding many
// brushes together
class poly_brush_t {
  static constexpr float s_value = 1.f/16.f;

public:
  poly_brush_t(const brush_t* brush, const float radius = s_value);

  std::vector<face_t>
  to_faces() const;

  static
  void
  weld(
    std::vector<poly_brush_t*>& brushes, 
    const float radius = s_value);

  static
  void
  sort_and_weld(
    std::vector<poly_brush_t>& brushes, 
    const float radius = s_value);

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