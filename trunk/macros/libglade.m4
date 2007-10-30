# libglade.m4
# 
# Copyright 2007 The University of Oxford
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
 

AC_DEFUN([AM_PATH_LIBGLADE],
[
AC_PROVIDE([AM_PATH_LIBGLADE])

AC_ARG_WITH(libglade-prefix,
[  --with-libglade-prefix=LIBGLADE_PREFIX  Location of libglade],
LIBGLADE_PREFIX="$withval")

echo xxxxxxxxxxxxxxxxxxxxxxxxxx LIBGLADE_PREFIX is $LIBGLADE_PREFIX

if test $coot_gtk2 = TRUE ; then

   AC_MSG_CHECKING(libglade)

   saved_LIBS="$LIBS"
   saved_CXXFLAGS="$CXXFLAGS"


   # Note:  I tried $LIBGLADE_PREFIX/lib/libglade-2.0.la, like Ezra 
   # says is the preferred method now, but libguile test in configure
   # fails to link if I do that:
   # /home/paule/glade-3/lib/libglade-2.0.la: file not recognized: File format not recognized
   # collect2: ld returned 1 exit status
   # 
   # so, go back to old way of -Lxx -lxx

   # 
   if test x$LIBGLADE_PREFIX != x ; then
      LIBGLADE_LIBS="-L$LIBGLADE_PREFIX/lib -lglade-2.0"
      LIBGLADE_CFLAGS="-I$LIBGLADE_PREFIX/include/libglade-2.0 -DUSE_LIBGLADE"
   else
      LIBGLADE_LIBS=-lglade-2.0
      LIBGLADE_CFLAGS="-I/usr/include/libglade-2.0"
   fi
   
   LIBS="$LIBS $LIBGLADE_LIBS $pkg_cv_GTK_LIBS"
   CFLAGS="$CFLAGS $LIBGLADE_CFLAGS $pkg_cv_GTK_CFLAGS -DUSE_LIBGLADE"
   AC_TRY_LINK([#include <glade/glade.h>], [GladeXML *xml = glade_xml_new("x",NULL,NULL);], have_libglade=yes, have_libglade=no)

   CXXFLAGS="$saved_CXXFLAGS"
   LIBS="$saved_LIBS"
   AC_MSG_RESULT($have_libglade)

fi

AC_SUBST(LIBGLADE_CFLAGS)
AC_SUBST(LIBGLADE_LIBS)

])
