/**
 * @file bvh_utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CONVERTER_BVH_UTILS_H
#define CONVERTER_BVH_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct bvh_t bvh_t;
typedef struct scene_t scene_t;
typedef struct serializer_scene_data_t serializer_scene_data_t;
typedef struct allocator_t allocator_t;

bvh_t* 
create_bvh_from_scene(
  scene_t* scene, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif