/**
 * @file poly_brush.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-10-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <converter/parsers/quake/topology/poly_brush.h>


namespace topology {

poly_brush_t::poly_brush_t(const brush_t* brush, const float radius)
  : polygons{brush->to_polygons()}
{
  const float r2 = radius * radius;
  std::vector<uint32_t> hits;

  auto is_at = [this, r2, &hits](const point3f& position) -> int32_t {
    for (uint32_t i = 0, count = meta.positions.size(); i < count; ++i) {
      point3f target = meta.positions[i];
      mult_set_v3f(&target, 1.f/(float)hits[i]);

      float length = distance_points_squared(position, target);
      if (length <= r2)
        return (int32_t)i;
    }

    return -1;
  };

  for (auto& polygon : polygons) {
    // create the indexing structure per polygon
    meta.polygons.push_back(indexed_poly_t{});
    indexed_poly_t& indexed_poly = meta.polygons.back();

    for (auto& vertex : polygon.points) {
      int32_t index = is_at(vertex);

      if (index == -1) {
        indexed_poly.indices.push_back(meta.positions.size());
        meta.positions.push_back(vertex);
        hits.push_back(1);
      } else {
        indexed_poly.indices.push_back((uint32_t)index);
        add_set_v3f(&meta.positions[index], &vertex);
        hits[index]++;
      }
    }   
  }

  // normalize meta.positions
  for (uint32_t i = 0, count = meta.positions.size(); i < count; ++i) {
    point3f& target = meta.positions[i];
    mult_set_v3f(&target, 1.f/(float)hits[i]);
  }

  // update the polygons vertices with the averaged positions
  for (uint32_t i = 0, count = meta.polygons.size(); i < count; ++i) {
    indexed_poly_t& indexed_poly = meta.polygons[i];
    polygon_t& polygon = polygons[i];
    for (uint32_t j = 0; j < indexed_poly.indices.size(); ++j)
      polygon.points[j] = meta.positions[indexed_poly.indices[j]];
  }
}

void
poly_brush_t::weld(
  std::vector<poly_brush_t*>& brushes,
  const float radius)
{
  const float r2 = radius * radius;
  using vertex_indices_t = std::vector<uint32_t>;

  std::vector<vertex_indices_t> brushes_meta;
  std::vector<point3f> positions;
  std::vector<uint32_t> hits;

  auto is_at = [r2, &hits, &positions](const point3f& position) -> int32_t {
    for (uint32_t i = 0, count = positions.size(); i < count; ++i) {
      point3f target = positions[i];
      mult_set_v3f(&target, 1.f/(float)hits[i]);

      float length = distance_points_squared(position, target);
      if (length <= r2)
        return (int32_t)i;
    }

    return -1;
  };

  for (poly_brush_t* brush : brushes) {
    brushes_meta.emplace_back();
    // maps the current brush meta positions to the intra-brushes shared ones
    vertex_indices_t& to_index = brushes_meta.back();

    for (point3f& vertex : brush->meta.positions) {
      int32_t index = is_at(vertex);
      if (index == -1) {
        to_index.push_back(positions.size());
        positions.push_back(vertex);
        hits.push_back(1);
      } else {
        to_index.push_back((uint32_t)index);
        add_set_v3f(&positions[index], &vertex);
        hits[index]++;
      }
    }
  }

  // normalize the positions
  for (uint32_t i = 0, count = positions.size(); i < count; ++i) {
    point3f& target = positions[i];
    mult_set_v3f(&target, 1.f/(float)hits[i]);
  }

  // copy back the positions into the brushes meta data and their polygons
  for (uint32_t i = 0, count = brushes.size(); i < count; ++i) {
    poly_brush_t* brush = brushes[i];
    vertex_indices_t& to_index = brushes_meta[i];

    // copy back into meta.positions of the current brush
    for (uint32_t j = 0; j < brush->meta.positions.size(); ++j)
      brush->meta.positions[j] = positions[to_index[j]];

    // update the orginal polygons
    for (uint32_t j = 0; j < brush->meta.polygons.size(); ++j) {
      indexed_poly_t& indexed_poly = brush->meta.polygons[j];
      polygon_t& polygon = brush->polygons[j];
      
      for (uint32_t k = 0; k < indexed_poly.indices.size(); ++k)
        polygon.points[k] = brush->meta.positions[indexed_poly.indices[k]];
    }
  }
}

std::vector<face_t>
poly_brush_t::to_faces() const
{
  std::vector<face_t> faces;
  for (auto& polygon : polygons) {
    auto tris = polygon.triangulate();
    faces.insert(faces.end(), tris.begin(), tris.end());
  }
  
  return faces;
}

}