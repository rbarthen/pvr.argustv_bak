/*
 *  Copyright (C) 2020-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2010 Marcel Groothuis
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#if defined(TARGET_WINDOWS)
#pragma warning(disable : 4244) //wchar to char = loss of data
#endif

#include "utils.h"

#include "addon.h"

#include <algorithm> // sort
#include <kodi/General.h>
#include <string>

#include <cstdarg>
#include <kodi/Filesystem.h>
#include <stdio.h>


#define FORMAT_BLOCK_SIZE 2048 // # of bytes to increment per try

namespace Json
{
void printValueTree(const Json::Value& value, const std::string& path)
{
  switch (value.type())
  {
    case Json::nullValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=null\n", path.c_str());
      break;
    case Json::intValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=%d\n", path.c_str(), value.asInt());
      break;
    case Json::uintValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=%u\n", path.c_str(), value.asUInt());
      break;
    case Json::realValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=%.16g\n", path.c_str(), value.asDouble());
      break;
    case Json::stringValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=\"%s\"\n", path.c_str(), value.asString().c_str());
      break;
    case Json::booleanValue:
      kodi::Log(ADDON_LOG_DEBUG, "%s=%s\n", path.c_str(), value.asBool() ? "true" : "false");
      break;
    case Json::arrayValue:
    {
      kodi::Log(ADDON_LOG_DEBUG, "%s=[]\n", path.c_str());
      int size = value.size();
      for (int index = 0; index < size; ++index)
      {
        static char buffer[16];
        snprintf(buffer, 16, "[%d]", index);
        printValueTree(value[index], path + buffer);
      }
    }
    break;
    case Json::objectValue:
    {
      kodi::Log(ADDON_LOG_DEBUG, "%s={}\n", path.c_str());
      Json::Value::Members members(value.getMemberNames());
      std::sort(members.begin(), members.end());
      std::string suffix = *(path.end() - 1) == '.' ? "" : ".";
      for (Json::Value::Members::iterator it = members.begin(); it != members.end(); ++it)
      {
        const std::string& name = *it;
        printValueTree(value[name], path + suffix + name);
      }
    }
    break;
    default:
      break;
  }
}
} //namespace Json

namespace BASE64
{

static const char* to_base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";

std::string b64_encode(unsigned char const* in, unsigned int in_len, bool urlEncode)
{
  std::string ret;
  int i(3);
  unsigned char c_3[3];
  unsigned char c_4[4];

  while (in_len)
  {
    i = in_len > 2 ? 3 : in_len;
    in_len -= i;
    c_3[0] = *(in++);
    c_3[1] = i > 1 ? *(in++) : 0;
    c_3[2] = i > 2 ? *(in++) : 0;

    c_4[0] = (c_3[0] & 0xfc) >> 2;
    c_4[1] = ((c_3[0] & 0x03) << 4) + ((c_3[1] & 0xf0) >> 4);
    c_4[2] = ((c_3[1] & 0x0f) << 2) + ((c_3[2] & 0xc0) >> 6);
    c_4[3] = c_3[2] & 0x3f;

    for (int j = 0; (j < i + 1); ++j)
    {
      if (urlEncode && to_base64[c_4[j]] == '+')
        ret += "%2B";
      else if (urlEncode && to_base64[c_4[j]] == '/')
        ret += "%2F";
      else
        ret += to_base64[c_4[j]];
    }
  }
  while ((i++ < 3))
    ret += urlEncode ? "%3D" : "=";
  return ret;
}

} //Namespace BASE64

namespace Utils
{

// format related string functions taken from:
// http://www.flipcode.com/archives/Safe_sprintf.shtml

bool Str2Bool(const std::string& str)
{
  return str.compare("True") == 0 ? true : false;
}

// Split function borrowed from pvr.wmc for GetRecordingEdl
std::vector<std::string> Split(const std::string& input,
                               const std::string& delimiter,
                               unsigned int iMaxStrings /* = 0 */)
{
  std::vector<std::string> results;
  if (input.empty())
    return results;

  size_t iPos = std::string::npos;
  size_t newPos = std::string::npos;
  size_t sizeS2 = delimiter.size();
  size_t isize = input.size();

  std::vector<unsigned int> positions;

  newPos = input.find(delimiter, 0);

  if (newPos == std::string::npos)
  {
    results.push_back(input);
    return results;
  }

  while (newPos != std::string::npos)
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.find(delimiter, iPos + sizeS2);
  }

  // numFound is the number of delimiters which is one less
  // than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
    numFound = iMaxStrings - 1;

  for (unsigned int i = 0; i <= numFound; i++)
  {
    std::string s;
    if (i == 0)
    {
      if (i == numFound)
        s = input;
      else
        s = input.substr(i, positions[i]);
    }
    else
    {
      size_t offset = positions[i - 1] + sizeS2;
      if (offset < isize)
      {
        if (i == numFound)
          s = input.substr(offset);
        else if (i > 0)
          s = input.substr(positions[i - 1] + sizeS2, positions[i] - positions[i - 1] - sizeS2);
      }
    }
    results.push_back(s);
  }
  return results;
}

std::string Format(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::string str = FormatV(fmt, args);
  va_end(args);

  return str;
}

std::string FormatV(const char* fmt, va_list args)
{
  if (fmt == nullptr)
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  char* cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
  if (cstr == nullptr)
    return "";

  while (1)
  {
    va_copy(argCopy, args);

    int nActual = vsnprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::string str(cstr, nActual);
      free(cstr);
      return str;
    }
    if (nActual > -1) // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else // Let's try to double the size (glibc 2.0)
      size *= 2;

    char* new_cstr = reinterpret_cast<char*>(realloc(cstr, sizeof(char) * size));
    if (new_cstr == nullptr)
    {
      free(cstr);
      return "";
    }

    cstr = new_cstr;
  }

  free(cstr);
  return "";
}

bool EndsWith(std::string const& fullString, std::string const& ending)
{
  if (fullString.length() >= ending.length())
  {
    return (0 ==
            fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
  }
  else
  {
    return false;
  }
}

bool StartsWith(std::string const& fullString, std::string const& starting)
{
  if (fullString.length() >= starting.length())
  {
    return (0 == fullString.compare(0, starting.length(), starting));
  }
  else
  {
    return false;
  }
}

// return the directory from the input file path
std::string GetDirectoryPath(std::string const& path)
{
  size_t found = path.find_last_of("/\\");
  if (found != std::string::npos)
    return path.substr(0, found);
  else
    return path;
}

bool ReadFileContents(std::string const& strFileName, std::string& strContent)
{
  kodi::vfs::CFile fileHandle;
  if (fileHandle.OpenFile(strFileName))
  {
    std::string buffer;
    while (fileHandle.ReadLine(buffer))
      strContent.append(buffer);
    return true;
  }
  return false;
}

bool WriteFileContents(std::string const& strFileName, const std::string& strContent)
{
  kodi::vfs::CFile fileHandle;
  if (fileHandle.OpenFileForWrite(strFileName, true))
  {
    int rc = fileHandle.Write(strContent.c_str(), strContent.length());
    if (rc)
    {
      kodi::Log(ADDON_LOG_DEBUG, "wrote file %s", strFileName.c_str());
    }
    else
    {
      kodi::Log(ADDON_LOG_ERROR, "can not write to %s", strFileName.c_str());
    }
    return rc >= 0;
  }
  return false;
}

} /* namespace Utils */

// transform [\\nascat\qrecordings\NCIS\2012-05-15_20-30_SBS 6_NCIS.ts]
// into      [smb://user:password@nascat/qrecordings/NCIS/2012-05-15_20-30_SBS 6_NCIS.ts]
std::string ToCIFS(std::string& UNCName)
{
  std::string CIFSname = UNCName;
  std::string SMBPrefix = "smb://";
  size_t found;
  while ((found = CIFSname.find("\\")) != std::string::npos)
  {
    CIFSname.replace(found, 1, "/");
  }
  CIFSname.erase(0, 2);
  CIFSname.insert(0, SMBPrefix);
  return CIFSname;
}

bool InsertUser(const CArgusTVAddon& base, std::string& UNCName)
{
  if (base.GetSettings().User().empty())
    return false;

  if (UNCName.find("smb://") == 0)
  {
    std::string SMBPrefix = "smb://" + base.GetSettings().User();

    if (!base.GetSettings().Pass().empty())
      SMBPrefix.append(":" + base.GetSettings().Pass());

    SMBPrefix.append("@");

    UNCName.replace(0, std::string("smb://").length(), SMBPrefix);
    kodi::Log(ADDON_LOG_DEBUG, "Account Info added to SMB url");
    return true;
  }
  return false;
}


// transform [smb://user:password@nascat/qrecordings/NCIS/2012-05-15_20-30_SBS 6_NCIS.ts]
// into      [\\nascat\qrecordings\NCIS\2012-05-15_20-30_SBS 6_NCIS.ts]
std::string ToUNC(std::string& CIFSName)
{
  std::string UNCname = CIFSName;

  UNCname.erase(0, 6);
  size_t found;
  while ((found = UNCname.find("/")) != std::string::npos)
  {
    UNCname.replace(found, 1, "\\");
  }
  UNCname.insert(0, "\\\\");
  return UNCname;
}

std::string ToUNC(const char* CIFSName)
{
  std::string temp = CIFSName;
  return ToUNC(temp);
}

//////////////////////////////////////////////////////////////////////////////
