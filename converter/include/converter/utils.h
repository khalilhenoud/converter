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
#include <filesystem>
#include <assimp/types.h>
#include <assimp/material.h>
#include <math/quatf.h>
#include <math/vector3f.h>
#include <entity/mesh/material.h>
#include <entity/mesh/color.h>


typedef struct allocator_t allocator_t;
struct aiScene;

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
  color_rgba_t* target,
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
copy_color(
  color_rgba_t* target,
  aiColor3D* source,
  aiReturn do_copy)
{
  if (do_copy == AI_SUCCESS) {
    target->data[0] = source->r;
    target->data[1] = source->g;
    target->data[2] = source->b;
    target->data[3] = 1.f;
  }
}

inline
void
copy_vec3(
  vector3f *target,
  aiVector3D *source,
  aiReturn do_copy)
{
  if (do_copy == AI_SUCCESS) {
    target->data[0] = source->x;
    target->data[1] = source->y;
    target->data[2] = source->z;
  }
}

inline
void
copy_quat(
  quatf *target,
  aiQuaternion *source,
  aiReturn do_copy)
{
  if (do_copy == AI_SUCCESS) {
    target->data[QUAT_S] = source->w;
    target->data[QUAT_X] = source->x;
    target->data[QUAT_Y] = source->y;
    target->data[QUAT_Z] = source->z;
  }
}

inline
void
copy_texture_transform(
  texture_properties_t* target,
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
std::vector<std::filesystem::path>
get_all_files_in_directory(std::string directory)
{
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(directory))
    files.push_back(entry.path());
  return files;
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
std::string
get_extension(std::string path)
{
  path = path.substr(path.find_last_of("/\\") + 1);
  std::string with_extension = path;
  std::string extension =
    with_extension.substr(with_extension.find_last_of(".") + 1);
  return extension;
}

inline
std::vector<std::string>
filter_existing(std::vector<std::string>& files)
{
  std::vector<std::string> non_existing;
  auto iter = files.begin();
  while (iter != files.end()) {
    if (!std::filesystem::exists(*iter)) {
      non_existing.push_back(*iter);
      iter = files.erase(iter);
      continue;
    }

    ++iter;
  }

  return non_existing;
}

inline
void
replace_missing_files(
  std::string target_dir,
  std::vector<std::string> files,
  std::string defaulted)
{
  for (auto& file : files) {
    auto target_path = file;
    target_path = target_path.substr(target_path.find_last_of("/\\") + 1);
    target_path = target_dir + target_path;
    bool success = std::filesystem::copy_file(defaulted, target_path);
    assert(success);
  }
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