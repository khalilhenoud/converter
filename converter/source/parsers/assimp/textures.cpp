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
#include <library/containers/cvector.h>
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
  cvector_def(&scene->texture_repo);

  if (textures.size()) {
    cvector_setup(&scene->texture_repo, get_type_data(texture_t), 4, allocator);
    cvector_resize(&scene->texture_repo, textures.size());

    for (uint32_t i = 0; i < scene->texture_repo.size; ++i) {
      // only keep the file name.
      textures[i] = textures[i].substr(textures[i].find_last_of("/\\") + 1);
      texture_t *texture = cvector_as(&scene->texture_repo, i, texture_t);
      cstring_setup(&texture->path, textures[i].c_str(), allocator);
    }
  }
}