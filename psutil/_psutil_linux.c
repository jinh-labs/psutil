/*
 * Copyright (c) 2009, Giampaolo Rodola'. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Linux-specific functions.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <Python.h>
#include <linux/ethtool.h>  // DUPLEX_*

#include "arch/all/init.h"

// May happen on old RedHat versions, see:
// https://github.com/giampaolo/psutil/issues/607
#ifndef DUPLEX_UNKNOWN
#define DUPLEX_UNKNOWN 0xff
#endif

static PyMethodDef mod_methods[] = {
    // --- per-process functions
    {"proc_ioprio_get", psutil_proc_ioprio_get, METH_VARARGS},
    {"proc_ioprio_set", psutil_proc_ioprio_set, METH_VARARGS},
#ifdef PSUTIL_HAS_CPU_AFFINITY
    {"proc_cpu_affinity_get", psutil_proc_cpu_affinity_get, METH_VARARGS},
    {"proc_cpu_affinity_set", psutil_proc_cpu_affinity_set, METH_VARARGS},
#endif
    // --- system related functions
    {"disk_partitions", psutil_disk_partitions, METH_VARARGS},
    {"net_if_duplex_speed", psutil_net_if_duplex_speed, METH_VARARGS},
#ifdef PSUTIL_HAS_HEAP_INFO
    {"heap_info", psutil_heap_info, METH_VARARGS},
#endif
#ifdef PSUTIL_HAS_HEAP_TRIM
    {"heap_trim", psutil_heap_trim, METH_VARARGS},
#endif

    // --- linux specific
    {"linux_sysinfo", psutil_linux_sysinfo, METH_VARARGS},
    // --- others
    {"check_pid_range", psutil_check_pid_range, METH_VARARGS},
    {"set_debug", psutil_set_debug, METH_VARARGS},
    {NULL, NULL, 0, NULL}
};


static int
_psutil_linux_exec(PyObject *mod) {
    // Record the module name so C helpers that don't receive the module
    // object can resolve the current interpreter's module (see
    // psutil_set_zombie_error()).
    psutil_posix_set_module(mod);

    if (psutil_setup() != 0)
        return -1;
    if (psutil_posix_add_constants(mod) != 0)
        return -1;
    if (psutil_posix_add_methods(mod) != 0)
        return -1;

    if (PyModule_AddIntConstant(mod, "version", PSUTIL_VERSION))
        return -1;
    if (PyModule_AddIntConstant(mod, "DUPLEX_HALF", DUPLEX_HALF))
        return -1;
    if (PyModule_AddIntConstant(mod, "DUPLEX_FULL", DUPLEX_FULL))
        return -1;
    if (PyModule_AddIntConstant(mod, "DUPLEX_UNKNOWN", DUPLEX_UNKNOWN))
        return -1;

    return 0;
}


static PyModuleDef_Slot _psutil_linux_slots[] = {
    {Py_mod_exec, _psutil_linux_exec},
// Declare that the module does not rely on the GIL, for free-threaded
// builds. This is the multi-phase-init replacement for the unstable
// PyUnstable_Module_SetGIL() call; the slot only exists on 3.13+.
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
// Declare support for being loaded in multiple (shared-GIL) sub-interpreters.
// Only exposed on Python >= 3.12 APIs; the stable-ABI wheel omits it, but
// CPython already defaults multi-phase modules to this value.
#ifdef Py_mod_multiple_interpreters
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED},
#endif
    {0, NULL}
};


static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_psutil_linux",
    .m_size = sizeof(struct module_state),
    .m_methods = mod_methods,
    .m_slots = _psutil_linux_slots,
    .m_traverse = psutil_posix_traverse,
    .m_clear = psutil_posix_clear,
    .m_free = psutil_posix_free,
};


PyMODINIT_FUNC
PyInit__psutil_linux(void) {
    return PyModuleDef_Init(&moduledef);
}
