/**
 * @file string_utils.h
 * @author khalilhenoud@gmail.com
 * @brief non-general string functions to be used with the map parser.
 * @version 0.1
 * @date 2025-07-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <string>


namespace string_utils {

bool
replace(
  std::string& str, 
  const std::string& from, 
  const std::string& to);

// NOTE: qpakman replaces some wad file tokens with an OS safe replacement.
std::string
get_sanitized(std::string str);

}