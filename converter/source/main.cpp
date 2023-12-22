/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <vector>
#include <malloc.h>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <library/allocator/allocator.h>
#include <serializer/serializer_bin.h>
#include <converter/utils.h>
#include <converter/parsers/nodes.h>
#include <converter/parsers/meshes.h>
#include <converter/parsers/materials.h>
#include <converter/parsers/textures.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


std::vector<uintptr_t> allocated;
std::string data_folder = "..\\data\\rooms\\";

void* allocate(size_t size)
{
  void* block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void free_block(void* block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(), 
      allocated.end(), 
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }), 
    allocated.end());
  free(block);
}

int main(int argc, char *argv[])
{
  assert(argc >= 2 && "provide path to mesh file!");
  const char* mesh_file = argv[1];

  allocator_t allocator;
  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = NULL;
  allocator.mem_realloc = NULL;

  Assimp::Importer Importer;
  const aiScene* pScene = Importer.ReadFile(
    mesh_file, 
    aiProcess_Triangulate | 
    aiProcess_GenSmoothNormals | 
    aiProcess_FlipUVs | 
    aiProcess_JoinIdenticalVertices);

  if (!pScene)
    printf("Error parsing '%s': '%s'\n", mesh_file, Importer.GetErrorString());
  else {
    printf("Parsing was successful");

    serializer_scene_data_t* scene_bin = 
      (serializer_scene_data_t*)allocator.mem_alloc(
        sizeof(serializer_scene_data_t));

    auto textures = 
    populate_materials(scene_bin, pScene, &allocator);
    populate_textures(scene_bin, &allocator, textures);
    populate_meshes(scene_bin, pScene, &allocator);
    populate_nodes(scene_bin, pScene, &allocator);

    // get the trimmed file name, since I want to use it to create a folder.
    std::string name = get_simple_name(mesh_file);
    std::string target_path = data_folder + name;
    ensure_clean_directory(target_path);
    copy_files(target_path + "\\", textures);
    
    // serialize the bin file.
    std::string target_bin = target_path + "\\" + name + ".bin";
    serialize_bin(target_bin.c_str(), scene_bin);
    free_bin(scene_bin, &allocator);
  }
  
  assert(allocated.size() == 0);
  return 0;
}