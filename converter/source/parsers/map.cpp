/**
 * @file map.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <vector>
#include <list>
#include <deque>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <converter/parsers/map.h>
#include <math/c/vector3f.h>
#include <math/c/segment.h>
#include <math/c/face.h>
#include <library/allocator/allocator.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <loaders/loader_map_data.h>
#include <serializer/serializer_scene_data.h>


typedef
struct {
  face_t face;
  vector3f normal;
} l_face_t;

typedef
struct {
  std::deque<point3f> points;
} l_polygon_t;

typedef
struct {
  segment_t segment;
} l_edge_t;

typedef
struct {
  face_t face;
  vector3f normal;
} b_plane_t;

static
void
transform_cube(mesh_t* cube)
{
  // 4096 units both side of the three axis.
  float scale = 4096.f * 2;
  for (uint32_t i = 0; i < cube->vertices_count * 3; ++i)
    cube->vertices[i] *= scale;
}

inline
bool
identical_points(point3f& p1, point3f& p2, float epsilon = 1.f)
{
  vector3f diff = diff_v3f(&p1, &p2);
  return length_v3f(&diff) < epsilon;
}

inline
float
distance_points(point3f& p1, point3f& p2)
{
  vector3f diff = diff_v3f(&p1, &p2);
  return length_v3f(&diff);
}

static
std::vector<l_face_t>
triangulate_polygon(
  l_polygon_t& polygon, 
  const vector3f& normal)
{
  std::vector<l_face_t> tris;

  // iterate over all vertices check which qualify as an ear. pick the best
  // candidate.
  while (polygon.points.size() > 3) {
    std::unordered_map<uint32_t, float> i_to_angle;
    uint32_t count = polygon.points.size();
    for (uint32_t i_at = 0; i_at < count; ++i_at) {
      uint32_t i_before = (i_at + count - 1) % count;
      uint32_t i_after = (i_at + 1) % count;

      face_t tri = {
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

      face_t tri = {
        polygon.points[i_before], 
        polygon.points[i_at], 
        polygon.points[i_after]};
      tris.push_back({tri, normal});
      polygon.points.erase(polygon.points.begin() + i_at);
    }
  }

  face_t tri = {
    polygon.points[0],
    polygon.points[1],
    polygon.points[2] };
  tris.push_back({ tri, normal });

  return tris;
}

static
void
simplify_poly(l_polygon_t& poly)
{
  // simplify the poly, by removing colinear lines.
  for (uint32_t i = 1; i < poly.points.size(); ) {
    uint32_t count = poly.points.size();
    uint32_t i_at = i;
    uint32_t i_prev = (i + count - 1) % count;
    uint32_t i_next = (i + 1) % count;
    vector3f diff1, diff2;
    vector3f_set_diff_v3f(&diff1, &poly.points[i_prev], &poly.points[i_at]);
    normalize_set_v3f(&diff1);
    vector3f_set_diff_v3f(&diff2, &poly.points[i_at], &poly.points[i_next]);
    normalize_set_v3f(&diff2);
    float dot = dot_product_v3f(&diff1, &diff2);
    // we can remove the point.
    if (IS_SAME_LP(dot, 1.f))
      poly.points.erase(poly.points.begin() + i);
    else
      ++i;
  }

  // simplify the poly further, by removing duplicate subsequent vertices.
  for (uint32_t i = 0; i < poly.points.size(); ) {
    uint32_t count = poly.points.size();
    uint32_t i_at = i;
    uint32_t i_next = (i + 1) % count;
    point3f& p1 = poly.points[i_at];
    point3f& p2 = poly.points[i_next];
    if (identical_points(p1, p2))
      poly.points.erase(poly.points.begin() + i);
    else
      ++i;
  }
}

static
std::vector<l_face_t>
create_connecting_faces(
  const std::vector<l_face_t>& cut_faces, 
  std::list<l_edge_t>& edges)
{
  {
    // strip collapsed edges.
    auto iter = edges.begin();
    while (iter != edges.end()) {
      point3f& p1 = iter->segment.points[0];
      point3f& p2 = iter->segment.points[1];
      if (identical_points(p1, p2))
        edges.erase(iter++);
      else
        ++iter;
    }
  }

  // erroneous generation, early out.
  if (edges.size() < 2)
    return std::vector<l_face_t>{};

  // connect the edges that will make up the new polygon.
  l_polygon_t poly;
  std::list<l_edge_t>::iterator iter = edges.begin();
  poly.points.push_back(iter->segment.points[0]);
  poly.points.push_back(iter->segment.points[1]);
  edges.erase(iter++);
  point3f* back = &poly.points.back();

  // Closest distance and early out when closed makes more sense.
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
      distances[0] = distance_points(*back, iter->segment.points[0]);
      distances[1] = distance_points(*back, iter->segment.points[1]);
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
      if (identical_points(copy->segment.points[1], poly.points.front()))
        closed = true;
      else
        poly.points.push_back(copy->segment.points[1]);
    } else if (min_distance_i == 1) {
      if (identical_points(copy->segment.points[0], poly.points.front()))
        closed = true;
      else
        poly.points.push_back(copy->segment.points[0]);
    } else
      assert(min_distance_i != 2);

    edges.erase(copy);
    closed = edges.empty() ? true : closed;
    back = &poly.points.back();
  }

  edges.clear();

  simplify_poly(poly);

  // this could happen if the edges making up the poly are colinear
  if (poly.points.size() < 3)
    return std::vector<l_face_t>();

  // find the face normal, all vertices of cut_faces are on the other side.
  vector3f normal;
  {
    face_t nface = { poly.points[0], poly.points[1], poly.points[2] }; 
    get_faces_normals(&nface, 1, &normal);

    // check the first point that isn't on the face, the direction will tell us
    // whether we need to swap the normal sign or not.
    bool done = false;
    for (auto& face: cut_faces) {
      for (uint32_t i = 0; i < 3; ++i) {
        point_halfspace_classification_t side = classify_point_halfspace(
          &nface, &normal, face.face.points + i);

        if (side != POINT_ON_PLANE) {
          if (side == POINT_IN_POSITIVE_HALFSPACE) {
            mult_set_v3f(&normal, -1.f);
            std::reverse(poly.points.begin(), poly.points.end());
          }
          done = true;
          break;
        }
      }

      if (done)
        break;
    }
  }

  // triangulate the poly into triangles, use ear clipping.
  return triangulate_polygon(poly, normal);
}

std::vector<l_face_t>
brush_to_faces(
  std::vector<l_face_t> faces, 
  const loader_map_brush_data_t* brush)
{
  std::vector<b_plane_t> planes;
  // convert the brush planes to an easier format to work with.
  for (uint32_t i = 0; i < brush->face_count; ++i) {
    int32_t* data = brush->faces[i].data;
    point3f p1 = { (float)data[0], (float)data[1], (float)data[2] };
    point3f p2 = { (float)data[3], (float)data[4], (float)data[5] };
    point3f p3 = { (float)data[6], (float)data[7], (float)data[8] };
    // flip the 3rd and 2nd point to adhere to the way the quake format works
    face_t face = { p1, p3, p2 };
    vector3f normal;
    get_faces_normals(&face, 1, &normal);
    planes.push_back({ face, normal });
  }

  // cut the faces by the planes, keep only the parts of the faces that makes
  // sense and introduce the polygon that ties the edges together.
  for (auto& plane : planes) {
    std::list<l_edge_t> edges;
    std::vector<l_face_t> cut_faces;

    for (uint32_t iface = 0, count = faces.size(); iface < count; ++iface) {      
      l_face_t& current = faces[iface];

      point_halfspace_classification_t pt_classify[3];
      int32_t on_positive = 0, on_negative = 0, on_plane = 0;
      for (uint32_t ivert = 0; ivert < 3; ++ivert) {
        pt_classify[ivert] = classify_point_halfspace(
          &plane.face, 
          &plane.normal, 
          current.face.points + ivert);
        on_positive += pt_classify[ivert] == POINT_IN_POSITIVE_HALFSPACE;
        on_negative += pt_classify[ivert] == POINT_IN_NEGATIVE_HALFSPACE;
        on_plane += pt_classify[ivert] == POINT_ON_PLANE; 
      }

      // if no vertex is in the positive space then save the current face.
      if (on_positive == 0) 
        cut_faces.push_back(current);
      else if (on_negative == 0)
        ; // reject face wholly.
      else {
        // 2 cases in how the face should be split. depending whether any of the
        // verts is incident.
        if (on_plane != 0) {
          uint32_t index = 0;
          index = (pt_classify[1] == POINT_ON_PLANE) ? 1 : index;
          index = (pt_classify[2] == POINT_ON_PLANE) ? 2 : index;
          // the segment that needs to be split is [index + 1, index + 2];
          segment_t segment;
          segment.points[0] = current.face.points[(index + 1) % 3];
          segment.points[1] = current.face.points[(index + 2) % 3];
          point3f intersection;
          float t = 0.f;
          segment_plane_classification_t result = classify_segment_face(
            &plane.face, &plane.normal, &segment, &intersection, &t);
          assert(result == SEGMENT_PLANE_INTERSECT_ON_SEGMENT);

          // check which triangle needs to be added of the 2 split.
          if (pt_classify[(index + 1) % 3] == POINT_IN_NEGATIVE_HALFSPACE) {
            l_face_t back = { 
              { 
                current.face.points[index], 
                current.face.points[(index + 1) % 3], 
                intersection 
              }, current.normal};
            cut_faces.push_back(back);
          } else {
            l_face_t back = { 
              { 
                current.face.points[index],
                intersection,
                current.face.points[(index + 2) % 3], 
              }, current.normal};
            cut_faces.push_back(back);
          }

          // add the index-intersection segment to the edges list.
          edges.push_back( { current.face.points[index], intersection} );
        } else {
          // we need to go through each segment making up the face and split it
          l_polygon_t poly_front;
          l_polygon_t poly_back;
          segment_t edge;
          uint32_t edge_index = 0;

          for (uint32_t ivert = 0; ivert < 3; ++ivert) {
            uint32_t idx0 = (ivert + 0) % 3; 
            uint32_t idx1 = (ivert + 1) % 3; 
            // both segment vertices are on one side of the plane.
            if (pt_classify[idx0] == pt_classify[idx1]) {
              if (pt_classify[idx0] == POINT_IN_POSITIVE_HALFSPACE)
                poly_front.points.push_back(current.face.points[idx0]);
              else
                poly_back.points.push_back(current.face.points[idx0]);
            } else {
              // intersection calculation is required here.
              segment_t segment;
              segment.points[0] = current.face.points[idx0];
              segment.points[1] = current.face.points[idx1];

              point3f intersection;
              float t = 0.f;
              segment_plane_classification_t result = classify_segment_face(
                &plane.face, &plane.normal, &segment, &intersection, &t);
              // TODO: change conditions, t could be 1.00000002
              //assert(result == SEGMENT_PLANE_INTERSECT_ON_SEGMENT);

              if (pt_classify[idx0] == POINT_IN_POSITIVE_HALFSPACE)
                poly_front.points.push_back(current.face.points[idx0]);
              else
                poly_back.points.push_back(current.face.points[idx0]);
              poly_front.points.push_back(intersection);
              poly_back.points.push_back(intersection);
              edge.points[edge_index++] = intersection;
              assert(edge_index <= 2);
            }
          }

          edges.push_back({edge});

          // triangulate poly_back and add the result to the cut_face list
          if (poly_back.points.size() == 3) {
            face_t back_tri = { 
              poly_back.points[0], 
              poly_back.points[1], 
              poly_back.points[2] };
            cut_faces.push_back({ back_tri, current.normal });
          } else if (poly_back.points.size() == 4) {
            face_t back_tri0 = { 
              poly_back.points[0], 
              poly_back.points[1], 
              poly_back.points[2] };
            cut_faces.push_back({ back_tri0, current.normal });
            face_t back_tri1 = { 
              poly_back.points[0], 
              poly_back.points[2], 
              poly_back.points[3] };
            cut_faces.push_back({ back_tri1, current.normal });
          } else
            assert(0 && "it is either 3 sides or 4 sides! this makes no sense");
        }
      }
    }

    // we choose 2 because we assume the last 2 vertices auto connect, subject
    // to change.
    assert(edges.size() == 0 || edges.size() >= 2);

    // if the edges are valid, create a polygon to fill the gap and add it to
    // cut_faces.
    auto connecting = create_connecting_faces(cut_faces, edges);
    cut_faces.insert(cut_faces.end(), connecting.begin(), connecting.end());
    
    // use the new cut face list.
    faces = cut_faces;
  }

  return faces;
}

// returns serializer_mesh_data_t* typecast to void*.
static
void
map_to_serializer_mesh(
  loader_map_data_t* map_data, 
  serializer_mesh_data_t* result, 
  const allocator_t* allocator)
{
  mesh_t* cube = create_unit_cube(allocator);
  transform_cube(cube);

  // convert the cube to a more manageable format.
  std::vector<l_face_t> faces;
  point3f p1, p2, p3;
  vector3f normal;
  size_t vec3f = sizeof(float) * 3;
  for (uint32_t i = 0; i < cube->indices_count; i +=3) {
    memcpy(p1.data, cube->vertices + cube->indices[i + 0] * 3, vec3f);    
    memcpy(p2.data, cube->vertices + cube->indices[i + 1] * 3, vec3f);    
    memcpy(p3.data, cube->vertices + cube->indices[i + 2] * 3, vec3f);
    memcpy(normal.data, cube->normals + cube->indices[i + 0] * 3, vec3f);
    faces.push_back({ {p1, p2, p3}, normal});
  }

  {
    std::vector<l_face_t> map_faces;
    for (uint32_t i = 0; i < map_data->world.brush_count; ++i) {
      auto cut_faces = brush_to_faces(faces, map_data->world.brushes + i);
      map_faces.insert(map_faces.end(), cut_faces.begin(), cut_faces.end());
    }

    {
      uint32_t count = map_faces.size();
      uint32_t sizef3 = sizeof(float) * 3;
      // populate the resultant mesh from the faces.
      result->vertices_count = map_faces.size() * 3;
      result->vertices = (float*)allocator->mem_alloc(
        sizef3 * result->vertices_count);
      result->normals = (float*)allocator->mem_alloc(
        sizef3 * result->vertices_count);
      result->uvs = (float*)allocator->mem_alloc(
        sizef3 * result->vertices_count);
      memset(result->uvs, 0, sizef3 * result->vertices_count);

      result->faces_count = map_faces.size();
      result->indices = (uint32_t*)allocator->mem_alloc(
        sizeof(uint32_t) * result->faces_count * 3);

      // copy the data into the mesh.
      uint32_t verti = 0, indexi = 0;
      for (uint32_t i = 0; i < count; ++i) {
        auto& face = map_faces[i];
        point3f* points = face.face.points;
        memcpy(result->vertices + (verti + 0) * 3, points[0].data, sizef3);
        memcpy(result->vertices + (verti + 1) * 3, points[1].data, sizef3);
        memcpy(result->vertices + (verti + 2) * 3, points[2].data, sizef3);
        memcpy(result->normals + (verti + 0) * 3, face.normal.data, sizef3);
        memcpy(result->normals + (verti + 1) * 3, face.normal.data, sizef3);
        memcpy(result->normals + (verti + 2) * 3, face.normal.data, sizef3);

        result->indices[indexi + 0] = verti + 0;
        result->indices[indexi + 1] = verti + 1;
        result->indices[indexi + 2] = verti + 2;

        indexi += 3;
        verti += 3;
      }
    }
  }

  free_mesh(cube, allocator);
}

serializer_scene_data_t*
map_to_bin(
  loader_map_data_t* map_data, 
  const allocator_t* allocator)
{
  serializer_scene_data_t* scene =
    (serializer_scene_data_t*)allocator->mem_alloc(
      sizeof(serializer_scene_data_t));
  memset(scene, 0, sizeof(serializer_scene_data_t));
  scene->mesh_repo.used = 1;
  scene->mesh_repo.data = 
    (serializer_mesh_data_t*)allocator->mem_alloc(
      sizeof(serializer_mesh_data_t));
  memset(scene->mesh_repo.data, 0, sizeof(serializer_mesh_data_t));
  map_to_serializer_mesh(map_data, scene->mesh_repo.data, allocator);
  scene->mesh_repo.data->materials.used = 1;
  scene->mesh_repo.data->materials.indices[0] = 0;
  
  scene->material_repo.used = 1;
  scene->material_repo.data = 
    (serializer_material_data_t*)allocator->mem_alloc(
      sizeof(serializer_material_data_t));
  memset(scene->material_repo.data, 0, sizeof(serializer_material_data_t));
  scene->material_repo.data->opacity = 1.f;
  scene->material_repo.data->shininess = 1.f;
  scene->material_repo.data->ambient.data[0] =
  scene->material_repo.data->ambient.data[1] =
  scene->material_repo.data->ambient.data[2] = 0.2f;
  scene->material_repo.data->ambient.data[3] = 1.f;
  scene->material_repo.data->diffuse.data[0] =
  scene->material_repo.data->diffuse.data[1] =
  scene->material_repo.data->diffuse.data[2] = 0.6f;
  scene->material_repo.data->diffuse.data[3] = 1.f;
  scene->material_repo.data->specular.data[0] =
  scene->material_repo.data->specular.data[1] =
  scene->material_repo.data->specular.data[2] = 0.6f;
  scene->material_repo.data->specular.data[3] = 1.f;

  scene->model_repo.used = 1;
  scene->model_repo.data = 
    (serializer_model_data_t*)allocator->mem_alloc(
      sizeof(serializer_model_data_t));
  memset(scene->model_repo.data, 0, sizeof(serializer_model_data_t));
  scene->model_repo.data[0].meshes.used = 1;
  scene->model_repo.data[0].meshes.indices[0] = 0;
  scene->model_repo.data[0].models.used = 0;
  // TODO: set the name here.
  // scene->model_repo.data[0].name.data;
  matrix4f_rotation_x(&scene->model_repo.data[0].transform, -K_PI/2.f);
  scene->camera_repo.used = 1;
  scene->camera_repo.data = 
    (serializer_camera_t*)allocator->mem_alloc(
      sizeof(serializer_camera_t));
  scene->camera_repo.data[0].position.data[0] = 
    (float)map_data->player_start[0];
  scene->camera_repo.data[0].position.data[1] = 
    (float)map_data->player_start[1];
  scene->camera_repo.data[0].position.data[2] = 
    (float)map_data->player_start[2];

  scene->light_repo.used = 1;
  scene->light_repo.data = 
    (serializer_light_data_t*)allocator->mem_alloc(
      sizeof(serializer_light_data_t));
  memset(scene->light_repo.data, 0, sizeof(serializer_light_data_t));
  scene->light_repo.data->type = SERIALIZER_LIGHT_TYPE_POINT;
  scene->light_repo.data->ambient.data[0] =
  scene->light_repo.data->ambient.data[1] =
  scene->light_repo.data->ambient.data[2] = 0.1f;
  scene->light_repo.data->ambient.data[3] = 1.f;
  scene->light_repo.data->diffuse.data[0] =
  scene->light_repo.data->diffuse.data[1] =
  scene->light_repo.data->diffuse.data[2] = 0.2f;
  scene->light_repo.data->diffuse.data[3] = 1.f;
  scene->light_repo.data->specular.data[0] =
  scene->light_repo.data->specular.data[1] =
  scene->light_repo.data->specular.data[2] = 0.f;
  scene->light_repo.data->specular.data[3] = 1.f;

  // premultiply the position.
  mult_set_m4f_p3f(
    &scene->model_repo.data[0].transform, &scene->camera_repo.data[0].position);
  scene->camera_repo.data[0].lookat_direction.data[0] = 
  scene->camera_repo.data[0].lookat_direction.data[1] = 0.f;
  scene->camera_repo.data[0].lookat_direction.data[2] = -1.f;
  scene->camera_repo.data[0].up_vector.data[0] = 
  scene->camera_repo.data[0].up_vector.data[2] = 0.f;
  scene->camera_repo.data[0].up_vector.data[1] = 1.f;
  return scene;
}