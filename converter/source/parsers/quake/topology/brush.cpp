/**
 * @file brush.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2025-07-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <converter/parsers/quake/string_utils.h>
#include <converter/parsers/quake/topology/brush.h>
#include <converter/parsers/quake/topology/edge.h>
#include <loaders/loader_map.h>
#include <math/vector3f.h>
#include <math/matrix4f.h>
#include <math/face.h>


namespace topology {

static
polygon_t
make_cube_face(
  const float scale,
  const matrix4f* transform)
{
  const float unit = 0.5f * scale;
  polygon_t polygon;

  polygon.normal = { 0, 0, 1.f };
  mult_set_m4f_v3f(transform, &polygon.normal);

  polygon.points.push_back({unit, -unit, unit});
  polygon.points.push_back({-unit, -unit, unit});
  polygon.points.push_back({-unit, unit, unit});
  polygon.points.push_back({unit, unit, unit});

  for (uint32_t i = 0; i < 4; ++i)
    mult_set_m4f_p3f(transform, &polygon.points[i]);

  return polygon;
}

std::vector<polygon_t>
make_cube(const float scale)
{
  constexpr float PI_D2 = K_PI / 2.f;
  std::vector<polygon_t> cube;

  matrix4f transform[6];
  matrix4f_set_identity(&transform[0]);
  matrix4f_rotation_y(&transform[1], PI_D2);
  matrix4f_rotation_y(&transform[2], K_PI);
  matrix4f_rotation_y(&transform[3], 3.f * PI_D2);
  matrix4f_rotation_x(&transform[4], PI_D2);
  matrix4f_rotation_x(&transform[5], 3.f * PI_D2);

  for (uint32_t i = 0; i < 6; ++i)
    cube.push_back(make_cube_face(scale, transform + i));

  return cube;
}

brush_t::brush_t(
    const loader_map_brush_data_t* brush,
    std::unordered_map<std::string, texture_info_t>& textures_info)
{
  for (uint32_t i = 0, count = brush->face_count; i < count; ++i) {
    int32_t* data = brush->faces[i].data;
    point3f p1 = { (float)data[0], (float)data[1], (float)data[2] };
    point3f p2 = { (float)data[3], (float)data[4], (float)data[5] };
    point3f p3 = { (float)data[6], (float)data[7], (float)data[8] };

    // flip the 3rd and 2nd point to adhere to the way the quake format works
    ::face_t face = { p1, p3, p2 };
    vector3f normal;
    get_faces_normals(&face, 1, &normal);

    // read the texture data associated with the plane
    texture_data_t texture_data = {
        { brush->faces[i].offset[0], brush->faces[i].offset[1] },
        brush->faces[i].rotation,
        { brush->faces[i].scale[0], brush->faces[i].scale[1] }
      };

    std::string texture = string_utils::get_sanitized(brush->faces[i].texture);

    // read the texture width/height
    texture_info_t texture_info;
    if (texture.size() != 0)
        texture_info = textures_info[texture];

    planes.push_back({ face, normal, texture, texture_data, texture_info });
  }
}

std::vector<polygon_t>
brush_t::to_polygons() const
{
  std::vector<polygon_t> stone = make_cube();

  for (auto& brush_plane : planes) {
    std::list<edge_t> intersection;
    std::vector<polygon_t> cut_stone;

    for (auto& face : stone) {
      polygon_t::clip_data_t data = face.clip(brush_plane);

      if (data.back && !data.front)
        cut_stone.push_back(*data.back);
      else if (!data.back && data.front)
        continue;
      else {
        cut_stone.push_back(*data.back);
        intersection.push_back(*data.edge);
      }
    }

    auto any = make_face(brush_plane, intersection);
    if (any.has_value())
      cut_stone.push_back(*any);

    stone = cut_stone;
  }

  return stone;
}

std::optional<polygon_t>
brush_t::make_face(
  const plane_t& plane,
  const std::list<edge_t>& edge_list)
{
  std::list<edge_t> edges = edge_list;

  {
    // strip collapsed edges
    auto iter = edges.begin();
    while (iter != edges.end()) {
      if (!valid_edge(*iter))
        edges.erase(iter++);
      else
        ++iter;
    }

    // erroneous generation, early out
    if (edges.size() < 2)
      return std::nullopt;
  }

  // connect the edges that will make up the new polygon
  polygon_t polygon;
  polygon.normal = plane.normal;
  polygon.texture = plane.texture;
  polygon.texture_data = plane.texture_data;
  polygon.texture_info = plane.texture_info;
  auto iter = edges.begin();
  polygon.points.push_back(iter->points[0]);
  polygon.points.push_back(iter->points[1]);
  edges.erase(iter++);
  point3f* back = &polygon.points.back();

  // closest distance and early out when closed, it is a more robust approach
  float min_distance;
  uint32_t min_distance_i;
  bool closed = false;
  while (!closed) {
    auto iter = edges.begin();
    auto copy = iter;
    min_distance = FLT_MAX;
    min_distance_i = 2;
    for (; iter != edges.end(); ++iter) {
      float distances[2];
      distances[0] = distance_points(*back, iter->points[0]);
      distances[1] = distance_points(*back, iter->points[1]);
      if (min_distance > distances[0]) {
        min_distance = distances[0];
        min_distance_i = 0;
        copy = iter;
      }

      if (min_distance > distances[1]) {
        min_distance = distances[1];
        min_distance_i = 1;
        copy = iter;
      }
    }

    if (min_distance_i == 0) {
      if (identical_points(copy->points[1], polygon.points.front()))
        closed = true;
      else
        polygon.points.push_back(copy->points[1]);
    } else if (min_distance_i == 1) {
      if (identical_points(copy->points[0], polygon.points.front()))
        closed = true;
      else
        polygon.points.push_back(copy->points[0]);
    } else
      assert(min_distance_i != 2);

    edges.erase(copy);
    closed = edges.empty() ? true : closed;
    back = &polygon.points.back();
  }

  edges.clear();

  if (!polygon.sanitize())
    return std::nullopt;

  // flip the ordering if needed
  vector3f normal;
  ::face_t nface = { polygon.points[0], polygon.points[1], polygon.points[2] };
  get_faces_normals(&nface, 1, &normal);

  if (dot_product_v3f(&normal, &polygon.normal) < 0.f)
    std::reverse(polygon.points.begin(), polygon.points.end());

  return polygon;
}

}