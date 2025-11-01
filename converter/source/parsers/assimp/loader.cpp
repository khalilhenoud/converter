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
#include <library/containers/cvector.h>
#include <library/streams/binary_stream.h>
#include <library/filesystem/io.h>
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
#include <converter/parsers/assimp/fonts.h>
#include <converter/parsers/assimp/bvhs.h>
#include <entity/c/scene/scene.h>


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

    scene_t *scene = scene_create(NULL, allocator);
    auto textures = 
    populate_materials(scene, pScene, allocator);
    populate_textures(scene, allocator, textures);
    populate_lights(scene, pScene, allocator);
    populate_meshes(scene, pScene, allocator);
    populate_nodes(scene, pScene, allocator);
    populate_cameras(scene, pScene, allocator);
    populate_default_font(scene, allocator);
    populate_bvhs(scene, allocator);

    // get the trimmed file name, since I want to use it to create a folder.
    std::string name = get_simple_name(scene_file);
    std::string target_path = data_folder + name;
    ensure_clean_directory(target_path);
    std::string texture_target_path = target_path + "\\textures";
    ensure_clean_directory(texture_target_path);
    copy_files(texture_target_path + "\\", textures);
    
    // serialize the bin file.
    std::string target_bin = target_path + "\\" + name + ".bin";
    binary_stream_t stream;
    binary_stream_def(&stream);
    binary_stream_setup(&stream, allocator);
    scene_serialize(scene, &stream);

    {
      file_handle_t file;
      file = open_file(target_bin.c_str(), 
        file_open_flags_t(FILE_OPEN_MODE_WRITE | FILE_OPEN_MODE_BINARY));
      assert((void *)file != NULL);
      write_buffer(
        file, 
        stream.data->data, stream.data->elem_data.size, stream.data->size);
      close_file(file);
    }
    
    binary_stream_cleanup(&stream);
    scene_free(scene, allocator);
  }

  printf("\n");
  printf("done!");
}