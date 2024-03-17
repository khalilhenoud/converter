/**
 * @file loader.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-03-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <library/allocator/allocator.h>
#include <serializer/serializer_bin.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <converter/parsers/assimp/loader.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/nodes.h>
#include <converter/parsers/assimp/meshes.h>
#include <converter/parsers/assimp/materials.h>
#include <converter/parsers/assimp/textures.h>
#include <converter/parsers/assimp/lights.h>
#include <converter/parsers/assimp/cameras.h>


extern std::string data_folder;
extern std::string tools_folder;

void
load_assimp(
  const char* scene_file, 
  const allocator_t* allocator)
{
  Assimp::Importer Importer;
  const aiScene* pScene = Importer.ReadFile(
    scene_file, 
    aiProcess_Triangulate | 
    aiProcess_GenSmoothNormals | 
    aiProcess_FlipUVs | 
    aiProcess_JoinIdenticalVertices);

  if (!pScene)
    printf(
      "Error parsing '%s': '%s'\n", scene_file, 
      Importer.GetErrorString());
  else {
    printf("Parsing was successful");

    serializer_scene_data_t* scene_bin = 
      (serializer_scene_data_t*)allocator->mem_alloc(
        sizeof(serializer_scene_data_t));

    auto textures = 
    populate_materials(scene_bin, pScene, allocator);
    populate_textures(scene_bin, allocator, textures);
    populate_lights(scene_bin, pScene, allocator);
    populate_meshes(scene_bin, pScene, allocator);
    populate_nodes(scene_bin, pScene, allocator);
    populate_cameras(scene_bin, pScene, allocator);

    // get the trimmed file name, since I want to use it to create a folder.
    std::string name = get_simple_name(scene_file);
    std::string target_path = data_folder + name;
    ensure_clean_directory(target_path);
    std::string texture_target_path = target_path + "\\textures";
    ensure_clean_directory(texture_target_path);
    copy_files(texture_target_path + "\\", textures);
    
    // serialize the bin file.
    std::string target_bin = target_path + "\\" + name + ".bin";
    serialize_bin(target_bin.c_str(), scene_bin);
    free_bin(scene_bin, allocator);
  }
}