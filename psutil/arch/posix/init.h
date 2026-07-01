/*
 * Copyright (c) 2009, Giampaolo Rodola'. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Per-interpreter module state shared by all POSIX platform modules
// (_psutil_linux, _psutil_bsd, _psutil_osx, _psutil_aix, _psutil_sunos).
// Keeping the exception here (rather than as a process-global) means each
// (sub)interpreter gets its own ZombieProcessError class object, which is
// required for `except cext.ZombieProcessError` to work correctly across
// interpreters.
struct module_state {
    PyObject *ZombieProcessError;
};

#define GETSTATE(mod) ((struct module_state *)PyModule_GetState(mod))

// GC / deallocation hooks for the module state (m_traverse / m_clear /
// m_free).
int psutil_posix_traverse(PyObject *mod, visitproc visit, void *arg);
int psutil_posix_clear(PyObject *mod);
void psutil_posix_free(void *mod);

// Records the platform module's fully-qualified name so that C helpers which
// do not receive the module object (e.g. psutil_raise_for_pid) can still
// resolve the *current interpreter's* module on demand. Call once from exec.
void psutil_posix_set_module(PyObject *mod);

// Raise the current interpreter's ZombieProcessError with the given message.
// Never raises with a NULL type (falls back to RuntimeError).
void psutil_set_zombie_error(const char *msg);

// convert a timeval struct to a double
#ifdef PSUTIL_SUNOS
#define PSUTIL_TV2DOUBLE(t) (((t).tv_nsec * 0.000000001) + (t).tv_sec)
#else
#define PSUTIL_TV2DOUBLE(t) ((t).tv_sec + (t).tv_usec / 1000000.0)
#endif

// clang-format off
#if !defined(PSUTIL_OPENBSD) && !defined(PSUTIL_AIX)
    #define PSUTIL_HAS_POSIX_USERS
    PyObject *psutil_users(PyObject *self, PyObject *args);
#endif

#if defined(PSUTIL_BSD) || defined(PSUTIL_OSX)
    #include <sys/types.h>
    #define PSUTIL_HAS_SYSCTL
    int psutil_sysctl(int *mib, u_int miblen, void *buf, size_t buflen);
    int psutil_sysctl_malloc(int *mib, u_int miblen, char **buf, size_t *buflen);
    size_t psutil_sysctl_argmax();

    #if !defined(PSUTIL_OPENBSD)
        #define PSUTIL_HAS_SYSCTLBYNAME
        int psutil_sysctlbyname(const char *name, void *buf, size_t buflen);
        int psutil_sysctlbyname_malloc(const char *name, char **buf, size_t *buflen);
    #endif
#endif

#if defined(PSUTIL_BSD) || defined(PSUTIL_OSX)
    #define PSUTIL_HAS_NET_IF_DUPLEX_SPEED
    PyObject *psutil_net_if_duplex_speed(PyObject *self, PyObject *args);
#endif
// clang-format on

// --- internal utils

int psutil_pid_exists(pid_t pid);
long psutil_getpagesize(void);
int psutil_posix_add_constants(PyObject *mod);
int psutil_posix_add_methods(PyObject *mod);
PyObject *psutil_raise_for_pid(pid_t pid, char *msg);

// --- Python wrappers

PyObject *psutil_getpagesize_pywrapper(PyObject *self, PyObject *args);
PyObject *psutil_net_if_addrs(PyObject *self, PyObject *args);
PyObject *psutil_net_if_flags(PyObject *self, PyObject *args);
PyObject *psutil_net_if_is_running(PyObject *self, PyObject *args);
PyObject *psutil_net_if_mtu(PyObject *self, PyObject *args);
PyObject *psutil_proc_priority_get(PyObject *self, PyObject *args);
PyObject *psutil_proc_priority_set(PyObject *self, PyObject *args);
#if defined(PSUTIL_OSX) || defined(PSUTIL_BSD)
PyObject *psutil_proc_is_zombie(PyObject *self, PyObject *args);
#endif
