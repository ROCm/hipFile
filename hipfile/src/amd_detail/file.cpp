/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "file.h"
#include "mountinfo.h"
#include "sys.h"

#include <algorithm>
#include <cstdlib>
#include <fcntl.h>
#include <iterator>
#include <utility>
#include <sys/sysmacros.h>
#include <vector>

using std::optional;
using std::shared_ptr;
using std::transform;
using std::vector;

namespace hipFile {

UnregisteredFile::UnregisteredFile(int fd)
    : m_fd(fd), m_stx{Context<Sys>::get()->statx(fd, "", AT_EMPTY_PATH,
#if defined(STATX_DIOALIGN)
                                                 STATX_TYPE | STATX_MODE | STATX_DIOALIGN
#else
                                                 STATX_TYPE | STATX_MODE
#endif
                                                 )},
      m_flags{Context<Sys>::get()->fcntl(fd, F_GETFL, 0)},
      m_mountinfo{
          Context<LibMountHelper>::get()->getMountInfo(makedev(m_stx.stx_dev_major, m_stx.stx_dev_minor))}
{
}

int
UnregisteredFile::getFd() const noexcept
{
    return m_fd;
}

struct statx
UnregisteredFile::getStatx() const noexcept
{
    return m_stx;
}

int
UnregisteredFile::getFlags() const noexcept
{
    return m_flags;
}

optional<MountInfo>
UnregisteredFile::getMountInfo() const noexcept
{
    return m_mountinfo;
}

hipFileHandle_t
IFile::getHandle() const
{
    return reinterpret_cast<hipFileHandle_t>(const_cast<IFile *>(this));
}

File::File(const UnregisteredFile &uf)
    : fd{uf.getFd()}, stx{uf.getStatx()}, status_flags{uf.getFlags()}, mountinfo{uf.getMountInfo()}
{
}

int
File::getFd() const
{
    return fd;
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
FileMap::registerFile(const UnregisteredFile &uf)
{
    if (from_fd.end() != from_fd.find(uf.getFd())) {
        throw FileAlreadyRegistered();
    }

    auto file                  = std::shared_ptr<IFile>(new File(uf));
    from_fd[file->getFd()]     = file;
    from_fh[file->getHandle()] = file;

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

    from_fd.erase(itr->second->getFd());
    from_fh.erase(fh);
}

void
FileMap::clear()
{
    from_fd.clear();
    from_fh.clear();
}

}
