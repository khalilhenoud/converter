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
#include <library/containers/cvector.h>
#include <entity/misc/font.h>
#include <entity/scene/scene.h>
#include <converter/parsers/assimp/fonts.h>


void
populate_default_font(
  scene_t *scene,
  const allocator_t *allocator)
{
  // set the font
  cvector_setup(&scene->font_repo, get_type_data(font_t), 4, allocator);
  cvector_resize(&scene->font_repo, 1);
  font_t *font = cvector_as(&scene->font_repo, 0, font_t);
  cstring_setup(&font->data_file, "\\font\\FontData.csv", allocator);
  cstring_setup(&font->image_file, "\\font\\ExportedFont.png", allocator);
}