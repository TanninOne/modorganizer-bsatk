/*
Mod Organizer BSA handling

Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "filehash.h"
#include <climits>
#include <cstring>
#include <cstdlib>
#include <algorithm>


#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif // MAX_PATH


static unsigned long genHashInt(const unsigned char *pos, const unsigned char *end)
{
  unsigned long hash = 0;
  for (; pos < end; ++pos) {
    hash *= 0x1003f;
    hash += *pos;
  }
  return hash;
}


BSAHash calculateBSAHash(const std::string &fileName)
{
  char fileNameLower[FILENAME_MAX + 1];
  int i = 0;
  for (; i < FILENAME_MAX && fileName[i] != '\0'; ++i) {
    fileNameLower[i] = tolower(fileName[i]);
    if (fileNameLower[i] == '\\') {
      fileNameLower[i] = '/';
    }
  }
  fileNameLower[i] = '\0';

  unsigned char *fileNameLowerU = reinterpret_cast<unsigned char*>(fileNameLower);

  char* ext = strrchr(fileNameLower, '.');
  if (ext == NULL) {
    ext = fileNameLower + strlen(fileNameLower);
  }
  unsigned char *extU = reinterpret_cast<unsigned char*>(ext);

  int length = ext - fileNameLower;

  BSAHash hash = 0ULL;

  if (length > 0) {
    hash = *(extU - 1) |
           ((length > 2 ? *(ext - 2) : 0) << 8) |
           (length << 16) |
           (fileNameLowerU[0] << 24);
  }

  if (strlen(ext) > 0) {
    if (strcmp(ext + 1, "kf") == 0) {
      hash |= 0x80;
    } else if (strcmp(ext + 1, "nif") == 0) {
      hash |= 0x8000;
    } else if (strcmp(ext + 1, "dds") == 0) {
      hash |= 0x8080;
    } else if (strcmp(ext + 1, "wav") == 0) {
      hash |= 0x80000000;
    }

    BSAHash temp = static_cast<BSAHash>(genHashInt(
                fileNameLowerU + 1, extU - 2));
    temp += static_cast<BSAHash>(genHashInt(
                extU, extU + strlen(ext)));

    hash |= (temp & 0xFFFFFFFF) << 32;
  }

  return hash;
}
