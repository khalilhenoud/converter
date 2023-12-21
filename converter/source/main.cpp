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
#include <cassert>
#include <malloc.h>
#include <algorithm>
#include <functional>
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
  assert(argc >= 3 && "provide path to mesh file!");
  const char* mesh_file = argv[1];
  const char* target_file = argv[2];

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

    ::serialize_bin(target_file, scene_bin);
    ::free_bin(scene_bin, &allocator);
  }
  
  assert(allocated.size() == 0);
  return 0;
}