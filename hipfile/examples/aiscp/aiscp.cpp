/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/* aiscp - AMD Infinity Storage Copy
 *
 * Usage: ./aiscp SOURCE DEST
 *
 * This _very_basic_ program copies SOURCE to DEST via GPU memory.
 *
 * Note: Currently SOURCE's size must be > 0 and a multiple of 4096.
 */

#include <hipfile.h>
#include <hip/hip_runtime_api.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/// @brief Open and register a file
/// @param path [in] Path to the file
/// @param flags [in] flags: Flags to pass to open (2)
/// @param mode [in] mode: Mode to pass to open (2)
/// @param fd [out] fd: The file descriptor of the opened file
/// @param handle [out] handle: The handle to use with hipFile APIs
/// @return zero on success, non-zero on failure
static int
open_file(const char *path, int flags, mode_t mode, int *fd, hipFileHandle_t *handle)
{
    hipFileError_t hipfile_err;
    hipFileDescr_t descr;

    *fd = open(path, flags | O_DIRECT, mode);
    if (-1 == *fd) {
        fprintf(stderr, "Could not open %s (%s)\n", path, strerror(errno));
        return 1;
    }

    descr.type      = hipFileHandleTypeOpaqueFD;
    descr.handle.fd = *fd;

    hipfile_err = hipFileHandleRegister(handle, &descr);
    if (hipFileSuccess != hipfile_err.err) {
        fprintf(stderr, "Could not register %s (%s)\n", path, hipFileOpStatusError(hipfile_err.err));
        close(*fd);
        return 1;
    }

    return 0;
}

/// @brief Unregister and close a file
/// @param path [in] Path to the file
/// @param fd [in] The file descriptor of the opened file
/// @param handle [in] The handle of the opened file
/// @return zero on success, non-zero on failure
static int
close_file(const char *path, int fd, hipFileHandle_t handle)
{
    hipFileHandleDeregister(handle);
    if (-1 == close(fd)) {
        fprintf(stderr, "Could not close %s (%s)\n", path, strerror(errno));
        return 1;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    const char     *src_path, *dst_path;
    int             src_fd, dst_fd;
    hipFileHandle_t src_handle, dst_handle;
    void           *devbuf;
    hipError_t      hip_err;
    int             exit_status = EXIT_FAILURE;
    size_t          file_size;
    ssize_t         nbytes;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s SOURCE DEST\n", argv[0]);
        exit(1);
    }

    src_path = argv[1];
    dst_path = argv[2];

    {
        struct stat statbuf;
        if (stat(src_path, &statbuf)) {
            fprintf(stderr, "Could not stat file: %s (%s)\n", src_path, strerror(errno));
            goto program_exit;
        }
        file_size = static_cast<size_t>(statbuf.st_size);
    }

    if (0 == file_size || file_size % 4096) {
        fprintf(stderr, "SOURCE's length must be non-zero and a multiple of 4096\n");
        goto program_exit;
    }

    if (open_file(src_path, O_DIRECT | O_RDONLY, 0, &src_fd, &src_handle)) {
        goto program_exit;
    }

    if (open_file(dst_path, O_DIRECT | O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH, &dst_fd,
                  &dst_handle)) {
        goto close_src;
    }

    hip_err = hipMalloc(&devbuf, file_size);
    if (hipSuccess != hip_err) {
        fprintf(stderr, "Could not allocate device buffer (%d)", hip_err);
        goto close_dst;
    }

    nbytes = hipFileRead(src_handle, devbuf, file_size, 0, 0);
    if (nbytes < 0 || file_size != static_cast<size_t>(nbytes)) {
        fprintf(stderr, "Could not read from %s (%zd) (%s)\n", src_path, nbytes,
                IS_HIPFILE_ERR(nbytes) ? HIPFILE_ERRSTR(nbytes) : strerror(errno));
        goto free_devbuf;
    }

    nbytes = hipFileWrite(dst_handle, devbuf, file_size, 0, 0);
    if (nbytes < 0 || file_size != static_cast<size_t>(nbytes)) {
        fprintf(stderr, "Could not write to %s (%zd) (%s)\n", src_path, nbytes,
                IS_HIPFILE_ERR(nbytes) ? HIPFILE_ERRSTR(nbytes) : strerror(errno));
        goto free_devbuf;
    }

    exit_status = EXIT_SUCCESS;

free_devbuf:
    hip_err = hipFree(devbuf);
    if (hipSuccess != hip_err) {
        fprintf(stderr, "Could free device buffer (%d)\n", hip_err);
        exit_status = EXIT_FAILURE;
    }

close_dst:
    if (close_file(dst_path, dst_fd, dst_handle)) {
        exit_status = EXIT_FAILURE;
    }

close_src:
    if (close_file(src_path, src_fd, src_handle)) {
        exit_status = EXIT_FAILURE;
    }

program_exit:
    return exit_status;
}
