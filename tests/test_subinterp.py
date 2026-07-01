#!/usr/bin/env python3

# Copyright (c) 2009, Giampaolo Rodola'. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for loading psutil in CPython sub-interpreters (PEP 489).

The C extensions use multi-phase initialization and keep their exception
classes in per-interpreter module state, so they can be loaded in *shared-GIL*
sub-interpreters on CPython 3.12+.

The tests create the sub-interpreter with check_multi_interp_extensions
enabled. That is the setting which enforces the Py_mod_multiple_interpreters
gate: an extension that does not advertise multi-interpreter support is
rejected. (The legacy run_in_subinterp() leaves this check off, so it would
import even a single-phase extension and would not exercise the gate.)

Per-interpreter GIL / free-threaded isolation is not supported yet, so the
sub-interpreter uses the shared GIL (own_gil=False).
"""

import sys
import textwrap

import psutil
from psutil import POSIX

from . import PsutilTestCase
from . import pytest

try:
    from test.support import run_in_subinterp_with_config
except ImportError:
    run_in_subinterp_with_config = None

# The C extension module (e.g. psutil._psutil_linux) for this platform.
CEXT = psutil._psplatform.cext
CEXT_NAME = CEXT.__name__

# Shared-GIL sub-interpreter with the multi-interpreter extension check on.
# All fields are required by run_in_subinterp_with_config().
_SUBINTERP_CONFIG = {
    "use_main_obmalloc": True,
    "allow_fork": False,
    "allow_exec": False,
    "allow_threads": True,
    "allow_daemon_threads": False,
    "check_multi_interp_extensions": True,
    "own_gil": False,
}


def run_psutil_in_subinterp(code):
    """Run `code` in a shared-GIL sub-interpreter with the multi-interpreter
    gate enabled; return its exit code (0 == success). A non-zero code means
    the code (or importing the extension) failed in the sub-interpreter.
    """
    return run_in_subinterp_with_config(code, **_SUBINTERP_CONFIG)


@pytest.mark.skipif(
    run_in_subinterp_with_config is None,
    reason="no run_in_subinterp_with_config",
)
@pytest.mark.skipif(
    sys.version_info[:2] < (3, 12), reason="needs Python >= 3.12"
)
class TestSubInterpreters(PsutilTestCase):
    def test_import_and_basic_apis(self):
        # psutil imports and its basic APIs work inside a sub-interpreter.
        code = textwrap.dedent("""
            import psutil
            assert len(psutil.pids()) > 0
            assert psutil.cpu_count() > 0
            p = psutil.Process()
            assert p.name()
            assert p.memory_info().rss > 0
        """)
        assert run_psutil_in_subinterp(code) == 0

    @pytest.mark.skipif(not POSIX, reason="POSIX only")
    def test_zombie_exception_is_per_interpreter(self):
        # The C-level ZombieProcessError must be a *distinct* object in each
        # interpreter, otherwise `except cext.ZombieProcessError` wouldn't
        # match an exception raised by the interpreter it runs in.
        main_id = id(CEXT.ZombieProcessError)
        path = self.get_testfn()
        code = textwrap.dedent(f"""
            import {CEXT_NAME} as cext
            exc = cext.ZombieProcessError
            assert issubclass(exc, BaseException)
            with open({path!r}, "w") as f:
                f.write(str(id(exc)))
        """)
        assert run_psutil_in_subinterp(code) == 0
        with open(path) as f:
            sub_id = int(f.read())
        # Same process address space, so distinct ids => distinct objects.
        assert sub_id != main_id
