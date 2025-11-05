/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "fastpath.h"
#include "rocfile.h"

#include <limits>
#include <sys/mman.h>

using namespace rocFile;
using namespace std;

/* The fastpath backend is used when:
 *  - IO is alligned (size, file_offset, buffer_offset)
 *  - The buffer is of type hipMemoryTypeDevice
 *  - The file has been opened with the O_DIRECT flag
 *  - The file is either a block device or a regular file
 *  - The file resides on a an xfs or ext4 (journaling mode: ordered) filesystem
 *
 * When using the fastpath the IO flows through the following layers
 *  - rocfile hip wrapper (userspace)
 *      - repository: hipFile
 *      - file: rocfile/hip.cpp
 *      - methods: Hip::hipAmdFileRead(...), Hip::hipAmdFileWrite(...)
 *      - Throws std::system_error if status is non-zero (set by kfd)
 *      - Throws Hip::runtime_error if hip runtime does not return hipSuccess or
 *        if rocFile was unable to find hipAmdFileRead/hipAmdFileWrite
 *  - hip runtime (userspace)
 *      - repository: rocm-systems
 *      - file: projects/clr/hipamd/src/hip_storage.cpp
 *      - functions: hipAmdFileRead(...), hipAmdFileWrite(...)
 *      - hipAmdFileRead & hipAmdFileWrite are not exposed through a public
 *        header so hipGetProcAddress() is used to lookup each function's function
 *        pointer
 *      - **Currently returns hipSuccess if size is zero (Need to fix)**
 *      - returns hipErrorInvalidDevice if unable to lookup current device
 *      - returns hipErrorUnknown if rocr runtime does not return true (success)
 *  - rocr runtime (userspace)
 *      - repository: rocm-systems
 *      - file: projects/clr/rocclr/device/rocm/rocdevice.cpp
 *      - methods: Device::amdFileRead(...), Device::amdFileWrite(...)
 *      - Does not perform any input validation
 *      - Logs on every IO failure
 *      - size_copied and status are never modified
 *      - returns false (error) if hsa does not return HSA_STATUS_SUCCESS
 *  - hsa (userspace)
 *      - repository: rocm-systems
 *      - file: projects/rocr-runtime/runtime/hsa-runtime/core/runtime/hsa_ext_amd.cpp
 *      - hsa_amd_ais_file_read(...), hsa_amd_ais_file_write(...)
 *      - ** Currently returns HSA_STATUS_ERROR_INVALID_ARGUMENT if size is zero
 *           or device (buffer) pointer is zero **
 *      - status and size_coppied are left untouched
 *      - if devicePtr is NULL or size is zero, HSA_STATUS_ERROR_INVALID_ARGUMENT is returned
 *      - if hsaKmtAisReadWriteFile(...) does not return success, HSA_STATUS_ERROR is returned
 *  - thunk (userspace)
 *      - repository: rocm-systems
 *      - file: projects/rocr-runtime/libhsakmt/src/ais.c
 *      - functions: hsaKmtAisReadWriteFile(...),
 *      - If the buffer pointer, and io_size combination are invalid, return HSAKMT_STATUS_INVALID_PARAMETER
 *          - converts buffer pointer + io_size to a buffer handle
 *          - Does not check for alignment
 *      - size and status are untouched and return HSAKMT_STATUS_INVALID_PARAMETER if
 *          - the buffer pointer + size combination are invalid
 *          - if AisFlags (read/write) are invalid
 *      - If the kfd ioctl does not return zero, return HSAKMT_STATUS_ERROR
 *  - kfd (chardev) (kernel)
 *      - repository: ROCm
 *      - file: amdgpu/drivers/gpu/drm/amd/amdkfd/kfd_chardev.c
 *      - function: kfd_ioctl_ais(...)
 *      - returns -EINVAL if op != READ or WRITE
 *      - returns -EINVAL if fd < 0
 *      - returns -EINVAL if unable to lookup device
 *      - returns -NODEV if AIS is not initialized on the device
 *      - returns -ESRCH if unable to bind the process ot the device
 *      - returns -EINVAL if the buffer's domain is not AMDGPU_GEM_DOMAIN_VRAM
 *      - returns an error if it is unable to pin the buffer object
 *      - returns the result of the call to kfd_ais_rw_file(...)
 *      - the value returned by kfd_ioctl_ais() is also stored in the status
 *        variable provided in the initial call to
 *        hipAmdFileRead/hipAmdFileWrite
 *  - kfd (kernel)
 *      - repository: ROCm
 *      - file: amdgpu/drivers/gpu/drm/amd/amdkfd/kfd_ais.c
 *      - function: kfd_ais_rw_file(...)
 *      - returns -EINVAL if file_offset or size are not page aligned (4K)
 *      - returns -EBADF unable to lookup the file object
 *      - returns -ENODEV if unable to lookup the storage device
 *      - returns -ENODEV if pcie_p2pdma_distance < 0
 *      - returns -EINVAL if buffer object domain is not AMDGPU_GEM_DOMAIN_VRAM
 *      - returns an error if unable to create a sg_table for the buffer object
 *      - returns -ENOMEM if unbale to init a bvec
 *      - ** Appears to return -EIO on short reads **
 *      - ** Calls dev_dbg on every IO that doesn't return an error **
 *      - ** Should not retry on short read/writes **
 */

int
Fastpath::score(shared_ptr<IFile> file, shared_ptr<IBuffer> buffer, size_t size, off_t file_offset,
                off_t buffer_offset) const
{
    bool accept_io{true};

    accept_io &= !!(file->getStatusFlags() & O_DIRECT);

    accept_io &= buffer->getType() == hipMemoryTypeDevice;

    accept_io &= 0 <= file_offset;
    accept_io &= 0 <= buffer_offset;

    accept_io &= !(size & 0xFFF);
    accept_io &= !(file_offset & 0xFFF);
    accept_io &= !(buffer_offset & 0xFFF);

    return accept_io ? 100 : -1;
}

ssize_t
Fastpath::io(IoType type, shared_ptr<IFile> file, shared_ptr<IBuffer> buffer, size_t size, off_t file_offset,
             off_t buffer_offset)
{
    void *devptr{reinterpret_cast<void *>(reinterpret_cast<intptr_t>(buffer->getBuffer()) + buffer_offset)};
    hipAmdFileHandle_t handle{};
    size_t             nbytes{};

    handle.fd = file->getFd();

    switch (type) {
        case IoType::Read:
            nbytes = Context<Hip>::get()->hipAmdFileRead(handle, devptr, size, file_offset);
            break;
        case IoType::Write:
            nbytes = Context<Hip>::get()->hipAmdFileWrite(handle, devptr, size, file_offset);
            break;
        default:
            throw std::runtime_error("Invalid IoType");
    }

    return static_cast<ssize_t>(nbytes);
}
