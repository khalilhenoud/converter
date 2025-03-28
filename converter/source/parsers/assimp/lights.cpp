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
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <entity/c/scene/light.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/lights.h>


void
populate_lights(
  scene_t *scene, 
  const aiScene* pScene, 
  const allocator_t* allocator)
{
  scene->light_repo.count = pScene->mNumLights;
  scene->light_repo.lights = (light_t*)allocator->mem_cont_alloc(
    sizeof(light_t), scene->light_repo.count);

  for (uint32_t i = 0; i < scene->light_repo.count; ++i) {
    light_t *light = scene->light_repo.lights + i;
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

    light->name = cstring_create(pLight->mName.C_Str(), allocator);
    copy_vec3(&light->position, &position, AI_SUCCESS);
    copy_vec3(&light->direction, &direction, AI_SUCCESS);
    copy_vec3(&light->up, &up, AI_SUCCESS);
    copy_float(&light->inner_cone, &pLight->mAngleInnerCone, AI_SUCCESS);
    copy_float(&light->outer_cone, &pLight->mAngleOuterCone, AI_SUCCESS);
    copy_float(
      &light->attenuation_constant, 
      &pLight->mAttenuationConstant, 
      AI_SUCCESS);
    copy_float(
      &light->attenuation_linear, 
      &pLight->mAttenuationLinear, 
      AI_SUCCESS);
    copy_float(
      &light->attenuation_quadratic, 
      &pLight->mAttenuationQuadratic, 
      AI_SUCCESS);
    copy_color(&light->diffuse, &pLight->mColorDiffuse, AI_SUCCESS);
    copy_color(&light->specular, &pLight->mColorSpecular, AI_SUCCESS);
    copy_color(&light->ambient, &pLight->mColorAmbient, AI_SUCCESS);

    if (pLight->mType == aiLightSource_DIRECTIONAL)
      light->type = LIGHT_TYPE_DIRECTIONAL;
    else if (pLight->mType == aiLightSource_POINT)
      light->type = LIGHT_TYPE_POINT;
    else if (pLight->mType == aiLightSource_SPOT)
      light->type = LIGHT_TYPE_SPOT;
    else 
      // NOTE: aiLightSource_AMBIENT and aiLightSource_AREA are currently not 
      // supported. So we default them to a point light.
      light->type = LIGHT_TYPE_POINT;
  }
}