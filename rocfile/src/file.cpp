/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "file.h"
#include "state.h"

#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <vector>

using std::optional;
using std::shared_ptr;
using std::transform;
using std::vector;

namespace rocFile {

rocFileHandle_t
IFile::getHandle() const
{
    return reinterpret_cast<rocFileHandle_t>(const_cast<IFile *>(this));
}

File::File(int _fd, const struct stat &fstat, int _status_flags, optional<MountInfo> _mountinfo)
    : fd{_fd}, device{fstat.st_dev}, mode{fstat.st_mode}, status_flags{_status_flags}, mountinfo{_mountinfo}
{
}

int
File::getFd() const
{
    return fd;
}

dev_t
File::getDevice() const
{
    return device;
}

mode_t
File::getMode() const
{
    return mode;
}

int
File::getStatusFlags() const
{
    return status_flags;
}

optional<MountInfo>
File::getMountInfo() const
{
    return mountinfo;
}

shared_ptr<IFile>
FileMap::getFile(rocFileHandle_t fh)
{
    auto itr = from_fh.find(fh);
    if (from_fh.end() == itr) {
        throw FileNotRegistered();
    }

    return itr->second;
}

rocFileHandle_t
FileMap::registerFile(int fd, struct stat &fstat, int status_flags, optional<MountInfo> mountinfo)
{
    if (from_fd.end() != from_fd.find(fd)) {
        throw FileAlreadyRegistered();
    }

    auto file                  = std::shared_ptr<IFile>(new File(fd, fstat, status_flags, mountinfo));
    from_fd[fd]                = file;
    from_fh[file->getHandle()] = file;

    return file->getHandle();
}

void
FileMap::deregisterFile(rocFileHandle_t fh)
{
    auto itr = from_fh.find(fh);

    if (from_fh.end() == itr) {
        throw FileNotRegistered();
    }

    if (2 < itr->second.use_count()) {
        throw FileOperationsOutstanding();
    }

    from_fd.erase(itr->second->getFd());
    from_fh.erase(fh);
}

void
FileMap::clear()
{
    from_fd.clear();
    from_fh.clear();
}

FileMap::~FileMap()
{
    try {
        // Create a list of registered files without causing the use_count to increase
        vector<rocFileHandle_t> file_handles;
        file_handles.reserve(from_fh.size());
        transform(from_fh.begin(), from_fh.end(), std::back_inserter(file_handles),
                  [](const auto &pair) { return pair.first; });

        for (const auto &fh : file_handles) {
            deregisterFile(fh);
        }
    }
    catch (...) {
#ifndef NDEBUG
        // TODO: Consider logging here and/or changing this behaviour
        //       before launch. It's not a great idea to call exit()
        //       from inside libraries, but this will at least alert
        //       us to badness while we develop.
        std::exit(EXIT_FAILURE);
#endif
    }
}

}
