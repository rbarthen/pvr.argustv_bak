/*
 *  Copyright (C) 2020-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2010 Marcel Groothuis
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <json/json.h>
#include <string>

#include "stdio.h"
#include <cstdarg>
#include <vector>

// don't use the UNUSED macro from StdString.h as it will generate "error: statement has no effect [-Werror=unused-value]"
#define NOTUSED(x) ((void)x)

class CArgusTVAddon;

namespace Json
{
/**
 * \brief  Json support function to print the response from AGUS TV.
 *         Can be used for debugging purposes.
 * \param  value  Reference to the Json::Value that you want to print
 * \param  path   Optional path
 */
void printValueTree(const Json::Value& value, const std::string& path = ".");
} // namespace Json

namespace BASE64
{
std::string b64_encode(unsigned char const* in, unsigned int in_len, bool urlEncode);
}

namespace Utils
{

std::vector<std::string> Split(const std::string& input,
                               const std::string& delimiter,
                               unsigned int iMaxStrings = 0);
bool Str2Bool(const std::string& str);
std::string Format(const char* fmt, ...);
std::string FormatV(const char* fmt, va_list args);
bool EndsWith(std::string const& fullString, std::string const& ending);
bool StartsWith(std::string const& fullString, std::string const& starting);
std::string GetDirectoryPath(std::string const& path);
bool ReadFileContents(std::string const& strFileName, std::string& strResult);
bool WriteFileContents(std::string const& strFileName, const std::string& strContent);

} /* namespace Utils */

std::string ToCIFS(std::string& UNCName);
std::string ToUNC(std::string& CIFSName);
std::string ToUNC(const char* CIFSName);
bool InsertUser(const CArgusTVAddon& base, std::string& UNCName);
