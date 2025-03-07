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
#include <serializer/utils.h>
#include <converter/parsers/quake/loader.h>
#include <converter/utils.h>
#include <converter/parsers/quake/map.h>
#include <loaders/loader_map.h>
#include <entity/c/scene/scene.h>
#include <library/containers/cvector.h>
#include <library/streams/binary_stream.h>


extern std::string data_folder;
extern std::string tools_folder;

void
load_qmap(
  const char* scene_file, 
  const allocator_t* allocator)
{
  loader_map_data_t* map = load_map(scene_file, allocator);
  printf("loading map file successful!");

#if 0
  serializer_scene_data_t* scene_bin = 
    (serializer_scene_data_t*)allocator->mem_alloc(
      sizeof(serializer_scene_data_t));
  memset(scene_bin, 0, sizeof(serializer_scene_data_t));
  std::vector<std::string> textures = map_to_bin(
    scene_file, map, scene_bin, allocator);
#else
  scene_t *scene = scene_create(NULL, allocator);
  std::vector<std::string> textures = map_to_bin(
    scene_file, map, scene, allocator);
#endif

  free_map(map, allocator);
  
  // get the trimmed file name, since I want to use it to create a folder.
  std::string name = get_simple_name(scene_file);
  std::string target_path = data_folder + name;
  ensure_clean_directory(target_path);
  std::string texture_target_path = target_path + "\\textures";
  ensure_clean_directory(texture_target_path);
  copy_files(texture_target_path + "\\", textures);

  std::string target_bin = target_path + "\\" + name + ".bin";
#if 1
  {
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
  }
  scene_free(scene, allocator);
#else
  serialize_bin(target_bin.c_str(), scene_bin);
  free_bin(scene_bin, allocator);
#endif
  
  printf("done!");
}