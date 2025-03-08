/**
 * @file fonts.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <cassert>
#include <functional>
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <entity/c/misc/font.h>
#include <entity/c/scene/scene.h>
#include <converter/parsers/assimp/fonts.h>


void
populate_default_font(
  scene_t *scene,
  const allocator_t *allocator)
{
  // set the font
  scene->font_repo.count = 1;
  scene->font_repo.fonts = (font_t *)allocator->mem_alloc(sizeof(font_t));
  scene->font_repo.fonts[0].data_file = cstring_create(
    "\\font\\FontData.csv", allocator);
  scene->font_repo.fonts[0].image_file = cstring_create(
    "\\font\\ExportedFont.png", allocator);
}