/**
 * @file cameras.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <cassert>
#include <functional>
#include <assimp/scene.h>
#include <assimp/camera.h>
#include <assimp/types.h>
#include <library/allocator/allocator.h>
#include <library/string/fixed_string.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/lights.h>
#include <serializer/serializer_scene_data.h>


void
populate_cameras(
  serializer_scene_data_t* scene_bin, 
  const aiScene* pScene, 
  const allocator_t* allocator)
{
  scene_bin->camera_repo.used = pScene->mNumCameras;
  scene_bin->camera_repo.data = 
  (serializer_camera_t*)allocator->mem_cont_alloc(
    sizeof(serializer_camera_t), 
    scene_bin->camera_repo.used);

  for (uint32_t i = 0; i < scene_bin->camera_repo.used; ++i) {
    serializer_camera_t *current = scene_bin->camera_repo.data + i;
    aiCamera* pCamera = pScene->mCameras[i];

    aiQuaternion rotation;
    aiVector3D position;
    aiVector3D direction, up;
    if (auto* node = pScene->mRootNode->FindNode(pCamera->mName)) {
      aiMatrix4x4 &transform = node->mTransformation;
      position = transform * pCamera->mPosition;
      direction = transform * pCamera->mLookAt;
      up = transform * pCamera->mUp;
    }
    else
      assert(0);

    // camera has no name right now
    // copy_str(current->name, pLight->mName.C_Str(), AI_SUCCESS);
    copy_vec3(&current->position, &position, AI_SUCCESS);
    copy_vec3(&current->lookat_direction, &direction, AI_SUCCESS);
    copy_vec3(&current->up_vector, &up, AI_SUCCESS);
  }
}