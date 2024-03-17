/**
 * @file textures.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/material.h>
#include <library/allocator/allocator.h>
#include <library/string/fixed_string.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/textures.h>
#include <serializer/serializer_scene_data.h>


void
populate_textures(
  serializer_scene_data_t* scene_bin,
  const allocator_t* allocator,
  std::vector<std::string> textures)
{
  // adding the textures
  scene_bin->texture_repo.used = (uint32_t)textures.size();
  if (scene_bin->texture_repo.used) {
    scene_bin->texture_repo.data =
      (serializer_texture_data_t*)allocator->mem_cont_alloc(
        sizeof(serializer_texture_data_t),
        scene_bin->texture_repo.used);

    for (uint32_t i = 0; i < scene_bin->texture_repo.used; ++i) {
      memset(
        scene_bin->texture_repo.data[i].path.data, 0,
        sizeof(scene_bin->texture_repo.data[i].path.data));
      // only keep the file name.
      textures[i] = textures[i].substr(textures[i].find_last_of("/\\") + 1);
      memcpy(
        scene_bin->texture_repo.data[i].path.data,
        textures[i].c_str(),
        strlen(textures[i].c_str()));
    }
  }
}