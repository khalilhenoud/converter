/**
 * @file string_utils.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-07-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <algorithm>
#include <converter/parsers/quake/string_utils.h>


namespace string_utils {

bool
replace(
  std::string& str,
  const std::string& from,
  const std::string& to)
{
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

std::string
get_sanitized(std::string str)
{
  std::transform(str.begin(), str.end(), str.begin(), 
    [](unsigned char c) { return std::tolower(c); });
  replace(str, "*", "star_");
  replace(str, "+", "plus_");
  replace(str, "-", "minu_");
  replace(str, "/", "divd_");
  return str;
}

}