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
#include <library/string/fixed_string.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/materials.h>
#include <serializer/serializer_scene_data.h>


std::vector<std::string>
populate_materials(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene,
  const allocator_t* allocator)
{
  std::vector<std::string> textures;

  // reading the materials.
  // NOTE: if pScene AI_SCENE_FLAGS_INCOMPLETE is set pScene might have no
  // materials.
  scene_bin->material_repo.used = pScene->mNumMaterials;
  scene_bin->material_repo.data = 
    (serializer_material_data_t*)allocator->mem_cont_alloc(
      sizeof(serializer_material_data_t),
      scene_bin->material_repo.used);

  for (uint32_t i = 0; i < scene_bin->material_repo.used; ++i) {
    auto* bin_material = scene_bin->material_repo.data + i;
    aiMaterial* pMaterial = pScene->mMaterials[i];
    
    aiColor4D color;
    aiReturn value;
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &color);
    copy_color(&bin_material->diffuse, &color, value);
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &color);
    copy_color(&bin_material->ambient, &color, value);
    value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &color);
    copy_color(&bin_material->specular, &color, value);
    
    float data_float = 1.f;
    value = aiGetMaterialFloat(pMaterial, AI_MATKEY_OPACITY, &data_float);
    copy_float(&bin_material->opacity, &data_float, value);
    value = aiGetMaterialFloat(pMaterial, AI_MATKEY_SHININESS, &data_float);
    copy_float(&bin_material->shininess, &data_float, value);

    aiString data_str;
    value = aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &data_str);
    copy_str(bin_material->name, data_str.C_Str(), value);

    // Read the number of textures used by this material, stop at 8 (this is
    // max supported).
    bin_material->textures.used = 0;
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
          
          serializer_texture_properties_t* texture_props = 
            bin_material->textures.data + 
            bin_material->textures.used++;
          texture_props->index = global_texture_index;

          copy_str(
            texture_props->type, 
            aiTextureTypeToString((aiTextureType)t_type_index), 
            AI_SUCCESS);

          aiUVTransform transform;
          value = aiGetMaterialUVTransform(
            pMaterial, 
            AI_MATKEY_UVTRANSFORM((aiTextureType)t_type_index, t_index), 
            &transform);
          copy_texture_transform(texture_props, &transform, value);

          if (bin_material->textures.used == 8)
            break;
        }
      }

      if (bin_material->textures.used == 8)
        break;
    }
  }

  return textures;
}