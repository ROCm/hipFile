/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"
#include "batch/batch.h"
#include "buffer.h"
#include "file.h"
#include "rocfile.h"
#include "stream.h"

#include <cstddef>
#include <cstdint>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

namespace rocFile {
template <typename T> struct Context;
}

namespace rocFile {

/// @brief The driver is not initialized
struct DriverNotInitialized : public std::runtime_error {
    DriverNotInitialized() : std::runtime_error("Driver is not initialized")
    {
    }
};

struct file_buffer_pair {
    std::shared_ptr<IFile>   file;
    std::shared_ptr<IBuffer> buffer;
};

struct file_buffer_stream_tuple {
    std::shared_ptr<IFile>   file;
    std::shared_ptr<IBuffer> buffer;
    std::shared_ptr<IStream> stream;
};

// rocFile "state"
//
// Important for:
//
// * Ensuring the BufferMap is cleared before the FileMap
// * Mimicking NVIDIA cuFile "buffer initialized" behavior
//
// The initialization behavior was determined empirically from observing
// cuFile behaviour and may not be important to maintain.
//
// Essentially:
// * A reference count of 0 means the driver is "uninitialized" and
//   will return "uninitialized driver" errors
// * A positive reference count means the driver is initialized for
//   normal use
// * When the driver transitions to a reference count of zero, the
//   BufferMap and FileMap will be cleared
//
class DriverState {
public:
    //
    // Batch interface
    //

    /// @brief Create a new batch context
    /// @param [in] capacity Maximum number of outstanding operations that this context can manage
    /// @return An opaque handle used to reference this new batch context
    virtual rocFileBatchHandle_t createBatchContext(unsigned capacity);

    /// @brief Destroy a batch context and release all associated resources
    /// @param [in] handle The handle for the batch context to destroy
    virtual void destroyBatchContext(rocFileBatchHandle_t handle);

    /// @brief Get a batch context
    /// @param [in] handle The opaque handle associated with a batch context
    /// @return A batch context
    virtual std::shared_ptr<IBatchContext> getBatchContext(rocFileBatchHandle_t handle);

    //
    // Buffer interface
    //

    /// @brief Creates and registers a buffer
    /// @param [in] buf Buffer pointer
    /// @param [in] length Buffer length
    /// @param [in] flags Buffer flags (unused)
    virtual void registerBuffer(const void *buf, size_t length, int flags);

    /// @brief Deregisters and destroys a buffer
    /// @param [in] buf Buffer pointer
    virtual void deregisterBuffer(const void *buf);

    /// @brief Look up a registered buffer using the buffer pointer
    /// @param [in] buf Buffer pointer
    /// @return A registered buffer
    virtual std::shared_ptr<IBuffer> getBuffer(const void *buf);

    /// @brief Look up a registered buffer. Returns a temporary unregistered
    ///        buffer (of size length, using flags) if no matching buffer is found.
    /// @param [in] buf Buffer pointer
    /// @param [in] length Buffer length
    /// @param [in] flags Buffer flags (unused)
    /// @return A registered or temporary unregistered buffer
    virtual std::shared_ptr<IBuffer> getBuffer(const void *buf, size_t length, int flags);

    //
    // File interface
    //

    /// @brief Registers a file. Files must be registered before they can be used with rocFile IO APIs
    /// @param [in] uf An unregistered file
    /// @return A handle to be used when calling rocFile IO APIs
    virtual rocFileHandle_t registerFile(const UnregisteredFile &uf);

    /// @brief Deregisters the file associated with the provided file handle
    /// @param [in] fh The handle of the file to deregister
    virtual void deregisterFile(rocFileHandle_t fh);

    /// @brief Look up a file given a rocFileHandle_t
    /// @param [in] fh The file handle to lookup the file with
    /// @return If file handle is valid, return a shared pointer to the file, otherwise throw
    /// FileNotRegistered.
    virtual std::shared_ptr<IFile> getFile(rocFileHandle_t fh);

    //
    // Stream interface
    //

    // @brief Registers a stream. Streams must be registered in order to set usage flags
    // @param [in] hip_stream A valid hipStream
    // @param [in] flags Stream flags
    virtual void registerStream(const hipStream_t hip_stream, uint32_t flags);

    // @brief Deregisters the stream
    // @param [in] hip_stream A valid hipStream
    virtual void deregisterStream(const hipStream_t hip_stream);

    // @brief Look up a stream given a hipStream_t
    // @param [in] hip_stream A valid hipStream
    // @return Return a shared pointer to the Stream
    virtual std::shared_ptr<IStream> getStream(hipStream_t hip_stream);

    //
    // Buffer and file calls
    //
    // These are for reducing the number of lock calls and are implemented
    // as needed for the rocFile code
    //

    /// @brief Look up a file and registered buffer
    ///
    /// This combined file + buffer getter reduces the number of lock calls.
    ///
    /// Like the buffer getter, this function emits a temporary unregistered buffer
    /// (of size length, using flags) if no matching buffer is found.
    ///
    /// @param [out] outFile File object
    /// @param [in]  fh      File handle
    /// @param [out] outBuf  Registered or temporary unregistered Buffer object
    /// @param [in]  buf     Buffer pointer
    /// @param [in]  length  Buffer length
    /// @param [in]  flags   Buffer flags (unused)
    virtual file_buffer_pair getFileAndBuffer(rocFileHandle_t fh, const void *buf, size_t length, int flags);

    //
    // Buffer, file, and stream calls
    //
    virtual file_buffer_stream_tuple getFileBufferAndStream(rocFileHandle_t fh, const void *buf,
                                                            size_t length, int flags, hipStream_t hipStream);

    //
    // Reference counts
    //

    /// @brief Increment the driver's reference count
    virtual void incrRefCount();

    /// @brief Decrement the driver's reference count
    ///
    /// When the count gets to 0, the buffer and file maps will be destroyed.
    virtual void decrRefCount();

    /// @brief Get the driver's current reference count
    /// @return The driver reference count
    virtual int64_t getRefCount() const;

    /// @brief Ensure the driver is initialized
    ///
    /// "Initializes" the driver by setting the reference count to 1
    /// if it is currently 0.
    ///
    /// @note For ensuring NVIDIA cuFile behaviour compatibility
    void ensureInitialized();

    //
    // Backends
    //

    /// @brief Get the backends that can service IO requests
    /// @return A collection of backends that can service IO requests
    virtual std::vector<std::shared_ptr<Backend>> getBackends() const;

    //
    // Misc
    //

    // Don't allow copying
    DriverState(const DriverState &)            = delete;
    DriverState &operator=(const DriverState &) = delete;

    // Don't allow moving
    DriverState(DriverState &&)            = delete;
    DriverState &operator=(DriverState &&) = delete;

protected:
    // Singletons should have private ctors.
    // However, it prevents creating Mocks that inherit from the
    // singleton for testing.
    DriverState();
    virtual ~DriverState();

private:
    // The driver's reference count
    int64_t ref_count;

    // Allows Context to manage DriverState.
    friend struct Context<DriverState>;

    // Manages the driver's File objects
    std::unique_ptr<FileMap> file_map;

    // Manages the allocated Batch Context's
    std::unique_ptr<BatchContextMap> batch_map;

    // Manages the driver's Buffer objects
    std::unique_ptr<BufferMap> buffer_map;

    // Manages the driver's Stream objects
    std::unique_ptr<StreamMap> stream_map;

    // Backends available to service IO requests
    const std::vector<std::shared_ptr<Backend>> backends;

    /// Mutex to protect the state
    mutable std::shared_mutex state_mutex;
};

}
