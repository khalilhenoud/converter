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
#include <library/string/cstring.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/textures.h>


void
populate_textures(
  scene_t* scene,
  const allocator_t* allocator,
  std::vector<std::string> textures)
{
  // adding the textures
  scene->texture_repo.count = (uint32_t)textures.size();
  if (scene->texture_repo.count) {
    scene->texture_repo.textures = (texture_t*)allocator->mem_cont_alloc(
      sizeof(texture_t), scene->texture_repo.count);

    for (uint32_t i = 0; i < scene->texture_repo.count; ++i) {
      // only keep the file name.
      textures[i] = textures[i].substr(textures[i].find_last_of("/\\") + 1);
      scene->texture_repo.textures[i].path = cstring_create(
        textures[i].c_str(), allocator);
    }
  }
}