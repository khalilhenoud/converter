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
#include <library/string/cstring.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/scene.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/lights.h>


void
populate_cameras(
  scene_t *scene, 
  const aiScene *pScene, 
  const allocator_t *allocator)
{
  scene->camera_repo.count = pScene->mNumCameras;
  scene->camera_repo.cameras = (camera_t*)allocator->mem_cont_alloc(
    sizeof(camera_t), scene->camera_repo.count);

  for (uint32_t i = 0; i < scene->camera_repo.count; ++i) {
    camera_t *camera = scene->camera_repo.cameras + i;
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
    copy_vec3(&camera->position, &position, AI_SUCCESS);
    copy_vec3(&camera->lookat_direction, &direction, AI_SUCCESS);
    copy_vec3(&camera->up_vector, &up, AI_SUCCESS);
  }
}