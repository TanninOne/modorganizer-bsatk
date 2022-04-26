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

#include "bsaarchive.h"
#include "bsaexception.h"
#include "bsafile.h"
#include "bsafolder.h"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <queue>
#include <memory>
#include <mutex>
#include <zlib.h>
#include <sys/stat.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using std::fstream;
using namespace std::chrono_literals;


namespace BSA {

Archive::Archive()
  : m_RootFolder(new Folder),
    m_ArchiveFlags(FLAG_HASDIRNAMES | FLAG_HASFILENAMES),
    m_Type(TYPE_SKYRIM)
{
}


Archive::~Archive()
{
  if (m_File.is_open()) {
    m_File.close();
  }
}


EType Archive::typeFromID(BSAULong typeID)
{
  switch (typeID) {
    case 0x67: return TYPE_OBLIVION;
    case 0x68: return TYPE_FALLOUT3;
    case 0x69: return TYPE_SKYRIMSE;
    default: throw data_invalid_exception(makeString("invalid type %d", typeID));
  }
}


BSAULong Archive::typeToID(EType type)
{
  switch (type) {
    case TYPE_OBLIVION: return 0x67;
    case TYPE_FALLOUT3: return 0x68;
    case TYPE_SKYRIMSE: return 0x69;
    default: throw data_invalid_exception(makeString("invalid type %d", type));
  }
}


Archive::Header Archive::readHeader(std::fstream &infile)
{
  Header result;

  char fileID[4];
  infile.read(fileID, 4);
  if (memcmp(fileID, "BSA\0", 4) != 0) {
    throw data_invalid_exception(makeString("not a bsa file"));
  }

  result.type  = typeFromID(readType<BSAULong>(infile));
  result.offset           = readType<BSAULong>(infile);
  result.archiveFlags     = readType<BSAULong>(infile);
  result.folderCount      = readType<BSAULong>(infile);
  result.fileCount        = readType<BSAULong>(infile);
  result.folderNameLength = readType<BSAULong>(infile);
  result.fileNameLength   = readType<BSAULong>(infile);
  result.fileFlags        = readType<BSAULong>(infile);

  return result;
}

EErrorCode Archive::read(const char *fileName, bool testHashes)
{
  m_File.open(fileName, fstream::in | fstream::binary);
  return read(testHashes);
}

#ifdef WIN32
EErrorCode Archive::read(const wchar_t *fileName, bool testHashes)
{
  m_File.open(fileName, fstream::in | fstream::binary);
  return read(testHashes);
}
#endif

EErrorCode Archive::read(bool testHashes) {
  if (!m_File.is_open()) {
    return ERROR_FILENOTFOUND;
  }
  m_File.exceptions(std::ios_base::badbit);
  try {
    Header header;
    header = readHeader(m_File);

    m_Type = header.type;
    m_ArchiveFlags = header.archiveFlags;

    // flat list of folders as they were stored in the archive
    std::vector<Folder::Ptr> folders;

    for (unsigned long i = 0; i < header.folderCount; ++i) {
      folders.push_back(m_RootFolder->addFolder(m_File, m_Type, header.fileNameLength, header.offset));
    }

    m_File.seekg(header.offset);

    bool hashesValid = true;
    for (std::vector<Folder::Ptr>::iterator iter = folders.begin();
         iter != folders.end(); ++iter) {
      if (!(*iter)->resolveFileNames(m_File, testHashes)) {
        hashesValid = false;
      }
    }
    return hashesValid ? ERROR_NONE : ERROR_INVALIDHASHES;
  } catch (std::ios_base::failure&) {
    return ERROR_INVALIDDATA;
  }
}


void Archive::close()
{
  m_File.close();
}


BSAULong Archive::countFiles() const
{
  return m_RootFolder->countFiles();
}


std::vector<std::string> Archive::collectFolderNames() const
{
  std::vector<std::string> result;
  m_RootFolder->collectFolderNames(result);
  return result;
}


std::vector<std::string> Archive::collectFileNames() const
{
  std::vector<std::string> result;
  m_RootFolder->collectFileNames(result);
  return result;
}


BSAULong Archive::countCharacters(const std::vector<std::string> &list) const
{
  size_t sum = 0;
  for (std::vector<std::string>::const_iterator iter = list.begin();
       iter != list.end(); ++iter) {
    sum += iter->length() + 1;
  }
  return static_cast<BSAULong>(sum);
}

#ifndef WIN32
#define _stricmp strcasecmp
#endif // WIN32

static bool endsWith(const std::string &fileName, const char *extension)
{
  size_t endLength = strlen(extension);
  if (fileName.length() < endLength) {
    return false;
  }
  return _stricmp(&fileName[fileName.length() - endLength], extension) == 0;
}


BSAULong Archive::determineFileFlags(const std::vector<std::string> &fileList) const
{
  BSAULong result = 0;

  bool hasNIF = false;
  bool hasDDS = false;
  bool hasXML = false;
  bool hasWAV = false;
  bool hasMP3 = false;
  bool hasTXT = false;
  bool hasSPT = false;
  bool hasTEX = false;
  bool hasCTL = false;

  for (std::vector<std::string>::const_iterator iter = fileList.begin();
       iter != fileList.end(); ++iter) {
    if (!hasNIF && endsWith(*iter, ".nif")) {
      hasNIF = true;
      result |= 1 << 0;
    } else if (!hasDDS && endsWith(*iter, ".dds")) {
      hasDDS = true;
      result |= 1 << 1;
    } else if (!hasXML && endsWith(*iter, ".xml")) {
      hasXML = true;
      result |= 1 << 2;
    } else if (!hasWAV && endsWith(*iter, ".wav")) {
      hasWAV = true;
      result |= 1 << 3;
    } else if (!hasMP3 && endsWith(*iter, ".mp3")) {
      hasMP3 = true;
      result |= 1 << 4;
    } else if (!hasTXT && endsWith(*iter, ".txt")) {
      hasTXT = true;
      result |= 1 << 5;
    } else if (!hasSPT && endsWith(*iter, ".spt")) {
      hasSPT = true;
      result |= 1 << 6;
    } else if (!hasTEX && endsWith(*iter, ".tex")) {
      hasTEX = true;
      result |= 1 << 7;
    } else if (!hasCTL && endsWith(*iter, ".ctl")) {
      hasCTL = true;
      result |= 1 << 8;
    }
  }
  return result;
}


void Archive::writeHeader(std::fstream &outfile, BSAULong fileFlags, BSAULong numFolders,
                          BSAULong folderNamesLength, BSAULong fileNamesLength)
{
  outfile.write("BSA\0", 4);
  writeType<BSAULong>(outfile, typeToID(m_Type));
  writeType<BSAULong>(outfile, 0x24); // header size is static
  writeType<BSAULong>(outfile, m_ArchiveFlags);
  writeType<BSAULong>(outfile, numFolders);
  writeType<BSAULong>(outfile, countFiles());
  writeType<BSAULong>(outfile, folderNamesLength);
  writeType<BSAULong>(outfile, fileNamesLength);
  writeType<BSAULong>(outfile, fileFlags);
}


EErrorCode Archive::write(const char *fileName)
{
  std::fstream outfile;
  outfile.open(fileName, fstream::out | fstream::binary);
  if (!outfile.is_open()) {
    return ERROR_ACCESSFAILED;
  }
  outfile.exceptions(std::ios_base::badbit);

  std::vector<Folder::Ptr> folders;
  m_RootFolder->collectFolders(folders);

  std::vector<std::string> folderNames;
  std::vector<std::string> fileNames;
  BSAULong folderNamesLength = 0;
  BSAULong fileNamesLength = 0;
  for (std::vector<Folder::Ptr>::const_iterator folderIter = folders.begin();
       folderIter != folders.end(); ++folderIter) {
    std::string fullPath = (*folderIter)->getFullPath();
    folderNames.push_back(fullPath);
    folderNamesLength += static_cast<BSAULong>(fullPath.length());
    for (std::vector<File::Ptr>::const_iterator fileIter = (*folderIter)->m_Files.begin();
         fileIter != (*folderIter)->m_Files.end(); ++fileIter) {
      fileNames.push_back((*fileIter)->m_Name);
      fileNamesLength += static_cast<BSAULong>((*fileIter)->m_Name.length());
    }
  }

  try {
    writeHeader(outfile, determineFileFlags(fileNames),
                static_cast<BSAULong>(folderNames.size()), folderNamesLength,
                fileNamesLength);
#pragma message("folders (and files?) need to be sorted by hash!")
    // dummy-pass: before we can store the actual folder data

    // prepare folder and file headers
#pragma message("it's unnecessary to write actual data, placeholders are sufficient")
    for (std::vector<Folder::Ptr>::const_iterator folderIter = folders.begin();
         folderIter != folders.end(); ++folderIter) {
      (*folderIter)->writeHeader(outfile);
    }

    for (std::vector<Folder::Ptr>::const_iterator folderIter = folders.begin();
         folderIter != folders.end(); ++folderIter) {
      (*folderIter)->writeData(outfile, fileNamesLength);
    }

    // write file names
    for (std::vector<std::string>::const_iterator folderIter = fileNames.begin();
         folderIter != fileNames.end(); ++folderIter) {
      writeZString(outfile, *folderIter);
    }

    // write file data
    for (std::vector<Folder::Ptr>::iterator folderIter = folders.begin();
         folderIter != folders.end(); ++folderIter) {
      (*folderIter)->writeFileData(m_File, outfile);
    }

    outfile.seekp(0x24, fstream::beg);

    // re-write folder and file structure, this time with the correct
    // offsets
    for (std::vector<Folder::Ptr>::const_iterator folderIter = folders.begin();
         folderIter != folders.end(); ++folderIter) {
      (*folderIter)->writeHeader(outfile);
    }

    for (std::vector<Folder::Ptr>::const_iterator folderIter = folders.begin();
         folderIter != folders.end(); ++folderIter) {
      (*folderIter)->writeData(outfile, fileNamesLength);
    }

    outfile.close();
    return ERROR_NONE;
  } catch (std::ios_base::failure&) {
    outfile.close();
    return ERROR_INVALIDDATA;
  }
}


static const unsigned long CHUNK_SIZE = 128 * 1024;


EErrorCode Archive::extractDirect(File::Ptr file, std::ofstream &outFile) const
{
  EErrorCode result = ERROR_NONE;

  m_File.seekg(file->m_DataOffset, fstream::beg);
  if (namePrefixed()) {
    readBString(m_File);
  }

  std::unique_ptr<char[]> inBuffer(new char[CHUNK_SIZE]);

  try {
    unsigned long sizeLeft = file->m_FileSize;

    while (sizeLeft > 0) {
      int chunkSize = (std::min)(sizeLeft, CHUNK_SIZE);
      m_File.read(inBuffer.get(), chunkSize);
      outFile.write(inBuffer.get(), chunkSize);
      sizeLeft -= chunkSize;
    }
  } catch (const std::exception&) {
    result = ERROR_INVALIDDATA;
  }
  return result;
}


std::shared_ptr<unsigned char> Archive::decompress(unsigned char *inBuffer, BSAULong inSize,
                                                   EErrorCode &result, BSAULong &outSize)
{
  memcpy(&outSize, inBuffer, sizeof(BSAULong));
  inBuffer += sizeof(BSAULong);
  inSize -= sizeof(BSAULong);

  if ((inSize == 0) || (outSize == 0)) {
    return std::shared_ptr<unsigned char>();
  }

  std::shared_ptr<unsigned char> outBuffer(new unsigned char[outSize], array_deleter<unsigned char>());

  z_stream stream;
  try {
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    int zlibRet = inflateInit(&stream);
    if (zlibRet != Z_OK) {
      result = ERROR_ZLIBINITFAILED;
      return std::shared_ptr<unsigned char>();
    }

    stream.avail_in = inSize;

    stream.next_in = static_cast<Bytef*>(inBuffer);

    do {
      stream.avail_out = outSize;
      stream.next_out = reinterpret_cast<Bytef*>(outBuffer.get());
      zlibRet = inflate(&stream, Z_NO_FLUSH);
      if ((zlibRet != Z_OK) && (zlibRet != Z_STREAM_END) && (zlibRet != Z_BUF_ERROR)) {
#pragma message("pass result code to caller")
        throw std::runtime_error("invalid data");
      }
    } while (stream.avail_out == 0);
    inflateEnd(&stream);
    return outBuffer;
  } catch (const std::exception&) {
    result = ERROR_INVALIDDATA;
    inflateEnd(&stream);
    return std::shared_ptr<unsigned char>();
  }
}


EErrorCode Archive::extractCompressed(File::Ptr file, std::ofstream &outFile) const
{
  EErrorCode result = ERROR_NONE;

  if (file->m_FileSize == 0) {
    // don't try to read empty file
    return result;
  }

  m_File.clear();
  m_File.seekg(static_cast<std::ifstream::pos_type>(file->m_DataOffset), std::ios::beg);
  if (namePrefixed()) {
    readBString(m_File);
  }

  BSAULong inSize = file->m_FileSize + sizeof(BSAULong); // file data has original size prepended
  std::unique_ptr<unsigned char[]> inBuffer(new unsigned char[inSize]);
  m_File.read(reinterpret_cast<char*>(inBuffer.get()), inSize);
  BSAULong length = 0L;
  std::shared_ptr<unsigned char> buffer = decompress(inBuffer.get(), inSize, result, length);
  if (result == ERROR_NONE) {
    outFile.write(reinterpret_cast<char*>(buffer.get()), length);
  }

  return result;
}


EErrorCode Archive::extract(File::Ptr file, const char *outputDirectory) const
{
  std::string fileName = makeString("%s/%s", outputDirectory, file->getName().c_str());
  std::ofstream outputFile(fileName.c_str(), fstream::out | fstream::binary | fstream::trunc);
  if (!outputFile.is_open()) {
    return ERROR_ACCESSFAILED;
  }

  EErrorCode result = ERROR_NONE;
  if ((defaultCompressed() && !file->compressToggled()) ||
      (!defaultCompressed() && file->compressToggled())) {
    result = extractCompressed(file, outputFile);
  } else {
    result = extractDirect(file, outputFile);
  }
  outputFile.close();
  return result;
}


void Archive::readFiles(std::queue<FileInfo> &queue, std::mutex &mutex,
                        Semaphore &bufferCount,
                        Semaphore &queueFree,
                        std::vector<File::Ptr>::iterator begin, std::vector<File::Ptr>::iterator end,
                        std::condition_variable &cv)
{
  std::unique_lock<std::mutex> lock(m_ReaderMutex);
  for (; begin != end && (cv.wait_for(lock, 1us) == std::cv_status::timeout); ++begin) {
    queueFree.wait();
    FileInfo fileInfo;
    fileInfo.file = *begin;
    size_t size = static_cast<size_t>(fileInfo.file->m_FileSize);

    m_File.seekg(fileInfo.file->m_DataOffset);
    if (namePrefixed()) {
      std::string fullName = readBString(m_File);
      if (size <= fullName.length()) {
#pragma message("report error!")
        continue;
      }
      size -= fullName.length() + 1;
    }
    fileInfo.data = std::make_pair(
        std::shared_ptr<unsigned char>(new unsigned char[size]),
        static_cast<BSAULong>(size));
    m_File.read(reinterpret_cast<char*>(fileInfo.data.first.get()), size);

    {
      std::unique_lock<std::mutex> lock(mutex);
      queue.push(fileInfo);
    }
    bufferCount.post();
  }
  cv.notify_all();
}


inline bool fileExists(const std::string &name) {
  struct stat buffer;
  return stat(name.c_str(), &buffer) != -1;
}


void Archive::extractFiles(const std::string &targetDirectory,
                           std::queue<FileInfo> &queue, std::mutex &mutex,
                           Semaphore &bufferCount,
                           Semaphore &queueFree,
                           int totalFiles,
                           bool overwrite,
                           std::condition_variable &cv,
                           int &filesDone)
{
  std::unique_lock<std::mutex> lock(m_ExtractMutex);
  for (int i = 0; i < totalFiles; ++i) {
    bufferCount.wait();
    if (cv.wait_for(lock, 1us) == std::cv_status::no_timeout) {
      break;
    }

    FileInfo fileInfo;

    {
      std::unique_lock<std::mutex> lock(mutex);
      fileInfo = queue.front();
      ++filesDone;
      queue.pop();
    }
    queueFree.post();

    DataBuffer dataBuffer = fileInfo.data;

    std::string fileName = makeString("%s\\%s", targetDirectory.c_str(), fileInfo.file->getFilePath().c_str());
    if (!overwrite && fileExists(fileName)) {
      continue;
    }

    std::ofstream outputFile(fileName.c_str(), fstream::out | fstream::binary | fstream::trunc);

    if (compressed(fileInfo.file)) {
      if (!outputFile.is_open()) {
#pragma message("report error!")
        continue;
        //return ERROR_ACCESSFAILED;
      }

      EErrorCode result = ERROR_NONE;
      try {
        BSAULong length = 0UL;
        std::shared_ptr<unsigned char> buffer = decompress(dataBuffer.first.get(), dataBuffer.second + sizeof(BSAULong),
          result, length);
        if (buffer.get() != nullptr) {
          outputFile.write(reinterpret_cast<char*>(buffer.get()), length);
        }
      } catch (const std::exception &) {
#pragma message("report error!")
        continue;
      }
    } else {
      outputFile.write(reinterpret_cast<char*>(dataBuffer.first.get()), dataBuffer.second);
    }
  }
  cv.notify_all();
}


void Archive::createFolders(const std::string &targetDirectory, Folder::Ptr folder)
{
  for (std::vector<Folder::Ptr>::iterator iter = folder->m_SubFolders.begin();
       iter != folder->m_SubFolders.end(); ++iter) {
    std::string subDirName = targetDirectory + "\\" + (*iter)->getName();
    ::CreateDirectoryA(subDirName.c_str(), nullptr);
    createFolders(subDirName, *iter);
  }
}


EErrorCode Archive::extractAll(const char *outputDirectory,
                               const std::function<bool (int value, std::string fileName)> &progress,
                               bool overwrite)
{
#pragma message("report errors")
  createFolders(outputDirectory, m_RootFolder);

  std::vector<File::Ptr> fileList;
  m_RootFolder->collectFiles(fileList);
  std::sort(fileList.begin(), fileList.end(), ByOffset);
  m_File.seekg((*(fileList.begin()))->m_DataOffset);

  std::queue<FileInfo> buffers;
  std::mutex queueMutex;
  int filesDone = 0;
  Semaphore bufferCount(0);
  Semaphore queueFree(100);

  std::condition_variable readerCV;

  std::condition_variable extractCV;

  std::thread readerThread([&]() {
    this->readFiles(buffers, queueMutex, bufferCount, queueFree, fileList.begin(), fileList.end(), readerCV);
  });
    
  std::thread extractThread([&]() {
    this->extractFiles(outputDirectory, buffers, queueMutex, bufferCount, queueFree, fileList.size(), overwrite, extractCV, filesDone);
  });

  bool readerDone  = false;
  bool extractDone = false;
  bool canceled = false;


  while (!readerDone || !extractDone) {
    if (!readerDone) {
      std::unique_lock<std::mutex> lock(m_ReaderMutex);
      readerDone = readerCV.wait_for(lock, 100ms) == std::cv_status::no_timeout;
    }
    if (readerDone) {
      readerThread.join();

      std::unique_lock<std::mutex> lock(m_ExtractMutex);
      extractDone = extractCV.wait_for(lock, 100ms) == std::cv_status::no_timeout;

      // don't cancel extractor before reader is done or else reader may be stuck trying to write to a queue
      if (canceled) {
        // ensure the extract thread wakes up.
        extractCV.notify_all();
        bufferCount.post();
      }
    }
    size_t index
        = (std::min)(static_cast<size_t>(filesDone), fileList.size() - 1);
    if (!progress((filesDone * 100) / static_cast<int>(fileList.size()),
                  fileList[index]->getName())
        && !canceled) {
      readerCV.notify_all();
      canceled = true; // don't interrupt repeatedly
    }
  }

  return ERROR_NONE;
}


bool Archive::compressed(const File::Ptr &file)
{
  return ((defaultCompressed() && !file->compressToggled()) ||
          (!defaultCompressed() && file->compressToggled()));
}

bool Archive::isOpen()
{
  return m_File.is_open();
}

File::Ptr Archive::createFile(const std::string &name, const std::string &sourceName,
                              bool compressed)
{
  return File::Ptr(new File(name, sourceName, nullptr,
                            defaultCompressed() != compressed));
}


} // namespace BSA
