/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "stats.h"
#include "sys.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

static int
sendFd(int sock, int fd) noexcept
{
    int      data{1};
    iovec    iov{&data, sizeof(data)};
    msghdr   msgh;
    cmsghdr *cmsgp;

    union {
        char    buff[CMSG_SPACE(sizeof(int))];
        cmsghdr align;
    } controlMsg;

    msgh.msg_name       = nullptr;
    msgh.msg_namelen    = 0;
    msgh.msg_iov        = &iov;
    msgh.msg_iovlen     = 1;
    msgh.msg_control    = controlMsg.buff;
    msgh.msg_controllen = sizeof(controlMsg.buff);

    cmsgp             = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type  = SCM_RIGHTS;
    cmsgp->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsgp), &fd, sizeof(int));

    if (sendmsg(sock, &msgh, 0) == -1)
        return -1;

    return 0;
}

static void
populateSocketAddr(sockaddr_un &addr, pid_t pid) noexcept
{
    memset(&addr, 0, sizeof(addr));
    addr.sun_family  = AF_UNIX;
    addr.sun_path[0] = '\0'; // abstract namespace
    snprintf(&(addr.sun_path[1]), sizeof(addr.sun_path) - 1, "AISSTATS%08x", static_cast<uint32_t>(pid));
}

namespace hipFile {
StatsServer::StatsServer()
    : m_fd{FileDescriptor::make_managed(Context<Sys>::get()->memfd_create("AISSTATS", MFD_ALLOW_SEALING))},
      m_efd{FileDescriptor::make_managed(Context<Sys>::get()->eventfd(0, 0))}, m_stats{nullptr, &statsDeleter}
{
    int fd{m_fd.get()};
    Context<Sys>::get()->ftruncate(fd, sizeof(Stats));
    void *shm = Context<Sys>::get()->mmap(nullptr, sizeof(Stats), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    Context<Sys>::get()->fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_FUTURE_WRITE);
    m_stats  = UniqueStats{new (shm) Stats{}, &statsDeleter};
    m_thread = std::thread(&StatsServer::threadFn, this);
}

StatsServer::~StatsServer()
{
    if (m_thread.joinable()) {
        uint64_t i{1};
        write(m_efd.get(), &i, sizeof(i));
        m_thread.join();
    }
}

void
StatsServer::statsDeleter(Stats *s)
{
    if (s == nullptr) {
        return;
    }
    s->~Stats();
    Context<Sys>::get()->munmap(s, sizeof(Stats));
}

void
StatsServer::threadFn()
{
    int   sock{socket(AF_UNIX, SOCK_STREAM, 0)};
    pid_t pid{getpid()};
    if (sock == -1) {
        return;
    }
    sockaddr_un addr;
    populateSocketAddr(addr, pid);
    if (bind(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
        return;
    }
    if (listen(sock, 64) == -1) {
        return;
    }
    while (true) {
        pollfd pfd[2]{{sock, POLLIN, 0}, {m_efd.get(), POLLIN, 0}};
        poll(&pfd[0], 2, -1);
        if (pfd[0].revents & POLLIN) {
            socklen_t addrlen{sizeof(addr)};
            int       conn{accept(sock, reinterpret_cast<sockaddr *>(&addr), &addrlen)};
            if (conn == -1) {
                continue;
            }
            sendFd(conn, m_fd.get());
            close(conn);
        }
        if (pfd[1].revents & POLLIN) {
            break;
        }
    }
    close(sock);
}

void
statsAddFastPathRead(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFastPathReadBytes) += bytes;
    }
}

void
statsAddFastPathWrite(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFastPathWriteBytes) += bytes;
    }
}

void
statsAddFallbackPathRead(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFallbackPathReadBytes) += bytes;
    }
}

void
statsAddFallbackPathWrite(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFallbackPathWriteBytes) += bytes;
    }
}
}
