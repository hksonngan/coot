dnl configure.in
dnl 
dnl Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 The University of York
dnl Copyright 2008 The University of Oxford
dnl Author: Paul Emsley
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or (at
dnl your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
dnl 02110-1301, USA
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
dnl
dnl Process this file with autoconf to produce a configure script.

AC_INIT(coot, 0.8.2-pre)
AC_CONFIG_SRCDIR(src/main.cc)
AM_INIT_AUTOMAKE

dnl Pick up the macros.
AM_ACLOCAL_INCLUDE(macros)
AC_CONFIG_MACRO_DIR(macros)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AC_PROG_CXX
# AM_PROG_LIBTOOL that's old style. Now we use LT_INIT
#LT_INIT
# AC_PROG_INTLTOOL, no, not today.  Mac compilation problems, no perl/ XML or something.
# BL says Lets do some testing and use old style if needed
AC_MSG_CHECKING([which libtool initialization strategy to adopt])
AC_MSG_RESULT([m4_ifset([LT_INIT],[LT-INIT],[AC-PROG-LIBTOOL])])
m4_ifset([LT_INIT],
[LT_INIT],
[AC_PROG_LIBTOOL])

GNOME_COMPILE_WARNINGS
GNOME_CXX_WARNINGS

AC_CHECK_HEADERS([sys/stdtypes.h], [COOT_STDTYPES="-DHAVE_SYS_STDTYPES_H=1"], [COOT_STDTYPES="-DHAVE_SYS_STDTYPES_H=0"])

dnl AC_OPENMP
AX_OPENMP

AC_LIB_PREPARE_MULTILIB

AC_SUBST(COOT_STDTYPES)

EXPORT_DYNAMIC_FLAG_SPEC=$export_dynamic_flag_spec
AC_SUBST(EXPORT_DYNAMIC_FLAG_SPEC)


dnl We need to put a PKG_CHECK_MODULES in early so that pkg-config is
dnl setup first (it seems to be a side-effect of using PKG_CHECK_MODULES), 
dnl setting ac_cv_env_PKG_CONFIG and related. This is because the conditional
dnl  use of PKG_CHECK_MODULES for GPP4 causes problems when setting up pkg-config
dnl
dnl So, let's check GLIB here, should be harmless.
dnl
PKG_CHECK_MODULES([GLIB], [glib-2.0])


dnl test for WINDOWS_MINGW
AM_MINGW_WINDOWS
dnl test for LIBPNG (Windows only - maybe shouif here rather than in macro?)
if test "$have_windows_mingw" = yes; then
  AM_PATH_GLOB
fi

AM_PATH_LIBPNG

AM_PATH_SQLITE3

dnl =========================== crystallographic libraries ================================
dnl Check for gpp4

AC_ARG_WITH([gpp4], AS_HELP_STRING([--with-gpp4], [Build with the gpp4 library (default is NO)]))

if test "x$with_gpp4" = "xyes"; then
    PKG_CHECK_MODULES([CCP4], [gpp4])
fi

dnl Set platform-specific compilation flags - derived from clipper's configure
test "${target:+set}" = set || target="$host_os"
case "$target" in
   *linux* ) ;;
   *irix* )
	if test "`basename $CXX`" = CC; then
	   case "$CXXFLAGS" in 
	      *LANG::std* ) ;; 
              * )
		  CXXFLAGS="$CXXFLAGS -LANG:std -ptused"
           esac
           # This patches libtool... This change exists in libtool 1.4d and
           # is required such that -LANG: is not filtered out...
           AC_MSG_NOTICE([Patching libtool for irix CC])
           sed "s:no/\*-\*-irix\*):no/*-*-irix* | /*-*-irix*):" libtool > libtool.tp
           mv libtool.tp libtool
           chmod +x libtool
	fi
    ;;
esac           


dnl Check for MMDB.
AM_PATH_MMDB2(, [echo Error locating necessary mmdb; exit 1],,)

AM_WITH_SSM
AM_WITH_CCP4SRS

AM_SINGLE_FFTW2

dnl Check for Clipper.  we exit if clipper is not found.
#
AM_PATH_CLIPPER(, [echo Error locating Clipper - a CNS-enabled version of Clipper \(2.1-090520-ac or later\) is required now; exit 1],AC_MSG_ERROR([Cannot find Clipper]))

# we set default coot_guile to true
coot_guile=true
AM_WITH_GUILE(, [echo optional guile\?],,)
if test x$coot_guile = xtrue ; then
   GUILE_FLAGS
   AM_GUILE_LIB
   # guile-gui depends on a gui interface being present.  If it is, force check for guile-gui
   if test x$with_guile_gtk != x ; then
      AM_GUILE_GUI
   fi
   GUILE_VERSION=$($ac_cv_path_GUILE_CONFIG --version  2>&1 | awk '{split($NF, arr, "[[.]]");  print arr[[1]] "." arr[[2]]}')
   echo checking GUILE_VERSION: $GUILE_VERSION
   AC_SUBST(GUILE_VERSION)
fi


dnl =========================== GSL ================================

dnl test for GSL
# min_gsl_version=1.3
AM_PATH_GSL(1.3,[COOT_GSL_FLAGS=-DHAVE_GSL], [COOT_GSL_FLAGS=""])
AC_SUBST(COOT_GSL_FLAGS)

dnl =========================== GTK, GNOMECANVAS ===========================

coot_gtk2=TRUE

# AM_PATH_GTK(2.4.14)
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4.12)
PKG_CHECK_MODULES(GNOME_CANVAS, libgnomecanvas-2.0 >= 0.15.0)
if test -n "$GNOME_CANVAS_CFLAGS" ; then 
   GNOME_CANVAS_CFLAGS="-DHAVE_GNOME_CANVAS $GNOME_CANVAS_CFLAGS"
fi
AC_SUBST(GNOME_CANVAS_CFLAGS)

AM_PATH_GTKGLEXT_1_0(1.0.0,,[exit 1]) 
MAKE_COOTRC_CMD="sed -e 's/\#GTK2 //' "
AC_SUBST(MAKE_COOTRC_CMD)


dnl ============================ OPENGL ===================
#
if test "$have_windows_mingw" = yes; then
  AC_CHECK_HEADER([GL/gl.h], [], AC_MSG_ERROR([GL header not found]))
  AC_CHECK_LIB(opengl32, main, COOT_OPENGL_LIBS="-lopengl32", AC_MSG_ERROR([GL lib required])) 
else
  AC_CHECK_LIB(GL, glLineWidth, COOT_OPENGL_LIBS="-lGL")
fi
AC_SUBST([COOT_OPENGL_LIBS], [${COOT_OPENGL_LIBS}])

dnl =========================== GLUT ================================

dnl Check for GLUT
#
AM_PATH_GLUT(, [echo You need the GLUT utility library; exit 1], AC_MSG_ERROR([Cannot find proper GLUT version]))


dnl =========================== Python and Guile ======================

AM_WITH_PYTHON
AM_PATH_PYTHON

AM_WITH_GUILE_GTK
AM_PATH_GUILE_GTK

AX_BOOST_BASE
AX_BOOST_PYTHON

# So now BOOST_PYTHON_LIB is set (and that is typically "boost_python").
# How do we use that?  At the moment we want to only use 
# -l$(BOOST_PYTHON_LIB) when we have RDKIT_LIBS set

if test -z "$RDKIT_LIBS" ; then
   :
else 
   if test -z "$BOOST_PYTHON_LIB" ; then
      :
   else
      L_BOOST_PYTHON=-l$BOOST_PYTHON_LIB
   fi
fi
AC_SUBST(L_BOOST_PYTHON)

dnl =========================== GLADE ================================

# libglade:

# AM_PATH_LIBGLADE, problems in glade-3 include files not including
# declaration of GladeXML and other things.
AM_PATH_LIBGLADE

dnl =========================== GooCanvas ================================

AM_PATH_GOOCANVAS
# give something for the preprocessor to chew on...
if test "$GOOCANVAS_CFLAGS" != "" ; then
   GOOCANVAS_CFLAGS="-DHAVE_GOOCANVAS $GOOCANVAS_CFLAGS"
fi

AC_ARG_WITH(enhanced-ligand-tools, [ --with-enhanced-ligand-tools with optional complicated/experimental ligand-related dependencies ],
 coot_enhanced_ligand_additions="true",
 coot_enhanced_ligand_additions="false")

if test x$coot_enhanced_ligand_additions = xtrue ; then
   ENHANCED_LIGAND_TOOLS_CFLAGS="-DMAKE_ENHANCED_LIGAND_TOOLS"
else 	
   ENHANCED_LIGAND_TOOLS_CFLAGS=""
fi

AM_CONDITIONAL(MAKE_ENHANCED_LIGAND_TOOLS, test x$coot_enhanced_ligand_additions = xtrue)
AC_SUBST(ENHANCED_LIGAND_TOOLS_CFLAGS)

AC_SUBST(RDKIT_CXXFLAGS)
AC_SUBST(RDKIT_LIBS)

AC_SUBST(COOT_BUILD_INFO_STRING)


# #defines for SWIG:
#
# SWIG_FLAGS is something of a misnomer.  It actually defines
# COOT_USE_GTK2_INTERFACE, which is needed to correctly declare the
# correct callbacks in both GTK1 and GTK2 regimes.
#
if test $coot_gtk2 = TRUE ; then
  SWIG_FLAGS="-DCOOT_USE_GTK2_INTERFACE"
else 
  # the creation of coot.py in the src dir (currently) relies on
  # SWIG_FLAGS being blank for the GTK1 version.
  SWIG_FLAGS=""
fi
dnl echo ....... SWIG_FLAGS=$SWIG_FLAGS
AC_SUBST(SWIG_FLAGS)

AM_CONDITIONAL(USING_GTK2_VERSION, test x$coot_gtk2 = xTRUE)



dnl check for guile modules www and gtk here.

dnl check for GSL
dnl AC_CHECK_LIB(m,main)
dnl AC_CHECK_LIB(gslcblas,main)
dnl AC_CHECK_LIB(gsl,main)


AM_WITH_MYSQL_DATABASE

COOT_CURL

# 20090613 We need to be able to determine the build type, and embed
# that value in to coot binary.  We need to do that for the updating
# Coot.  THe build type will be set by COOT_SYS_BUILD_TYPE.  Which is 
# for now one variable.  I may split it into os/os-ver/python/arch etc 
# later.
AM_COOT_SYS_BUILD_TYPE


dnl Fix this another time.
dnl dnl Add the languages which your application supports here.
dnl ALL_LINGUAS="es es_ES"
dnl # AM_GNU_GETTEXT
dnl GETTEXT_PACKAGE=coot
dnl dnl we need to export ALL_LINGUAS here otherwise it is not expanded
dnl dnl for GTk1 on cycle at least, so that we would get:
dnl dnl No rule to make target `@ALL_LINGUAS@.po', needed by `@ALL_LINGUAS@ .gmo'.
dnl AC_SUBST(ALL_LINGUAS)
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AM_GLIB_GNU_GETTEXT

dnl dnl For now: just include the declarations for config.h.in
dnl AH_TEMPLATE([ENABLE_NLS], [Define if NLS support is used])
dnl AH_TEMPLATE([PACKAGE_LOCALE_DIR], [Directory of locales])


dnl Set PACKAGE_LOCALE_DIR in config.h.
# if test "x${prefix}" = "xNONE"; then
  # AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale")
# else
  # AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale")
# fi

AC_OUTPUT([
Makefile
macros/Makefile
pdb-templates/Makefile
scheme/Makefile
python/Makefile
compat/Makefile
utils/Makefile
coot-utils/Makefile
protein_db/Makefile
ccp4mg-utils/Makefile
coords/Makefile
db-main/Makefile
density-contour/Makefile
surface/Makefile
coot-surface/Makefile
angles/Makefile
mini-mol/Makefile
skeleton/Makefile
ligand/Makefile
geometry/Makefile
ideal/Makefile
build/Makefile
analysis/Makefile
high-res/Makefile
tw/Makefile
cootaneer/Makefile
cootilus/Makefile
greg-tests/Makefile
python-tests/Makefile
lidia-core/Makefile
libgtkgraph/Makefile
goograph/Makefile
lbg/Makefile
doc/Makefile
src/Makefile
src/coot
python/coot_load_modules.py
data/Makefile
pixmaps/Makefile
pixmaps/icons/Makefile
rama-data/Makefile
pyrogen/Makefile
pyrogen/pyrogen
auxil/Makefile
auxil/catcheck/Makefile	
auxil/rama/Makefile	
auxil/sec-str/Makefile	
auxil/strand-analysis/Makefile	
auxil/vectors/Makefile
rcrane/Makefile
])
# removed setup/Makefile
# po/Makefile.in
# lbg/Makefile

# GETTEXT related things: When AM_GNU_GETTEXT get enabled, these get enabled.
# po/Makefile
# intl/Makefile - sigh.  Fix this later, not now.


# or maybe:
#],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])

