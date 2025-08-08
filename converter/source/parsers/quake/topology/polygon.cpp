/**
 * @file polygon.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <algorithm>
#include <unordered_map>
#include <converter/parsers/quake/topology/polygon.h>
#include <converter/parsers/quake/topology/point.h>


namespace topology {

polygon_t::clip_data_t
polygon_t::clip(const plane_t& plane) const
{
  clip_data_t data;

  std::vector<point_halfspace_classification_t> pt_classify;
  int32_t on_positive = 0, on_negative = 0;
  for (auto& point : points) {
    auto classification = classify_point_halfspace(
      &plane.face, &plane.normal, &point);
    pt_classify.push_back(classification);
    on_positive += classification == POINT_IN_POSITIVE_HALFSPACE;
    on_negative += classification == POINT_IN_NEGATIVE_HALFSPACE; 
  }

  // to the back or colinear with the plane add to the back
  if (on_positive == 0 || (on_positive == 0 && on_negative == 0))
    data.back = std::make_shared<polygon_t>(*this);
  else if (on_negative == 0)
    data.front = std::make_shared<polygon_t>(*this);
  else {
    data.front = std::make_shared<polygon_t>(*this);
    data.front->points.clear();
    data.back = std::make_shared<polygon_t>(*this);
    data.back->points.clear();
    data.edge = std::make_shared<edge_t>();
    uint32_t edge_index = 0;

    auto&& classify_add = [&](uint32_t index) {
      if (pt_classify[index] == POINT_IN_POSITIVE_HALFSPACE)
        data.front->points.push_back(points[index]);
      else if (pt_classify[index] == POINT_IN_NEGATIVE_HALFSPACE)
        data.back->points.push_back(points[index]);
      else {
        data.front->points.push_back(points[index]);
        data.back->points.push_back(points[index]);
        data.edge->points[edge_index++] = points[index];
        assert(edge_index <= 2);
      }
    };

    for (uint32_t i = 0, count = points.size(); i < count; ++i) {
      uint32_t idx0 = i; 
      uint32_t idx1 = (i + 1) % count; 
      
      // no splitting is required, simply classify add the starting point
      if (
        pt_classify[idx0] == pt_classify[idx1] || 
        pt_classify[idx0] == POINT_ON_PLANE || 
        pt_classify[idx1] == POINT_ON_PLANE)
        classify_add(idx0);
      else {
        // intersection calculation is required here.
        segment_t segment;
        segment.points[0] = points[idx0];
        segment.points[1] = points[idx1];

        point3f intersection;
        float t = 0.f;
        segment_plane_classification_t result = classify_segment_face(
          &plane.face, &plane.normal, &segment, &intersection, &t);

        if (pt_classify[idx0] == POINT_IN_POSITIVE_HALFSPACE)
          data.front->points.push_back(points[idx0]);
        else
          data.back->points.push_back(points[idx0]);
        data.front->points.push_back(intersection);
        data.back->points.push_back(intersection);
        data.edge->points[edge_index++] = intersection;
        assert(edge_index <= 2);
      }
    }
  }

  return data;
}

std::vector<face_t>
polygon_t::triangulate() const
{
  std::vector<face_t> tris;
  polygon_t polygon = *this;

  // iterate to locate best candidate for ear clipping
  while (polygon.points.size() > 3) {
    std::unordered_map<uint32_t, float> i_to_angle;
    uint32_t count = polygon.points.size();
    for (uint32_t i_at = 0; i_at < count; ++i_at) {
      uint32_t i_before = (i_at + count - 1) % count;
      uint32_t i_after = (i_at + 1) % count;

      ::face_t tri = {
        polygon.points[i_before], 
        polygon.points[i_at], 
        polygon.points[i_after]};

      vector3f tri_normal;
      get_faces_normals(&tri, 1, &tri_normal);
      // if it is convex, make sure no other vertex is within the tri.
      if (dot_product_v3f(&normal, &tri_normal) > 0) {
        bool valid = true;

        // the vertex is only considered if no other vertex is in the ear.
        for (uint32_t vert_i = 0; vert_i < count; ++vert_i) {
          if (vert_i == i_at || vert_i == i_before || vert_i == i_after)
            continue;

          point3f closest;
          auto pt_iter = polygon.points.begin() + vert_i;
          coplanar_point_classification_t classify = 
          classify_coplanar_point_face(
            &tri, 
            &tri_normal, 
            &(*pt_iter), 
            &closest);

          if (classify == COPLANAR_POINT_ON_OR_INSIDE) {
            valid = false;
            break;
          }
        }

        // get the angle, we will need this.
        if (valid) {
          vector3f vec0, vec1;
          vector3f_set_diff_v3f(&vec0, tri.points + 0, tri.points + 1);
          normalize_set_v3f(&vec0);
          vector3f_set_diff_v3f(&vec1, tri.points + 1, tri.points + 2);
          normalize_set_v3f(&vec1);
          float dot = dot_product_v3f(&vec0, &vec1);
          if (dot < 0) 
            dot = K_PI - acosf(fabs(dot));
          else
            dot = acosf(dot);
          i_to_angle[i_at] = TO_DEGREES(dot);
        }
      }
    }

    {
      // clip the ear with the largest angle.
      auto ear = std::max_element(
        std::begin(i_to_angle), 
        std::end(i_to_angle), 
        [](
          const std::pair<uint32_t, float>& p1, 
          const std::pair<uint32_t, float>& p2) {
          return p1.second < p2.second;});

      uint32_t i_at = ear->first;
      uint32_t i_before = (i_at + count - 1) % count;
      uint32_t i_after = (i_at + 1) % count;

      ::face_t tri = {
        polygon.points[i_before], 
        polygon.points[i_at], 
        polygon.points[i_after]};

      point3f uv[3];
      uv[0] = get_texture_coordinates(
        tri.points[0], polygon.texture_data, polygon.texture_info, normal);
      uv[1] = get_texture_coordinates(
        tri.points[1], polygon.texture_data, polygon.texture_info, normal);
      uv[2] = get_texture_coordinates(
        tri.points[2], polygon.texture_data, polygon.texture_info, normal);

      tris.push_back({
        tri,
        normal,
        polygon.texture,
        polygon.texture_data,
        polygon.texture_info, 
        { uv[0], uv[1], uv[2] }});
      polygon.points.erase(polygon.points.begin() + i_at);
    }
  }

  ::face_t tri = {
    polygon.points[0],
    polygon.points[1],
    polygon.points[2] };
  
  point3f uv[3];
  uv[0] = get_texture_coordinates(
    tri.points[0], polygon.texture_data, polygon.texture_info, normal);
  uv[1] = get_texture_coordinates(
    tri.points[1], polygon.texture_data, polygon.texture_info, normal);
  uv[2] = get_texture_coordinates(
    tri.points[2], polygon.texture_data, polygon.texture_info, normal);
  
  tris.push_back({
    tri,
    normal,
    polygon.texture,
    polygon.texture_data,
    polygon.texture_info,
    { uv[0], uv[1], uv[2] }});

  return tris;
}

bool
polygon_t::sanitize()
{
  // simplify the poly by removing duplicate subsequent vertices
  for (uint32_t i = 0; i < points.size(); ) {
    uint32_t count = points.size();
    uint32_t i_at = i;
    uint32_t i_next = (i + 1) % count;
    point3f& p1 = points[i_at];
    point3f& p2 = points[i_next];
    if (identical_points(p1, p2))
      points.erase(points.begin() + i);
    else
      ++i;
  }

  // simplify the polygon, by removing colinear lines
  for (uint32_t i = 1; i < points.size(); ) {
    uint32_t count = points.size();
    uint32_t i_at = i;
    uint32_t i_prev = (i + count - 1) % count;
    uint32_t i_next = (i + 1) % count;
    vector3f diff1, diff2;
    vector3f_set_diff_v3f(&diff1, &points[i_prev], &points[i_at]);
    normalize_set_v3f(&diff1);
    vector3f_set_diff_v3f(&diff2, &points[i_at], &points[i_next]);
    normalize_set_v3f(&diff2);
    float dot = dot_product_v3f(&diff1, &diff2);
    // we can remove the point.
    if (IS_SAME_LP(dot, 1.f))
      points.erase(points.begin() + i);
    else
      ++i;
  }

  // this could happen if the edges making up the poly are colinear
  return points.size() >= 3;
}

}