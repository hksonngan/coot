# coot-windows.m4
# 
# Copyright 2006 The University of York
# Author: Paul Emsley
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA


AC_DEFUN([AM_MINGW_WINDOWS],
[

AC_MSG_CHECKING([if this is MINGW on Windows])

 COOT_WINDOWS_CFLAGS=
 COOT_WINDOWS_LDFLAGS=
 have_windows_mingw=no
 windows=false

 # BL: workaround needed for new MinGW
 ac_cv_build_alias=${ac_cv_build_alias:=$build_alias}

 case $ac_cv_build_alias in 

  *-mingw*)
    COOT_WINDOWS_CFLAGS="-DWINDOWS_MINGW -DUSE_GNOME_CANVAS"
    # BL says:: may need rethink for shared compilation of course!!
    COOT_WINDOWS_LDFLAGS="-static -lstdc++"
    have_windows_mingw=yes
    windows=true
    ;;
 esac

AM_CONDITIONAL([OS_WIN32], [test x$windows = xtrue])
AC_MSG_RESULT([$have_windows_mingw])
AC_SUBST(COOT_WINDOWS_CFLAGS)
AC_SUBST(COOT_WINDOWS_LDFLAGS)

])

