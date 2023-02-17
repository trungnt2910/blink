/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2023 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <wchar.h>

#include "blink/assert.h"
#include "blink/bitscan.h"
#include "blink/describeflags.h"
#include "blink/endian.h"
#include "blink/fds.h"
#include "blink/flag.h"
#include "blink/linux.h"
#include "blink/machine.h"
#include "blink/macros.h"
#include "blink/strace.h"
#include "blink/util.h"

#define APPEND(...) bi += snprintf(bp + bi, bn - bi, __VA_ARGS__)

struct thatispacked MagicNumber {
  int x;
  const char *s;
};

static const char *const kOpenAccmode[] = {
    "O_RDONLY",  //
    "O_WRONLY",  //
    "O_RDWR",    //
};

static const struct DescribeFlags kOpenFlags[] = {
    {O_APPEND_LINUX, "APPEND"},        //
    {O_CREAT_LINUX, "CREAT"},          //
    {O_EXCL_LINUX, "EXCL"},            //
    {O_TRUNC_LINUX, "TRUNC"},          //
    {O_NDELAY_LINUX, "NDELAY"},        //
    {O_DIRECT_LINUX, "DIRECT"},        //
    {O_TMPFILE_LINUX, "TMPFILE"},      // comes before O_DIRECTORY
    {O_DIRECTORY_LINUX, "DIRECTORY"},  //
    {O_NOFOLLOW_LINUX, "NOFOLLOW"},    //
    {O_CLOEXEC_LINUX, "CLOEXEC"},      //
    {O_NOCTTY_LINUX, "NOCTTY"},        //
    {O_ASYNC_LINUX, "ASYNC"},          //
    {O_NOATIME_LINUX, "NOATIME"},      //
    {O_PATH_LINUX, "PATH"},            //
    {O_LARGEFILE_LINUX, "LARGEFILE"},  //
    {O_SYNC_LINUX, "SYNC"},            // comes before O_DSYNC
    {O_DSYNC_LINUX, "DSYNC"},          //
};

static const struct DescribeFlags kProtFlags[] = {
    {PROT_READ_LINUX, "READ"},            //
    {PROT_WRITE_LINUX, "WRITE"},          //
    {PROT_EXEC_LINUX, "EXEC"},            //
    {PROT_SEM_LINUX, "SEM"},              //
    {PROT_GROWSDOWN_LINUX, "GROWSDOWN"},  //
    {PROT_GROWSUP_LINUX, "GROWSUP"},      //
};

static const struct DescribeFlags kMapFlags[] = {
    {MAP_SHARED_LINUX, "SHARED"},                    //
    {MAP_PRIVATE_LINUX, "PRIVATE"},                  //
    {MAP_FIXED_LINUX, "FIXED"},                      //
    {MAP_FIXED_NOREPLACE_LINUX, "FIXED_NOREPLACE"},  //
    {MAP_NONBLOCK_LINUX, "NONBLOCK"},                //
    {MAP_ANONYMOUS_LINUX, "ANONYMOUS"},              //
    {MAP_GROWSDOWN_LINUX, "GROWSDOWN"},              //
    {MAP_STACK_LINUX, "STACK"},                      //
    {MAP_NORESERVE_LINUX, "NORESERVE"},              //
    {MAP_POPULATE_LINUX, "POPULATE"},                //
    {MAP_HUGETLB_LINUX, "HUGETLB"},                  //
    {MAP_SYNC_LINUX, "SYNC"},                        //
    {MAP_UNINITIALIZED_LINUX, "UNINITIALIZED"},      //
};

static const struct DescribeFlags kSaFlags[] = {
    {SA_NOCLDSTOP_LINUX, "NOCLDSTOP"},            //
    {SA_NOCLDWAIT_LINUX, "NOCLDWAIT"},            //
    {SA_SIGINFO_LINUX, "SIGINFO"},                //
    {SA_UNSUPPORTED_LINUX, "UNSUPPORTED"},        //
    {SA_EXPOSE_TAGBITS_LINUX, "EXPOSE_TAGBITS"},  //
    {SA_RESTORER_LINUX, "RESTORER"},              //
    {SA_ONSTACK_LINUX, "ONSTACK"},                //
    {SA_RESTART_LINUX, "RESTART"},                //
    {SA_NODEFER_LINUX, "NODEFER"},                //
    {SA_RESETHAND_LINUX, "RESETHAND"},            //
};

static const struct DescribeFlags kAtFlags[] = {
    {AT_SYMLINK_NOFOLLOW_LINUX, "SYMLINK_NOFOLLOW"},  //
    {AT_REMOVEDIR_LINUX, "REMOVEDIR"},                //
    {AT_EACCESS_LINUX, "EACCESS"},                    //
    {AT_SYMLINK_FOLLOW_LINUX, "SYMLINK_FOLLOW"},      //
    {AT_EMPTY_PATH_LINUX, "EMPTY_PATH"},              //
};

#ifndef DISABLE_THREADS
static const struct DescribeFlags kCloneFlags[] = {
    {CLONE_VM_LINUX, "VM"},                          //
    {CLONE_THREAD_LINUX, "THREAD"},                  //
    {CLONE_FS_LINUX, "FS"},                          //
    {CLONE_FILES_LINUX, "FILES"},                    //
    {CLONE_SIGHAND_LINUX, "SIGHAND"},                //
    {CLONE_VFORK_LINUX, "VFORK"},                    //
    {CLONE_SYSVSEM_LINUX, "SYSVSEM"},                //
    {CLONE_SETTLS_LINUX, "SETTLS"},                  //
    {CLONE_PARENT_SETTID_LINUX, "PARENT_SETTID"},    //
    {CLONE_CHILD_CLEARTID_LINUX, "CHILD_CLEARTID"},  //
    {CLONE_DETACHED_LINUX, "DETACHED"},              //
    {CLONE_CHILD_SETTID_LINUX, "CHILD_SETTID"},      //
    {CLONE_NEWCGROUP_LINUX, "NEWCGROUP"},            //
    {CLONE_NEWUTS_LINUX, "NEWUTS"},                  //
    {CLONE_NEWIPC_LINUX, "NEWIPC"},                  //
    {CLONE_NEWUSER_LINUX, "NEWUSER"},                //
    {CLONE_NEWPID_LINUX, "NEWPID"},                  //
    {CLONE_NEWNET_LINUX, "NEWNET"},                  //
    {CLONE_IO_LINUX, "IO"},                          //
};
#endif

static const struct DescribeFlags kRenameFlags[] = {
    {RENAME_NOREPLACE_LINUX, "NOREPLACE"},  //
    {RENAME_EXCHANGE_LINUX, "EXCHANGE"},    //
    {RENAME_WHITEOUT_LINUX, "WHITEOUT"},    //
};

#ifdef HAVE_FORK
static const struct DescribeFlags kWaitFlags[] = {
    {WNOHANG_LINUX, "WNOHANG"},          //
    {WUNTRACED_LINUX, "WUNTRACED"},      //
    {WEXITED_LINUX, "WEXITED"},          //
    {WCONTINUED_LINUX, "WCONTINUED"},    //
    {WNOWAIT_LINUX, "WNOWAIT"},          //
    {__WNOTHREAD_LINUX, "__WNOTHREAD"},  //
    {__WALL_LINUX, "__WALL"},            //
    {__WCLONE_LINUX, "__WCLONE"},        //
};
#endif

#ifndef DISABLE_SOCKETS
static const struct DescribeFlags kSockFlags[] = {
    {SOCK_CLOEXEC_LINUX, "CLOEXEC"},    //
    {SOCK_NONBLOCK_LINUX, "NONBLOCK"},  //
};
#endif

const struct MagicNumber kSigHow[] = {
    {SIG_BLOCK, "SIG_BLOCK"},      //
    {SIG_UNBLOCK, "SIG_UNBLOCK"},  //
    {SIG_SETMASK, "SIG_SETMASK"},  //
};

const struct MagicNumber kSocketFamily[] = {
    {AF_UNSPEC_LINUX, "AF_UNSPEC"},    //
    {AF_UNIX_LINUX, "AF_UNIX"},        //
    {AF_INET_LINUX, "AF_INET"},        //
    {AF_INET6_LINUX, "AF_INET6"},      //
    {AF_NETLINK_LINUX, "AF_NETLINK"},  //
    {AF_PACKET_LINUX, "AF_PACKET"},    //
    {AF_VSOCK_LINUX, "AF_VSOCK"},      //
};

const struct MagicNumber kSocketType[] = {
    {SOCK_STREAM_LINUX, "SOCK_STREAM"},  //
    {SOCK_DGRAM_LINUX, "SOCK_DGRAM"},    //
    {SOCK_RAW_LINUX, "SOCK_RAW"},        //
};

const struct MagicNumber kClock[] = {
    {CLOCK_REALTIME_LINUX, "CLOCK_REALTIME"},                      //
    {CLOCK_MONOTONIC_LINUX, "CLOCK_MONOTONIC"},                    //
    {CLOCK_PROCESS_CPUTIME_ID_LINUX, "CLOCK_PROCESS_CPUTIME_ID"},  //
    {CLOCK_THREAD_CPUTIME_ID_LINUX, "CLOCK_THREAD_CPUTIME_ID"},    //
    {CLOCK_MONOTONIC_RAW_LINUX, "CLOCK_MONOTONIC_RAW"},            //
    {CLOCK_REALTIME_COARSE_LINUX, "CLOCK_REALTIME_COARSE"},        //
    {CLOCK_MONOTONIC_COARSE_LINUX, "CLOCK_MONOTONIC_COARSE"},      //
    {CLOCK_BOOTTIME_LINUX, "CLOCK_BOOTTIME"},                      //
    {CLOCK_REALTIME_ALARM_LINUX, "CLOCK_REALTIME_ALARM"},          //
    {CLOCK_BOOTTIME_ALARM_LINUX, "CLOCK_BOOTTIME_ALARM"},          //
    {CLOCK_TAI_LINUX, "CLOCK_TAI"},                                //
};

static const char *GetMagicNumber(const struct MagicNumber *p, int n, int x) {
  int i;
  _Thread_local static char ibuf[21];
  for (i = 0; i < n; ++i) {
    if (p[i].x == x) {
      return p[i].s;
    }
  }
  FormatInt64(ibuf, x);
  return ibuf;
}

static const char *DescribeSockaddr(const struct sockaddr_storage_linux *ss) {
  _Thread_local static char abuf[64], sabuf[80];
  switch (Read16(ss->family)) {
    case AF_UNIX_LINUX:
      return !ss->storage[sizeof(struct sockaddr_un_linux)]
                 ? ((const struct sockaddr_un_linux *)ss)->path
                 : 0;
    case AF_INET_LINUX:
      snprintf(sabuf, sizeof(sabuf), "{\"%s\", %d}",
               inet_ntop(Read16(ss->family),
                         &((const struct sockaddr_in_linux *)ss)->addr, abuf,
                         sizeof(abuf)),
               htons(((const struct sockaddr_in6_linux *)ss)->port));
      return sabuf;
    case AF_INET6_LINUX:
      snprintf(sabuf, sizeof(sabuf), "{\"%s\", %d}",
               inet_ntop(Read16(ss->family),
                         &((const struct sockaddr_in6_linux *)ss)->addr, abuf,
                         sizeof(abuf)),
               htons(((const struct sockaddr_in6_linux *)ss)->port));
      return sabuf;
    default:
      snprintf(abuf, sizeof(abuf), "{%s}",
               GetMagicNumber(kSocketFamily, ARRAYLEN(kSocketFamily),
                              Read16(ss->family)));
      return abuf;
  }
}

static void DescribeSigset(char *bp, int bn, u64 ss) {
  int sig, got = 0, bi = 0;
  if (popcount(ss) > 32) {
    APPEND("~");
    ss = ~ss;
  }
  APPEND("{");
  for (sig = 1; sig <= 64; ++sig) {
    if (ss & (1ull << (sig - 1))) {
      if (got) {
        APPEND(",");
      } else {
        got = true;
      }
      APPEND("%s", DescribeSignal(sig));
    }
  }
  APPEND("}");
}

static void Strace(struct Machine *m, const char *func, const char *fmt,
                   bool isentry, va_list va) {
  char *bp;
  i64 ax, arg;
  int c, i, bi, bn;
  char tmp[kStraceArgMax];
  char buf[7][kStraceArgMax];
  for (i = 0; i < 7; ++i) {
    buf[i][0] = 0;
  }
  i = 0;
  if (isentry) {
    strcpy(buf[0], "...");
    ++i;
  }
  for (ax = 0; (c = fmt[i]); ++i) {
    bi = 0;
    bp = buf[i];
    bn = sizeof(buf[i]);
    if (i >= 2) APPEND(", ");
    arg = va_arg(va, i64);
    if (IS_I32(c)) arg = (i32)arg;
    if (i == 0) ax = arg;
    if (i == 0 && arg == -1) {
      APPEND("-%s", DescribeHostErrno(errno));
      continue;
    }
    if (arg && IS_MEM_O(c) && !IS_ADDR(c)) APPEND("[");
    if (arg == -1) {
      APPEND("-1");
    } else if (c == I64_HEX[0] || (ax == -1 && (IS_MEM_O(c) || IS_MEM_IO(c)))) {
      APPEND("%#" PRIx64, arg);
    } else if (c == I64[0]) {
      APPEND("%" PRId64, arg);
    } else if (c == I32[0]) {
      APPEND("%" PRId32, (i32)arg);
    } else if (c == I32_OCTAL[0]) {
      APPEND("%#" PRIo32, (i32)arg);
    } else if (c == I32_FD[0] || c == I32_DIRFD[0]) {
      struct Fd *fd;
      if (c == I32_DIRFD[0] && arg == AT_FDCWD_LINUX) {
        APPEND("AT_FDCWD");
      } else {
        APPEND("%d", (int)arg);
        LOCK(&m->system->fds.lock);
        if ((fd = GetFd(&m->system->fds, arg))) {
          if (fd->path) {
            APPEND(" \"%s\"", fd->path);
          }
        }
        UNLOCK(&m->system->fds.lock);
      }
    } else if (IS_BUF(c)) {
      u8 *data;
      u64 len = va_arg(va, i64);
      unsigned j, preview = MIN(len, kStraceBufMax);
      if (c == O_BUF[0]) {
        len = MIN(len, ax);
        preview = MIN(preview, ax);
      }
      // APPEND("%#" PRIx64, arg);
      if ((data = (u8 *)SchlepR(m, arg, preview))) {
        APPEND("\"");
        for (j = 0; j < preview; ++j) {
          APPEND("%lc", (wint_t)kCp437[data[j]]);
        }
        APPEND("\"");
        if (j < len) {
          APPEND("...");
        }
      }
      ++i;
      snprintf(buf[i], sizeof(buf[i]), ", %#" PRIx64, len);
    } else if (c == I_STR[0]) {
      const char *str;
      if ((str = LoadStr(m, arg))) {
        APPEND("\"%s\"", str);
      } else {
        APPEND("%#" PRIx64, arg);
      }
    } else if (c == I32_SIG[0]) {
      APPEND("%s", DescribeSignal(arg));
#ifdef HAVE_FORK
    } else if (c == I32_WAITFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kWaitFlags, ARRAYLEN(kWaitFlags), "",
                    arg);
      APPEND("%s", DescribeSignal(arg));
    } else if (c == O_WSTATUS[0]) {
      const u8 *p;
      if ((p = (const u8 *)SchlepR(m, arg, 4))) {
        i32 s = Read32(p);
        if (WIFEXITED(s)) {
          APPEND("{WIFEXITED(s) && WEXITSTATUS(s) == %d}", WEXITSTATUS(s));
        } else if (WIFSTOPPED(s)) {
          APPEND("{WIFSTOPPED(s) && WSTOPSIG(s) == %s}",
                 DescribeSignal(WSTOPSIG(s)));
        } else if (WIFSIGNALED(s)) {
          APPEND("{WIFSIGNALED(s) && WTERMSIG(s) == %s}",
                 DescribeSignal(WSTOPSIG(s)));
        } else {
          APPEND("{%#" PRIx32 "}", s);
        }
      }
#endif
    } else if (c == I32_PROT[0]) {
      if (arg) {
        DescribeFlags(tmp, sizeof(tmp), kProtFlags, ARRAYLEN(kProtFlags),
                      "PROT_", arg);
        APPEND("%s", tmp);
      } else {
        APPEND("PROT_NONE");
      }
    } else if (c == I32_MAPFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kMapFlags, ARRAYLEN(kMapFlags), "MAP_",
                    arg);
      APPEND("%s", tmp);
    } else if (c == I32_OFLAGS[0]) {
      unsigned arg2 = arg;
      if ((arg2 & O_ACCMODE_LINUX) < ARRAYLEN(kOpenAccmode)) {
        APPEND("%s", kOpenAccmode[arg2 & O_ACCMODE_LINUX]);
        arg2 &= ~O_ACCMODE_LINUX;
      }
      if (arg2) {
        DescribeFlags(tmp, sizeof(tmp), kOpenFlags, ARRAYLEN(kOpenFlags), "O_",
                      arg2);
        APPEND("|%s", tmp);
      }
      if (!(arg & O_CREAT_LINUX) && !(arg & __O_TMPFILE_LINUX)) {
        ++i;  // ignore mode
      }
    } else if (c == I32_ATFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kAtFlags, ARRAYLEN(kAtFlags), "AT_", arg);
      APPEND("%s", tmp);
#ifndef DISABLE_THREADS
    } else if (c == I32_CLONEFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kCloneFlags, ARRAYLEN(kCloneFlags),
                    "CLONE_", arg);
      APPEND("%s", tmp);
#endif
    } else if (IS_SIGSET(c)) {
      const struct sigset_linux *ss;
      if ((ss = (const struct sigset_linux *)SchlepR(
               m, arg, sizeof(struct sigset_linux)))) {
        DescribeSigset(tmp, sizeof(tmp), Read64(ss->sigmask));
        APPEND("%s", tmp);
      } else {
        APPEND("%#" PRIx64, arg);
      }
#if defined(HAVE_FORK) || defined(HAVE_THREADS)
    } else if (c == O_PFDS[0]) {
      const u8 *pfds;
      if ((pfds = (const u8 *)SchlepR(m, arg, 8))) {
        APPEND("{%" PRId32 ", %" PRId32 "}", Read32(pfds), Read32(pfds + 4));
      } else {
        APPEND("%#" PRIx64, arg);
      }
#endif
    } else if (IS_TIME(c)) {
      const struct timespec_linux *ts;
      if ((ts = (const struct timespec_linux *)SchlepR(
               m, arg, sizeof(struct timespec_linux)))) {
        APPEND("{%" PRId64 ", %" PRId64 "}", Read64(ts->sec), Read64(ts->nsec));
      } else {
        APPEND("%#" PRIx64, arg);
      }
    } else if (IS_HAND(c)) {
      const struct sigaction_linux *sa;
      if ((sa = (const struct sigaction_linux *)SchlepR(
               m, arg, sizeof(struct sigaction_linux)))) {
        APPEND("{.sa_handler=");
        switch (Read64(sa->handler)) {
          case SIG_DFL_LINUX:
            APPEND("SIG_DFL");
            break;
          case SIG_IGN_LINUX:
            APPEND("SIG_DFL");
            break;
          default:
            APPEND("%#" PRIx64, Read64(sa->handler));
            break;
        }
        if (Read64(sa->flags)) {
          DescribeFlags(tmp, sizeof(tmp), kSaFlags, ARRAYLEN(kSaFlags), "SA_",
                        Read64(sa->flags));
          APPEND(", .sa_flags=%s", tmp);
        }
        if (Read64(sa->mask)) {
          DescribeSigset(tmp, sizeof(tmp), Read64(sa->mask));
          APPEND(", .sa_mask=%s", tmp);
        }
        if (Read64(sa->restorer)) {
          APPEND(", .sa_restorer=%#" PRIx64, Read64(sa->restorer));
        }
        APPEND("}");
      } else {
        APPEND("%#" PRIx64, arg);
      }
    } else if (c == I32_SIGHOW[0]) {
      APPEND("%s", GetMagicNumber(kSigHow, ARRAYLEN(kSigHow), arg));
#ifndef DISABLE_SOCKETS
    } else if (c == I32_CLOCK[0]) {
      APPEND("%s", GetMagicNumber(kClock, ARRAYLEN(kClock), arg));
    } else if (c == I32_FAMILY[0]) {
      APPEND("%s", GetMagicNumber(kSocketFamily, ARRAYLEN(kSocketFamily), arg));
    } else if (c == I32_SOCKTYPE[0]) {
      APPEND("%s", GetMagicNumber(kSocketType, ARRAYLEN(kSocketType), arg));
    } else if (c == I32_SOCKFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kSockFlags, ARRAYLEN(kSockFlags), "SOCK_",
                    arg);
      APPEND("%s", tmp);
    } else if (IS_ADDR(c)) {
      const u8 *p = 0;
      i64 len = va_arg(va, i64);
      struct sockaddr_storage_linux ss;
      memset(&ss, 0, sizeof(ss));
      if (c == I_ADDR[0]) len = (u32)len;
      if (c != I_ADDR[0] && !(p = (const u8 *)SchlepR(m, len, 4))) {
        APPEND("%#" PRIx64 ", %" PRId32, arg, Read32(p));
      } else {
        if (c == I_ADDR[0]) {
          len = (u32)len;
        } else {
          len = Read32(p);
        }
        if (c != I_ADDR[0]) APPEND("[");
        if (arg && CopyFromUserRead(m, &ss, arg, MIN(len, sizeof(ss))) != -1) {
          APPEND("%s", DescribeSockaddr(&ss));
        } else {
          APPEND("%#" PRIx64, arg);
        }
        if (c != I_ADDR[0]) APPEND("]");
        if (c == I_ADDR[0]) {
          APPEND(", %" PRId64, len);
        } else {
          APPEND(", [%" PRId64 "]", len);
        }
      }
      ++i;
#endif
    } else if (c == I32_RENFLAGS[0]) {
      DescribeFlags(tmp, sizeof(tmp), kRenameFlags, ARRAYLEN(kRenameFlags),
                    "RENAME_", arg);
      APPEND("%s", tmp);
    } else {
      unassert(!"missing strace signature specifier");
      __builtin_unreachable();
    }
    if (bi + 2 > bn) {
      bi = bn - 5;
      bp[bi++] = '.';
      bp[bi++] = '.';
      bp[bi++] = '.';
      bp[bi++] = 0;
    }
    if (arg && IS_MEM_O(c) && !IS_ADDR(c)) APPEND("]");
  }
  SYS_LOGF("%s(%s%s%s%s%s%s) -> %s", func, buf[1], buf[2], buf[3], buf[4],
           buf[5], buf[6], buf[0]);
}

void EnterStrace(struct Machine *m, const char *func, const char *fmt, ...) {
  va_list va;
  if (FLAG_strace > 1) {
    va_start(va, fmt);
    Strace(m, func, fmt, true, va);
    va_end(va);
  }
}

void LeaveStrace(struct Machine *m, const char *func, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  Strace(m, func, fmt, false, va);
  va_end(va);
}
