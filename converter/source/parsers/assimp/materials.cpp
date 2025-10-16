/**
 * @file materials.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <cassert>
#include <functional>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/material.h>
#include <library/allocator/allocator.h>
#include <library/containers/cvector.h>
#include <library/string/cstring.h>
#include <entity/c/mesh/color.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/material.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/materials.h>


std::vector<std::string>
populate_materials(
  scene_t *scene, 
  const aiScene* pScene,
  const allocator_t* allocator)
{
  std::vector<std::string> textures;

  // NOTE: if pScene AI_SCENE_FLAGS_INCOMPLETE is set pScene might have no
  // materials.
  cvector_setup(&scene->material_repo, get_type_data(material_t), 4, allocator);
  cvector_resize(&scene->material_repo, pScene->mNumMaterials);

  for (uint32_t i = 0; i < scene->material_repo.size; ++i) {
    material_t *material = cvector_as(&scene->material_repo, i, material_t);
    aiMaterial *pMaterial = pScene->mMaterials[i];
    
    aiColor4D color;
    aiReturn value;
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &color);
    copy_color(&material->diffuse, &color, value);
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &color);
    copy_color(&material->ambient, &color, value);
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &color);
    copy_color(&material->specular, &color, value);
    
    float data_float = 1.f;
    value = aiGetMaterialFloat(pMaterial, AI_MATKEY_OPACITY, &data_float);
    copy_float(&material->opacity, &data_float, value);
    value = aiGetMaterialFloat(pMaterial, AI_MATKEY_SHININESS, &data_float);
    copy_float(&material->shininess, &data_float, value);

    aiString data_str;
    value = aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &data_str);
    assert(value == AI_SUCCESS);
    material->name = cstring_create(data_str.C_Str(), allocator);

    // Read the number of textures used by this material, stop at 8 (this is
    // max supported).
    material->textures.used = 0;
    uint32_t t_type_index = 1;
    uint32_t t_type_total = aiTextureType::AI_TEXTURE_TYPE_MAX;
    for (; t_type_index < t_type_total; ++t_type_index) {
      uint32_t t_total = aiGetMaterialTextureCount(
        pMaterial, 
        (aiTextureType)t_type_index);

      for (uint32_t t_index = 0; t_index < t_total; ++t_index) {
        aiString path;
        value = aiGetMaterialTexture(
          pMaterial, 
          (aiTextureType)t_type_index, 
          t_index, 
          &path);
        
        if (value == AI_SUCCESS) {
          auto iter = std::find(
            textures.begin(), 
            textures.end(), 
            path.C_Str());

          uint32_t global_texture_index = iter - textures.begin();

          if (iter == textures.end())
            textures.push_back(path.C_Str());
          
          texture_properties_t* texture_props = 
            material->textures.data + 
            material->textures.used++;
          texture_props->index = global_texture_index;

          aiUVTransform transform;
          value = aiGetMaterialUVTransform(
            pMaterial, 
            AI_MATKEY_UVTRANSFORM((aiTextureType)t_type_index, t_index), 
            &transform);
          copy_texture_transform(texture_props, &transform, value);

          if (material->textures.used == 8)
            break;
        }
      }

      if (material->textures.used == 8)
        break;
    }
  }

  return textures;
}