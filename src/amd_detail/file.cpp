/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "file.h"
#include "file-descriptor.h"
#include "mountinfo.h"
#include "passkey.h"
#include "sys.h"

#include <fcntl.h>
#include <utility>
#include <string>
#include <sys/sysmacros.h>
#include <syslog.h>

using namespace std;

namespace hipFile {

UnregisteredFile::UnregisteredFile(int fd)
    : client_fd(FileDescriptor::make_unmanaged(fd)), buffered_fd{}, unbuffered_fd{},
      stx{Context<Sys>::get()->statx(fd, "", AT_EMPTY_PATH,
#if defined(STATX_DIOALIGN)
                                     STATX_TYPE | STATX_MODE | STATX_DIOALIGN
#else
                                     STATX_TYPE | STATX_MODE
#endif
                                     )},
      flags{Context<Sys>::get()->fcntl(fd, F_GETFL, 0)},
      mountinfo{Context<LibMountHelper>::get()->getMountInfo(makedev(stx.stx_dev_major, stx.stx_dev_minor))}
{
    std::string path = "/proc/self/fd/" + std::to_string(fd);

    if (flags & O_DIRECT) {
        unbuffered_fd = FileDescriptor::make_unmanaged(fd);
        buffered_fd   = FileDescriptor::make_managed(
            Context<Sys>::get()->open(path.c_str(), (flags | O_CLOEXEC) & ~O_DIRECT));
    }
    else {
        buffered_fd   = FileDescriptor::make_unmanaged(fd);
        unbuffered_fd = FileDescriptor::make_managed(
            Context<Sys>::get()->open(path.c_str(), (flags | O_CLOEXEC) | O_DIRECT));
    }
}

hipFileHandle_t
IFile::getHandle() const
{
    return reinterpret_cast<hipFileHandle_t>(const_cast<IFile *>(this));
}

File::File(UnregisteredFile &&uf, const PassKey<FileMap> &)
    : client_fd{std::move(uf.client_fd)}, buffered_fd{std::move(uf.buffered_fd)},
      unbuffered_fd{std::move(uf.unbuffered_fd)}, stx{uf.stx}, status_flags{uf.flags}, mountinfo{uf.mountinfo}
{
}

int
File::getClientFd() const
{
    return client_fd.get();
}

int
File::getBufferedFd() const
{
    return buffered_fd.get();
}

int
File::getUnbufferedFd() const
{
    return unbuffered_fd.get();
}

const struct statx &
File::getStatx() const noexcept
{
    return stx;
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
FileMap::getFile(hipFileHandle_t fh)
{
    auto itr = from_fh.find(fh);
    if (from_fh.end() == itr) {
        throw FileNotRegistered();
    }

    return itr->second;
}

hipFileHandle_t
FileMap::registerFile(UnregisteredFile &&uf)
{
    if (from_fd.end() != from_fd.find(uf.client_fd.get())) {
        throw FileAlreadyRegistered();
    }

    auto file                    = std::shared_ptr<IFile>(new File(std::move(uf), PassKey<FileMap>{}));
    from_fd[file->getClientFd()] = file;
    from_fh[file->getHandle()]   = file;

    return file->getHandle();
}

void
FileMap::deregisterFile(hipFileHandle_t fh)
{
    auto itr = from_fh.find(fh);

    if (from_fh.end() == itr) {
        throw FileNotRegistered();
    }

    if (2 < itr->second.use_count()) {
        throw FileOperationsOutstanding();
    }

    from_fd.erase(itr->second->getClientFd());
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
    // Complain in the logs if we're shutting down a FileMap
    // with files that are still in use.
    int count = 0;
    for (const auto &p : from_fh) {
        if (p.second.use_count() > 2) {
            count++;
        }
    }

    if (count > 0) {
        Context<Sys>::get()->syslog(LOG_CRIT, "FileMap state is being destructed with in-use files");
    }
}

}
