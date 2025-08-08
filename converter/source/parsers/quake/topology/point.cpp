/**
 * @file point.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <cstring>
#include <converter/parsers/quake/topology/point.h>
#include <converter/parsers/quake/topology/texture_data.h>


namespace topology {

////////////////////////////////////////////////////////////////////////////////
// NOTE: this code is from the gtkradiant project.
static
vector3f base_axis[18] =
{
	{0,0,1}, {1,0,0}, {0,-1,0},     // floor
	{0,0,-1}, {1,0,0}, {0,-1,0},    // ceiling
	{1,0,0}, {0,1,0}, {0,0,-1},     // west wall
	{-1,0,0}, {0,1,0}, {0,0,-1},    // east wall
	{0,1,0}, {1,0,0}, {0,0,-1},     // south wall
	{0,-1,0}, {1,0,0}, {0,0,-1}     // north wall
};

static
void
get_texture_axis_from_plane(
  vector3f& xv, 
  vector3f& yv,
  const vector3f& normal)
{
	int32_t best_axis = 0;
	float dot, best = 0.f;

  for (int32_t i = 0; i < 6; ++i) {
    dot = dot_product_v3f(&normal, &base_axis[i * 3]);
    if (dot > best) {
      best = dot;
      best_axis = i;
    }
  }

  xv = base_axis[best_axis * 3 + 1];
  yv = base_axis[best_axis * 3 + 2];
}

static
void
get_face_texture_vectors(
  const texture_data_t& texture_data,
  const texture_info_t& texture_info,
  const vector3f& normal,
  float STfromXYZ[2][4])
{
	vector3f pvecs[2];
	int32_t sv, tv;
	float ang, sinv, cosv;
	float ns, nt;
	int32_t i, j;
  texture_data_t td = texture_data;

	memset(STfromXYZ, 0, 8 * sizeof(float));

  td.scale[0] = td.scale[0] == 0 ? 1.f : td.scale[0];
  td.scale[1] = td.scale[1] == 0 ? 1.f : td.scale[1];

	// get natural texture axis
	get_texture_axis_from_plane(pvecs[0], pvecs[1], normal);

	// rotate axis
	if (td.rotation == 0) {
		sinv = 0; 
    cosv = 1;
	} else if (td.rotation == 90) {
		sinv = 1; 
    cosv = 0;
	} else if (td.rotation == 180) {
		sinv = 0; 
    cosv = -1;
	} else if (td.rotation == 270) {
		sinv = -1; 
    cosv = 0;
	} else {
		ang = TO_RADIANS(td.rotation);
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (pvecs[0].data[0])
		sv = 0;
	else if (pvecs[0].data[1])
		sv = 1;
	else
		sv = 2;

	if (pvecs[1].data[0])
		tv = 0;
	else if (pvecs[1].data[1])
		tv = 1;
	else
		tv = 2;

	for (i = 0; i < 2; ++i) {
		ns = cosv * pvecs[i].data[sv] - sinv * pvecs[i].data[tv];
		nt = sinv * pvecs[i].data[sv] + cosv * pvecs[i].data[tv];
		STfromXYZ[i][sv] = ns;
		STfromXYZ[i][tv] = nt;
	}

	// scale
	for (i = 0; i < 2; ++i)
		for (j = 0; j < 3; ++j)
			STfromXYZ[i][j] = STfromXYZ[i][j] / td.scale[i];

	// shift
	STfromXYZ[0][3] = (float)td.offset[0];
	STfromXYZ[1][3] = (float)td.offset[1];

	for (j = 0; j < 4; ++j) {
		STfromXYZ[0][j] /= (float)texture_info.width;
		STfromXYZ[1][j] /= (float)texture_info.height;
	}
}

point3f
get_texture_coordinates(
  const point3f& point, 
  const texture_data_t& texture_data,
  const texture_info_t& texture_info,
  const vector3f& normal)
{
  float STfromXYZ[2][4];
	get_face_texture_vectors(texture_data, texture_info, normal, STfromXYZ);

  point3f p[2] = {
    { STfromXYZ[0][0], STfromXYZ[0][1], STfromXYZ[0][2] },
    { STfromXYZ[1][0], STfromXYZ[1][1], STfromXYZ[1][2] }};
	
  point3f uvw = { 0, 0, 0 };
	uvw.data[0] = dot_product_v3f(&point, p + 0) + STfromXYZ[0][3];
	uvw.data[1] = dot_product_v3f(&point, p + 1) + STfromXYZ[1][3];
	// NOTE: we flip since we are dealing with pngs.
	uvw.data[1] *= -1.f;

  return uvw;
}
////////////////////////////////////////////////////////////////////////////////

}