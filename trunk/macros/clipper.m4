# Makefile.am
# 
# Copyright 2002 The University of York
# Author: Paul Emsley
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
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
# umtz, cmap, fftw, cctbx, boost and python).  But we do not do that
# here at the moment.
#
# Note that the clipper include files are not in the include directory
# as one would expect from an "installed" package.
# PE 1/2/2002.
#
# AM_PATH_CLIPPER([ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
AC_DEFUN([AM_PATH_CLIPPER],
[
AC_PROVIDE([AM_PATH_CLIPPER])


AC_ARG_WITH(clipper-prefix, [  --with-clipper-prefix=PFX Prefix where Clipper has been built],
 clipper_prefix="$withval",
 clipper_prefix="")


saved_LIBS="$LIBS"
saved_CFLAGS="$CFLAGS"

if test x$clipper_prefix != x; then
	# very likely the majority of cases, we will try to configure with:
	# --clippper-prefix=/some/thing
	#

 # should ideally be CLIPPER_CFLAGS="-I$clipper_prefix/include", and the like
 # when clipper and dependencies get installed.
 #  
 CLIPPER_CXXFLAGS="-I$clipper_prefix/include"
# -I$clipper_prefix/cctbx
 
 # yes, libmmtz.a is in -L$clipper_prefix/umtz!
 #
 # Note, I do not know how the boost libs work, so I do not include them.
 # FIXME?
 
 # HACK! FIXME
 # added lz, we should have proper autoconf check for this.
 #
 CLIPPER_LDOPTS="-L$clipper_prefix/lib -lclipper-mtz -lclipper-cif -lclipper-phs -lclipper-contrib -lclipper-mmdb -lclipper-mmdbold -lclipper-core -lmccp4 $MMDB_LIBS -lrfftw -lfftw -lz -lm"
 CLIPPER_LDOPTS="-L$clipper_prefix/lib -lclipper-ccp4 -lclipper-cif -lclipper-phs -lclipper-contrib -lclipper-mmdb -lclipper-core -lccp4c $MMDB_LIBS -lrfftw -lfftw -lz -lm"
# -L$clipper_prefix/boost/lib -lclipper-cctbx -L$clipper_prefix/cctbx/lib -lsgtbx -luctbx 
else
 # the compiler looks in the "standard" places for clipper.  In real life,
 # it would be quite unlikely that clipper would be installed in /usr/include, 
 # /usr/lib etc. so this code will not usually find the right dependencies.
 CLIPPER_CXXFLAGS=""
 CLIPPER_LDOPTS="-lclipper-mtz -lclipper-cif -lclipper-phs -lclipper-contrib -lclipper-mmdb -lclipper-mmdbold -lclipper-core -lmccp4 $MMDB_LIBS -lrfftw -lfftw -lz -lm"
fi

AC_MSG_CHECKING([for Clipper])

	LIBS="$save_LIBS $CLIPPER_LDOPTS"
	CFLAGS="$save_CFLAGS $CLIPPER_CXXFLAGS"
	# AC_TRY_LINK uses the c compiler, so we will temporarily 
	# reassign $CC to the c++ compiler.
 	#
	CC_save="$CC"
	CC="$CXX $CXXFLAGS"
	AC_TRY_LINK([#include "clipper/core/xmap.h"] ,[ clipper::Xmap<float> a; ], have_clipper=yes, have_clipper=no)
	CC="$CC_save"
	AC_MSG_RESULT($have_clipper)

if test x$have_clipper = xyes; then

 LIBS="$saved_LIBS"
 CFLAGS="$saved_CFLAGS"
 CLIPPER_LIBS="$CLIPPER_LDOPTS"
 ifelse([$1], , :, [$1])

else

 LIBS="$saved_LIBS"
 CFLAGS="$saved_CFLAGS"
 CLIPPER_LIBS=""
 CLIPPER_CXXFLAGS=""
 ifelse([$2], , :, [$2])

fi

AC_SUBST(CLIPPER_CXXFLAGS)
AC_SUBST(CLIPPER_LIBS)

])
