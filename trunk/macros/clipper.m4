# Makefile.am
# 
# Copyright 2002 The University of York
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

# This version of clipper.m4 depends absolutely on you having used
# Kevin's install.sh 
# (from http://www.ysbl.york.ac.uk/~cowtan/clipper/clipper.html) to get 
# the clipper dependences and put them in place.
#
# We can in future test each of the dependences, (currently 
# fftw, cctbx, boost and python).  But we do not do that
# here at the moment.
#
# Note that the clipper include files are not in the include directory
# as one would expect from an "installed" package.
# PE 1/2/2002.
#
# AM_PATH_CLIPPER([ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
#
AC_DEFUN([AM_PATH_CLIPPER],
[
AC_PROVIDE([AM_PATH_CLIPPER])


AC_MSG_CHECKING([for Clipper])

if ${PKG_CONFIG} clipper ; then 
   CLIPPER_CXXFLAGS="$($PKG_CONFIG --cflags clipper)"
   CLIPPER_LIBS="$($PKG_CONFIG --libs clipper)"
   coot_found_clipper=yes
else
   coot_found_clipper=no
fi

AC_MSG_RESULT($coot_found_clipper)

if test $coot_found_clipper = no ; then
   AC_MSG_FAILURE([Clipper not found])
fi

AC_SUBST(CLIPPER_CXXFLAGS)
AC_SUBST(CLIPPER_LIBS)

])
