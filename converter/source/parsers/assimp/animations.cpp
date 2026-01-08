/**
 * @file animations.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-01-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <cassert>
#include <converter/parsers/assimp/animations.h>
#include <converter/utils.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <entity/scene/animation.h>
#include <entity/scene/scene.h>
#include <library/allocator/allocator.h>
#include <library/containers/cvector.h>
#include <library/string/cstring.h>


void
populate_animations(
  scene_t *scene,
  const aiScene *pScene,
  const allocator_t *allocator)
{
  cvector_setup(
    &scene->animation_repo, get_type_data(animation_t), 0, allocator);
  cvector_resize(&scene->animation_repo, pScene->mNumAnimations);

  for (uint32_t i = 0; i < pScene->mNumMeshes; ++i) {
    animation_t *anim = cvector_as(&scene->animation_repo, i, animation_t);
    aiAnimation *aiAnim = pScene->mAnimations[i];

    cstring_setup(&anim->name, aiAnim->mName.C_Str(), allocator);
    anim->duration = (float)aiAnim->mDuration;
    anim->ticks_per_second = aiAnim->mTicksPerSecond;

    cvector_setup(&anim->channels, get_type_data(anim_node_t), 0, allocator);
    cvector_resize(&anim->channels, aiAnim->mNumChannels);

    for (uint32_t j = 0; j < aiAnim->mNumChannels; ++j) {
      anim_node_t *anim_node = cvector_as(&anim->channels, j, anim_node_t);
      aiNodeAnim *aiNodeAnim = aiAnim->mChannels[j];

      cstring_setup(&anim_node->name, aiNodeAnim->mNodeName.C_Str(), allocator);
      cvector_setup(
        &anim_node->position_keys, get_type_data(position_key_t), 0, allocator);
      cvector_resize(&anim_node->position_keys, aiNodeAnim->mNumPositionKeys);

      for (uint32_t k = 0; k < aiNodeAnim->mNumPositionKeys; ++k) {
        position_key_t *position = cvector_as(
          &anim_node->position_keys, k, position_key_t);
        aiVectorKey *aiPosition = aiNodeAnim->mPositionKeys + k;
        position->time = (float)aiPosition->mTime;
        copy_vec3(&position->value, &aiPosition->mValue, AI_SUCCESS);
      }

      cvector_setup(
        &anim_node->rotation_keys, get_type_data(rotation_key_t), 0, allocator);
      cvector_resize(&anim_node->rotation_keys, aiNodeAnim->mNumRotationKeys);

      for (uint32_t k = 0; k < aiNodeAnim->mNumRotationKeys; ++k) {
        rotation_key_t *rotation = cvector_as(
          &anim_node->rotation_keys, k, rotation_key_t);
        aiQuatKey *aiRotation = aiNodeAnim->mRotationKeys + k;
        rotation->time = (float)aiRotation->mTime;
        copy_quat(&rotation->value, &aiRotation->mValue, AI_SUCCESS);
      }

      cvector_setup(
        &anim_node->scale_keys, get_type_data(scale_key_t), 0, allocator);
      cvector_resize(&anim_node->scale_keys, aiNodeAnim->mNumScalingKeys);

      for (uint32_t k = 0; k < aiNodeAnim->mNumScalingKeys; ++k) {
        scale_key_t *scale = cvector_as(&anim_node->scale_keys, k, scale_key_t);
        aiVectorKey *aiScale = aiNodeAnim->mScalingKeys + k;
        scale->time = (float)aiScale->mTime;
        copy_vec3(&scale->value, &aiScale->mValue, AI_SUCCESS);
      }
    }
  }
}