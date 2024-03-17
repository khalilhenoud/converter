/**
 * @file lights.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <cassert>
#include <functional>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <library/string/fixed_string.h>
#include <library/allocator/allocator.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/lights.h>
#include <serializer/serializer_scene_data.h>


void
populate_lights(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator)
{
  scene_bin->light_repo.used = pScene->mNumLights;
  scene_bin->light_repo.data = 
  (serializer_light_data_t*)allocator->mem_cont_alloc(
    sizeof(serializer_light_data_t), 
    scene_bin->light_repo.used);

  for (uint32_t i = 0; i < scene_bin->light_repo.used; ++i) {
    serializer_light_data_t *current = scene_bin->light_repo.data + i;
    aiLight* pLight = pScene->mLights[i];

    aiQuaternion rotation;
    aiVector3D position;
    aiVector3D direction, up;
    if (auto* node = pScene->mRootNode->FindNode(pLight->mName)) {
      aiMatrix4x4 &transform = node->mTransformation;
      position = transform * pLight->mPosition;
      direction = transform * pLight->mDirection;
      up = transform * pLight->mUp;
    }
    else
      assert(0);

    copy_str(current->name, pLight->mName.C_Str(), AI_SUCCESS);
    copy_vec3(&current->position, &position, AI_SUCCESS);
    copy_vec3(&current->direction, &direction, AI_SUCCESS);
    copy_vec3(&current->up, &up, AI_SUCCESS);
    copy_float(&current->inner_cone, &pLight->mAngleInnerCone, AI_SUCCESS);
    copy_float(&current->outer_cone, &pLight->mAngleOuterCone, AI_SUCCESS);
    copy_float(
      &current->attenuation_constant, 
      &pLight->mAttenuationConstant, 
      AI_SUCCESS);
    copy_float(
      &current->attenuation_linear, 
      &pLight->mAttenuationLinear, 
      AI_SUCCESS);
    copy_float(
      &current->attenuation_quadratic, 
      &pLight->mAttenuationQuadratic, 
      AI_SUCCESS);
    copy_color(&current->diffuse, &pLight->mColorDiffuse, AI_SUCCESS);
    copy_color(&current->specular, &pLight->mColorSpecular, AI_SUCCESS);
    copy_color(&current->ambient, &pLight->mColorAmbient, AI_SUCCESS);

    if (pLight->mType == aiLightSource_DIRECTIONAL)
      current->type = SERIALIZER_LIGHT_TYPE_DIRECTIONAL;
    else if (pLight->mType == aiLightSource_POINT)
      current->type = SERIALIZER_LIGHT_TYPE_POINT;
    else if (pLight->mType == aiLightSource_SPOT)
      current->type = SERIALIZER_LIGHT_TYPE_SPOT;
    else 
      // NOTE: aiLightSource_AMBIENT and aiLightSource_AREA are currently not 
      // supported. So we default them to a point light.
      current->type = SERIALIZER_LIGHT_TYPE_POINT;
  }
}