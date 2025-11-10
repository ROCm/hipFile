/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "buffer.h"
#include "context.h"
#include "fastpath.h"
#include "file.h"
#include "hip.h"
#include "io.h"

#include <cstdint>
#include <fcntl.h>
#include <hip/hip_runtime_api.h>
#include <stdexcept>

using namespace rocFile;
using namespace std;

/* The fastpath backend is used when:
 *  - The file has been opened with the O_DIRECT flag
 *  - file_offset is 4KiB aligned
 *  - buffer_offset is 4KiB aligned
 *  - size is a multiple of 4KiB
 *  - The buffer type is hipMemoryTypeDevice
 *
 * When using the fastpath the IO flows through the following
 *  - rocFile HIP wrapper (userspace)
 *      - repository: hipFile
 *      - file: rocfile/hip.cpp
 *      - methods: Hip::hipAmdFileRead(...), Hip::hipAmdFileWrite(...)
 *      - throws
 *          - std::system_error if status is non-zero (set by kfd)
 *          - Hip::runtime_error if hip runtime does not return hipSuccess or
 *            if rocFile was unable to find hipAmdFileRead/hipAmdFileWrite
 *  - HIP Runtime (userspace)
 *      - repository: rocm-systems
 *      - file: projects/clr/hipamd/src/hip_storage.cpp
 *      - functions: hipAmdFileRead(...), hipAmdFileWrite(...)
 *      - hipAmdFileRead & hipAmdFileWrite are not exposed through a public
 *        header so hipGetProcAddress() is used to lookup each function's function
 *        pointer
 *      - returns
 *          - hipSuccess if size is zero
 *              - This check should be removed
 *          - hipErrorInvalidDevice if unable to lookup current device
 *          - hipErrorUnknown if rocr runtime does not return success
 *  - ROCR Runtime (userspace)
 *      - repository: rocm-systems
 *      - file: projects/clr/rocclr/device/rocm/rocdevice.cpp
 *      - methods: Device::amdFileRead(...), Device::amdFileWrite(...)
 *      - Does not perform any input validation
 *      - Logs on every IO failure
 *      - size_copied and status are never modified
 *      - returns error if HSA does not return HSA_STATUS_SUCCESS
 *  - HSA (userspace)
 *      - repository: rocm-systems
 *      - file: projects/rocr-runtime/runtime/hsa-runtime/core/runtime/hsa_ext_amd.cpp
 *      - hsa_amd_ais_file_read(...), hsa_amd_ais_file_write(...)
 *      - status and size_coppied untouched
 *      - returns
 *          - HSA_STATUS_ERROR_INVALID_ARGUMENT if size is zero
 *              - This check should be be removed
 *          - HSA_STATUS_ERROR_INVALID_ARGUMENT if devicePtr is NULL
 *              - This check should be be removed
 *          - HSA_STATUS_ERROR if hsaKmtAisReadWriteFile(...) does not return success
 *  - Thunk (userspace)
 *      - repository: rocm-systems
 *      - file: projects/rocr-runtime/libhsakmt/src/ais.c
 *      - functions: hsaKmtAisReadWriteFile(...),
 *      - If the buffer pointer, and io_size combination are invalid, return HSAKMT_STATUS_INVALID_PARAMETER
 *          - converts buffer pointer + io_size to a buffer handle (no alignment check)
 *      - size_copied and status are untouched
 *      - returns
 *          - HSAKMT_STATUS_INVALID_PARAMETER if
 *              - the buffer pointer + size combination are invalid
 *              - if AisFlags (read/write) are invalid
 *          - HSAKMT_STATUS_ERROR if the kfd ioctl does not return success
 *  - KFD (chardev) (kernel)
 *      - repository: ROCm
 *      - file: amdgpu/drivers/gpu/drm/amd/amdkfd/kfd_chardev.c
 *      - function: kfd_ioctl_ais(...)
 *      - returns
 *          - -EINVAL if op != READ or WRITE
 *          - -EINVAL if fd < 0
 *          - -EINVAL if unable to lookup device
 *          - -NODEV if AIS is not initialized on the device
 *          - -ESRCH if unable to bind the process to the device
 *          - -EINVAL if the buffer's domain is not AMDGPU_GEM_DOMAIN_VRAM
 *          - an error if it is unable to pin the buffer object
 *              - error is from amdgpu_amdkfd_gpuvm_pin_bo(...)
 *          - the result of the call to kfd_ais_rw_file(...)
 *      - the value returned is also written to the status variable
 *  - KFD (kernel)
 *      - repository: ROCm
 *      - file: amdgpu/drivers/gpu/drm/amd/amdkfd/kfd_ais.c
 *      - function: kfd_ais_rw_file(...)
 *      - Calls dev_dbg on every successful IO
 *          - May want to remove this
 *      - returns
 *          - -EINVAL if file_offset or size are not page aligned (4K)
 *              - This check should be removed /if/ the IO stack checks for alignment.
 *          - -EBADF unable to lookup the file object
 *          - -ENODEV if unable to lookup the storage device
 *          - -ENODEV if pcie_p2pdma_distance < 0
 *              - This should be changed to return -EREMOTE
 *          - -EINVAL if buffer object domain is not AMDGPU_GEM_DOMAIN_VRAM
 *          - an error if unable to create a sg_table for the buffer object
 *              - error is from amdgpu_amdkfd_gpuvm_get_sg_table(...)
 *          - -ENOMEM if unable to init a bvec
 *          - -EIO on short reads
 *              - Need to confirm & fix
 */

int
Fastpath::score(shared_ptr<IFile> file, shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
                hoff_t buffer_offset) const
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
Fastpath::io(IoType type, shared_ptr<IFile> file, shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
             hoff_t buffer_offset)
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
