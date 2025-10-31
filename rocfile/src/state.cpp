/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend/fallback.h"
#include "batch/batch.h"
#include "buffer.h"
#include "mountinfo.h"
#include "state.h"
#include "sys.h"

#include "hipfile-warnings.h"

#include <fcntl.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

using rocFile::buffer::IBuffer;
using rocFile::context::Context;
using rocFile::file::IFile;
using rocFile::stream::IStream;

using std::shared_lock;
using std::shared_mutex;
using std::shared_ptr;
using std::unique_lock;

namespace rocFile {

DriverState::DriverState()
    : ref_count{0}, backends{std::shared_ptr<backend::Backend>(new backend::Fallback{})}
{
    this->file_map   = std::make_unique<file::FileMap>();
    this->batch_map  = std::make_unique<batch::BatchContextMap>();
    this->buffer_map = std::make_unique<buffer::BufferMap>();
    this->stream_map = std::make_unique<stream::StreamMap>();
}

DriverState::~DriverState()
{
}

//
// Batch interface
//

rocFileBatchHandle_t
DriverState::createBatchContext(unsigned capacity)
{
    return batch_map->createContext(capacity);
}

void
DriverState::destroyBatchContext(rocFileBatchHandle_t handle)
{
    batch_map->destroyContext(handle);
}

std::shared_ptr<batch::IBatchContext>
DriverState::getBatchContext(rocFileBatchHandle_t handle)
{
    return batch_map->get(handle);
}

//
// Buffer interface
//

void
DriverState::registerBuffer(const void *buf, size_t length, int flags)
{
    unique_lock<shared_mutex> ulock{state_mutex};

    // For NVIDIA cuFile compatibility, implicitly "initialize"
    // the driver if it's currently uninitialized
    if (ref_count == 0) {
        ref_count++;
    }

    buffer_map->registerBuffer(buf, length, flags);
}

void
DriverState::deregisterBuffer(const void *buf)
{
    unique_lock<shared_mutex> ulock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    buffer_map->deregisterBuffer(buf);
}

shared_ptr<IBuffer>
DriverState::getBuffer(const void *buf)
{
    unique_lock<shared_mutex> ulock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    return buffer_map->getBuffer(buf);
}

shared_ptr<IBuffer>
DriverState::getBuffer(const void *buf, size_t length, int flags)
{
    // NOTE: This mutex only protects the map, so we'll
    //       also need to protect the data
    shared_lock<shared_mutex> slock{state_mutex};

    return buffer_map->getBuffer(buf, length, flags);
}

//
// File interface
//

rocFileHandle_t
DriverState::registerFile(int fd)
{
    // Get file information outside of the state mutex to avoid potentially
    // stalling other IO threads
    auto fstat{Context<Sys>::get()->fstat(fd)};
    auto status_flags{Context<Sys>::get()->fcntl(fd, F_GETFL, 0)};
    auto mountinfo{Context<LibMountHelper>::get()->getMountInfo(fstat.st_dev)};

    unique_lock<shared_mutex> ulock{state_mutex};

    // For NVIDIA cuFile compatibility, implicitly "initialize"
    // the driver if it's currently uninitialized
    if (ref_count == 0) {
        ref_count++;
    }

    return file_map->registerFile(fd, fstat, status_flags, mountinfo);
}

void
DriverState::deregisterFile(rocFileHandle_t fh)
{
    unique_lock<shared_mutex> ulock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    file_map->deregisterFile(fh);
}

shared_ptr<IFile>
DriverState::getFile(rocFileHandle_t fh)
{
    // NOTE: This mutex only protects the map, so we'll
    //       also need to protect the data
    shared_lock<shared_mutex> slock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    return file_map->getFile(fh);
}

//
// Stream interface
//

void
DriverState::registerStream(const hipStream_t hip_stream, uint32_t flags)
{
    unique_lock<shared_mutex> ulock{state_mutex};
    stream_map->registerStream(hip_stream, flags);
}

void
DriverState::deregisterStream(const hipStream_t hip_stream)
{
    unique_lock<shared_mutex> ulock{state_mutex};
    stream_map->deregisterStream(hip_stream);
}

shared_ptr<IStream>
DriverState::getStream(hipStream_t hip_stream)
{
    shared_lock<shared_mutex> slock{state_mutex};
    return stream_map->getStream(hip_stream);
}

//
// Buffer and file calls
//
// These are for reducing the number of lock calls and are implemented
// as needed for the rocFile code
//

file_buffer_pair
DriverState::getFileAndBuffer(rocFileHandle_t fh, const void *buf, size_t length, int flags)
{
    // NOTE: This mutex only protects the map, so we'll
    //       also need to protect the data
    shared_lock<shared_mutex> slock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    return {file_map->getFile(fh), buffer_map->getBuffer(buf, length, flags)};
}

//
// Buffer and file and stream calls
//

file_buffer_stream_tuple
DriverState::getFileBufferAndStream(rocFileHandle_t fh, const void *buf, size_t length, int flags,
                                    hipStream_t hipStream)
{
    shared_lock<shared_mutex> slock{state_mutex};

    if (ref_count == 0) {
        throw DriverNotInitialized();
    }

    return {file_map->getFile(fh), buffer_map->getBuffer(buf, length, flags),
            stream_map->getStream(hipStream)};
}

//
// Reference counts
//

void
DriverState::incrRefCount()
{
    unique_lock<shared_mutex> ulock{state_mutex};

    ref_count++;
}

void
DriverState::decrRefCount()
{
    unique_lock<shared_mutex> ulock{state_mutex};

    // Decrement the reference count
    if (ref_count > 0) {
        ref_count--;
    }

    // Clear the maps if the reference count just hit zero
    if (ref_count == 0) {
        buffer_map->clear();
        file_map->clear();
    }

    // Negative reference count, should never get here
    if (ref_count < 0) {
        throw std::runtime_error("negative state reference count");
    }
}

int64_t
DriverState::getRefCount() const
{
    shared_lock<shared_mutex> slock{state_mutex};

    return ref_count;
}

void
DriverState::ensureInitialized()
{
    unique_lock<shared_mutex> ulock{state_mutex};

    if (ref_count == 0) {
        ref_count++;
    }
}

std::vector<std::shared_ptr<backend::Backend>>
DriverState::getBackends() const
{
    return backends;
}
}
