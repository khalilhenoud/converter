/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <cassert>
#include <vector>
#include <string>
#include <assimp/types.h>
#include <assimp/material.h>
#include <library/string/fixed_string.h>
#include <serializer/serializer_scene_data.h>
#include <filesystem>


typedef struct allocator_t allocator_t;
struct aiScene;

inline
void
copy_str(fixed_str_t& target, const char* source, aiReturn do_copy) 
{
  if (do_copy == AI_SUCCESS) {
    memset(target.data, 0, sizeof(target.data));
    memcpy(target.data, source, strlen(source));
  }
}

inline
void
copy_float(float* target, float* source, aiReturn do_copy) 
{
  if (do_copy == AI_SUCCESS)
    *target = *source;
}

inline
void
copy_color(
  serializer_color_data_t* target, 
  aiColor4D* source, 
  aiReturn do_copy) 
{
  if (do_copy == AI_SUCCESS) {
    target->data[0] = source->r;
    target->data[1] = source->g;
    target->data[2] = source->b;
    target->data[3] = source->a;
  }
}

inline
void 
copy_texture_transform(
  serializer_texture_properties_t* target, 
  aiUVTransform* source,
  aiReturn do_copy) 
{
  if (do_copy == AI_SUCCESS) {
    target->angle = source->mRotation;
    target->u = source->mTranslation.x;
    target->v = source->mTranslation.y;
    target->u_scale = source->mScaling.x;
    target->v_scale = source->mScaling.y;
  }
}

inline
void
ensure_clean_directory(std::string directory)
{
  if (std::filesystem::exists(directory)) {
    // delete the folder if it already exists.
    bool success = std::filesystem::remove_all(directory);
    assert(success && "failed to remove the directory and its content");
  }

  // create the folder anew.
  bool success = std::filesystem::create_directory(directory);
  assert(success && "failed to create the directory");
}

inline
std::string
get_simple_name(std::string path)
{
  path = path.substr(path.find_last_of("/\\") + 1);
  std::string with_extension = path;
  std::string extension = 
    with_extension.substr(with_extension.find_last_of("."));
  return path.substr(0, path.length() - extension.length());
}

inline
void
copy_files(std::string target_dir, std::vector<std::string> files)
{
  for (auto& file : files) {
    auto target_path = file;
    target_path = target_path.substr(target_path.find_last_of("/\\") + 1);
    target_path = target_dir + target_path;
    bool success = std::filesystem::copy_file(file, target_path);
    assert(success);
  } 
}