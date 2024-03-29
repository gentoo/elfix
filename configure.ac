#
#	configure.ac: this file is part of the elfix package
#	Copyright (C) 2011  Anthony G. Basile
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

AC_PREREQ([2.71])
AC_INIT([elfix],[0.9],[https://bugs.gentoo.org/])
AC_CONFIG_SRCDIR([src/paxctl-ng.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([1.15 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([no])

LT_PREREQ([2.4])
LT_INIT([dlopen])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_SED

# Checks for header files.
AC_CHECK_HEADERS(
    [errno.h err.h fcntl.h libgen.h stdio.h stdlib.h string.h \
    sys/mman.h sys/stat.h sys/types.h unistd.h],
    [],
    [AC_MSG_ERROR(["Missing necessary header"])]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MMAP
AC_CHECK_FUNCS([memset strerror])

AC_ARG_ENABLE(
    [tests],
    AS_HELP_STRING(
        [--enable-tests],
        [perform tests]
    ),
    [test "x$enable_tests" = "xyes"]
)
AM_CONDITIONAL([TEST],[test "x$enable_tests" = "xyes"])

AC_ARG_ENABLE(
    [ptpax],
    AS_HELP_STRING(
        [--enable-ptpax],
        [enable support for pax markings in PT_PAX ELF phdr]
    )
)

AS_IF(
    [test "x$enable_ptpax" != "xno"],
    [
        AC_CHECK_HEADERS(
            [gelf.h],
            [],
            [AC_MSG_ERROR(["Missing necessary gelf.h"])]
        )
        AC_CHECK_LIB(
            [elf],
            [elf_begin],
            [],
            [AC_MSG_ERROR(["Missing necessary function elf_begin in libelf"])]
        )
        AC_CHECK_DECL(
            [ELF_C_RDWR_MMAP],
            [],
            [AC_MSG_ERROR(["Missing necessary DECL ELF_C_RDWR_MMAP in libelf.h"])],
            [[#include <libelf.h>]]
        )
        AC_CHECK_DECLS(
            [PT_PAX_FLAGS, PF_PAGEEXEC, PF_MPROTECT, PF_RANDMMAP],
            [],
            [CFLAGS="${CFLAGS} -DNEED_PAX_DECLS"],
            [[#include <gelf.h>]]
        )
        CFLAGS="${CFLAGS} -DPTPAX"
    ],
    [
        CFLAGS="${CFLAGS} -UPTPAX -DNEED_PAX_DECLS"
    ]
)

AC_ARG_ENABLE(
    [xtpax],
    AS_HELP_STRING(
        [--enable-xtpax],
        [enable support for pax markings in xattrs]
    )
)

AS_IF(
    [test "x$enable_xtpax" != "xno"],
    [
        AC_CHECK_HEADERS(
            [sys/xattr.h],
            [],
            [AC_MSG_ERROR(["Missing necessary sys/xattr.h"])]
        )
        AC_CHECK_LIB(
            [attr],
            [fgetxattr],
            [],
            [AC_MSG_ERROR(["Missing necessary function fgetxattr in libattr"])]
        )
        CFLAGS="${CFLAGS} -DXTPAX"
    ],
    [
        CFLAGS="${CFLAGS} -UXTPAX"
    ]
)

if [test "x$enable_ptpax" = "xno" -a "x$enable_xtpax" = "xno" ]; then
    AC_MSG_ERROR(["You must enable either ptpax or xtpax"])
fi

AM_CONDITIONAL([DUALTEST],[test "x$enable_ptpax" = "xyes" -a  "x$enable_xtpax" = "xyes"])

# Ready to configure our files
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    scripts/Makefile
    doc/Makefile
    tests/Makefile
    tests/pxtpax/Makefile
    tests/paxmodule/Makefile
    tests/revdeppaxtest/Makefile
])

AC_OUTPUT
