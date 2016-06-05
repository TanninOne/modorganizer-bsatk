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

#ifndef BSAFILE_H
#define BSAFILE_H


#include "filehash.h"
#include "errorcodes.h"
#include <fstream>
#include <vector>
#include <memory>


namespace BSA {


class Folder;
class Archive;

class File {

  friend class Folder;
  friend class Archive;

public:

  typedef std::shared_ptr<File> Ptr;
  friend bool ByOffset(const File::Ptr &LHS, const File::Ptr &RHS);

public:

  /**
   * @return the name of the file
   */
  const std::string &getName() const { return m_Name; }
  /**
   * @return full path of this file within the archive
   */
  std::string getFilePath() const;
  /**
   * @return size of the file. If the source is an archive and the file is
   *         compressed, this returns the compressed size!
   */
  BSAULong getFileSize() const { return m_FileSize; }

private:

  // copy constructor not implemented
  File(const File &reference);

  // assignment operator not implemented
  File& operator=(const File& reference);

  /**
   * construct file from source archive
   * @param file a file read from
   * @param folder the folder to add the file to
   */
  File(std::fstream &file, Folder *folder);

  /**
   * construct from loose file
   * @param name the base name of the file inside the archive
   * @param sourceFile the file to read from
   * @param folder the folder to add the file to
   * @param toggleCompressed if true, the default compression mode of the
   *                         archive is overwritten
   */
  File(const std::string &name, const std::string &sourceFile,
       Folder *folder, bool toggleCompressed);
  /**
   * @return true if its compression mode for this file differs from the archive default
   */
  bool compressToggled() const { return m_ToggleCompressed; }

  /**
   * @return offset to the file data. Only valid if the source of the file is
   *         an archive
   */
  BSAULong getDataOffset() const { return m_DataOffset; }
  void writeHeader(std::fstream &file) const;
  EErrorCode writeData(std::fstream &sourceArchive, std::fstream &targetArchive) const;

  void setFileSize(BSAULong fileSize) { m_FileSize = fileSize; }

  void readFileName(std::fstream &file, bool testHashes);

private:

  Folder *m_Folder;
  bool m_New;

  BSAHash m_NameHash;
  std::string m_Name;
  mutable BSAULong m_FileSize;
  BSAULong m_DataOffset;
  bool m_ToggleCompressed;

  std::string m_SourceFile;
  bool m_ToggleCompressedWrite;
  mutable BSAULong m_DataOffsetWrite;
};


extern bool ByOffset(const File::Ptr &LHS, const File::Ptr &RHS);


} // namespace BSA

#endif // BSAFILE_H
