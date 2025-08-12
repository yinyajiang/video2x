#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string>

namespace image2x {


#ifdef _WIN32
typedef wchar_t CharType;
#define STR(x) L##x
#else
typedef char CharType;
#define STR(x) x
#endif

#ifdef _WIN32
typedef std::wstring StringType;
#else
typedef std::string StringType;
#endif

bool file_is_readable(const std::filesystem::path& path);

std::string path_to_u8string(const std::filesystem::path& path);

std::string string_type_to_u8string(const StringType &wstr);

StringType path_to_string_type(const std::filesystem::path& path);

StringType to_string_type(int value);

StringType utf8_to_string_type(std::string value);

}
