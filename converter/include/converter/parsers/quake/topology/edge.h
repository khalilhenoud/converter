/**
 * @file edge.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <math/c/segment.h>
#include <converter/parsers/quake/topology/point.h>


namespace topology {

using edge_t = segment_t;

inline
bool 
valid_edge(const edge_t& edge)
{
  return !identical_points(edge.points[0], edge.points[1]);
}

}