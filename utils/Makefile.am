# coot-utils/Makefile.am
# 
# Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 The University of York
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

AM_CPPFLAGS = \
	-I$(top_srcdir)

AM_CXXFLAGS = $(COOT_WINDOWS_CFLAGS) \
	$(GLOB_CFLAGS)		     \
	-DPKGDATADIR='"$(pkgdatadir)"'

lib_LTLIBRARIES=libcoot-utils.la

pkginclude_HEADERS = coot-utils.hh coot-fasta.hh
pkgincludedir = $(includedir)/coot/utils


# for simple things:
libcoot_utils_la_SOURCES = coot-utils.cc coot-fasta.cc atomic-numbers.cc \
	gdk-keysyms.cc hsv-rgb.cc

libcoot_utils_la_LIBADD = \
	$(GLOB_LIBS)

libcoot_utils_la_LDFLAGS = $(SHARED_LDFLAGS)
