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
#include <cassert>
#include <library/allocator/allocator.h>
#include <converter/utils.h>
#include <converter/parsers/assimp/loader.h>
#include <converter/parsers/quake/loader.h>


std::string data_folder = "";
std::string tools_folder = "";

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
  allocator_t allocator;
  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = nullptr;

  assert(argc >= 4 && "provide path to mesh file!");
  data_folder = argv[1];
  tools_folder = argv[2];
  const char* scene_file = argv[3];

  if (get_extension(scene_file) == "map")
    load_qmap(scene_file, &allocator);
  else
    load_assimp(scene_file, &allocator);
  
  assert(allocated.size() == 0);
  return 0;
}