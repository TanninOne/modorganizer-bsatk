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


#ifndef BSA_ARCHIVE_H
#define BSA_ARCHIVE_H


#include "errorcodes.h"
#include "bsatype.h"
#include "bsatypes.h"
#include "bsafolder.h"
#include "semaphore.h"
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>


namespace BSA {

template<typename T>
struct array_deleter
{
  void operator ()(const T *p)
  { 
    delete[] p; 
  }
};
  
class File;

/**
 * @brief top level structure to represent a bsa file
 */
class Archive {

public:

  typedef std::pair<std::shared_ptr<unsigned char>, BSAULong> DataBuffer;

private:

  static const unsigned int FLAG_HASDIRNAMES       = 0x00000001;
  static const unsigned int FLAG_HASFILENAMES      = 0x00000002;
  static const unsigned int FLAG_DEFAULTCOMPRESSED = 0x00000004;
  static const unsigned int FLAG_NAMEPREFIXED      = 0x00000100; // if set, the full file name is prefixed before a data block

public:
  /**
   * constructor
   */
  Archive();
  ~Archive();
  /**
   * read the archive from file
   * @param fileName name of the file to read from
   * @param testHashes if true, the hashes of file names will be checked to ensure the file is valid.
   *                    This can be skipped for performance reasons
   * @return ERROR_NONE on success or an error code
   */
  EErrorCode read(const char *fileName, bool testHashes);
  /**
   * read the archive from file
   * @param fileName name of the file to read from
   * @param testHashes if true, the hashes of file names will be checked to ensure the file is valid.
   *                    This can be skipped for performance reasons
   * @return ERROR_NONE on success or an error code
   */
#ifdef WIN32
  EErrorCode read(const wchar_t *fileName, bool testHashes);
#endif
  /**
   * write the archive to disc
   * @param fileName name of the file to write to
   * @return ERROR_NONE on success or an error code
   */
  EErrorCode write(const char *fileName);
  /**
   * @brief close the archive
   */
  void close();
  /**
   * change the archive type
   * @param type new archive type
   */
  void setType(EType type) { m_Type = type; }
  /**
   * @return type of the archive (supported game)
   */
  EType getType() const { return m_Type; }
  /**
   * retrieve top-level folder
   * @return descriptor of the root folder
   */
  Folder::Ptr getRoot() { return m_RootFolder; }
  /**
   * extract a file from the archive
   * @param file descriptor of the file to extract
   * @param outputDirectory name of the directory to extract to.
   *                        may be absolute or relative
   * @return ERROR_NONE on success or an error code
   */
  EErrorCode extract(File::Ptr file, const char *outputDirectory) const;

  /**
   * extract all files. this is potentially faster than iterating over all files and
   * extracting each
   * @param outputDirectory name of the directory to extract to.
   *                        may be absolute or relative
   * @param progress callback function called on progress
   * @param overwrite if true (default) files are overwritten if they exist
   * @return ERROR_NONE on success or an error code
   */
  EErrorCode extractAll(const char *outputDirectory,
                        const std::function<bool (int value, std::string fileName)> &progress,
                        bool overwrite = true);

  /**
   * @param file the file to check
   * @return true if the file is compressed, false otherwise
   */
  bool compressed(const File::Ptr &file);

  /**
   * @return true if the file stream is still open, false otherwise
   */
  bool isOpen();

  /**
   * create a new file to be placed in this archive. The new file is NOT
   * added to a folder, use BSA::Folder::addFile for that
   * @param name name of the file to be used inside the archive
   * @param sourceName filename path to the file to add
   * @param compressed true if the file should be compressed (not supported yet!)
   * @return pointer to the new file
   */
  File::Ptr createFile(const std::string &name, const std::string &sourceName,
                       bool compressed);

private:

  struct Header {
    char fileIdentifier[4];
    EType type;
    BSAULong offset;
    BSAULong archiveFlags;
    BSAULong folderCount;
    BSAULong fileCount;
    BSAULong folderNameLength;
    BSAULong fileNameLength;
    BSAULong fileFlags;
  };

  struct FileInfo {
    File::Ptr file;
    DataBuffer data;
  };


private:

  EErrorCode read(bool testHashes);

  static Header readHeader(std::fstream &infile);

  static EType typeFromID(BSAULong typeID);

  static std::shared_ptr<unsigned char> decompress(unsigned char *inBuffer, BSAULong inSize, EErrorCode &result, BSAULong &outSize);


  BSAULong typeToID(EType type);

  Folder readFolderRecord(std::fstream &file);

//  EErrorCode extractDirect(const File &fileInfo, std::ofstream &outFile);
//  EErrorCode extractCompressed(const File &fileInfo, std::ofstream &outFile);

  bool defaultCompressed() const { return (m_ArchiveFlags & FLAG_DEFAULTCOMPRESSED) != 0; }
  // starting with FO3 the bsa may prefix the file name to the file blob if archive flag 0x100 is set
  bool namePrefixed() const { return (m_Type != TYPE_OBLIVION) && ((m_ArchiveFlags & FLAG_NAMEPREFIXED) != 0); }

  BSAULong countFiles() const;

  std::vector<std::string> collectFolderNames() const;
  std::vector<std::string> collectFileNames() const;

  BSAULong countCharacters(const std::vector<std::string> &list) const;
  BSAULong determineFileFlags(const std::vector<std::string> &fileList) const;

  void writeHeader(std::fstream &outfile, BSAULong fileFlags, BSAULong numFolders,
                   BSAULong folderNamesLength, BSAULong fileNamesLength);

  EErrorCode extractDirect(File::Ptr file, std::ofstream &outFile) const;
  EErrorCode extractCompressed(File::Ptr file, std::ofstream &outFile) const;


  void createFolders(const std::string &targetDirectory, Folder::Ptr folder);

  void readFiles(std::queue<FileInfo> &queue,
                 std::mutex &mutex,
                 Semaphore &bufferCount,
                 Semaphore &queueFree,
                 std::vector<File::Ptr>::iterator begin,
                 std::vector<File::Ptr>::iterator end,
                 std::condition_variable &cv);

  void extractFiles(const std::string &targetDirectory,
                    std::queue<FileInfo> &queue, std::mutex &mutex,
                    Semaphore &bufferCount,
                    Semaphore &queueFree,
                    int totalFiles, bool overwrite,
                    std::condition_variable &cv, int &filesDone);
private:

  mutable std::fstream m_File;

  Folder::Ptr m_RootFolder;

  BSAULong m_ArchiveFlags;
  EType m_Type;

  std::mutex m_ReaderMutex;
  std::mutex m_ExtractMutex;

};

} // namespace BSA

#endif // BSA_ARCHIVE_H

