# geometry/Makefile.am
# 
# Copyright 2002, 2003 The University of York
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

lib_LTLIBRARIES=libcoot-geometry.la

EXTRA_DIST = notes Makefile.simple

pkginclude_HEADERS =  protein-geometry.hh read-sbase.hh lbg-graph.hh srs-interface.hh \
   dict-mismatches.hh hb-types.hh

pkgincludedir = $(includedir)/coot/geometry

AM_CPPFLAGS = -I$(top_srcdir) \
	 $(CLIPPER_CXXFLAGS) $(CCP4SRS_CXXFLAGS) $(MMDB_CXXFLAGS)

AM_CXXFLAGS = \
	-DDATADIR='"$(datadir)"'       \
	-DPKGDATADIR='"$(pkgdatadir)"' \
	$(COOT_WINDOWS_CFLAGS)


libcoot_geometry_la_SOURCES = protein-geometry.cc protein-geometry-mmdb.cc \
    read-sbase.cc energy-lib.cc 	 \
    chem_mods.cc \
    lbg-graph.cc  dreiding.cc dict-utils.cc cif-parse.cc

libcoot_geometry_la_LIBADD = \
        $(top_builddir)/mini-mol/libcoot-mini-mol.la \
        $(top_builddir)/utils/libcoot-utils.la       \
        $(CLIPPER_LIBS) $(CCP4SRS_LIBS) $(MMDB_LIBS)

libcoot_geometry_la_LDFLAGS = $(SHARED_LDFLAGS)

# libcoot_geometry_la_CXXFLAGS = -g -O1

check_PROGRAMS = test-geometry test-read-components test-ccp4srs 

bin_PROGRAMS = \
	coot.compare-dictionaries          \
	coot.make-shelx-restraints         \
	coot.dictionary-bond-distributions \
	coot.available-comp-id

test_geometry_SOURCES = test-geometry.cc

test_ccp4srs_SOURCES = test-ccp4srs.cc

test_read_components_SOURCES = test-read-components.cc test-read-components.hh

test_ccp4srs_LDADD = ./libcoot-geometry.la    

test_geometry_LDADD = ./libcoot-geometry.la

test_read_components_LDADD = ./libcoot-geometry.la

coot_dictionary_bond_distributions_SOURCES = dictionary-bond-distributions.cc

coot_dictionary_bond_distributions_LDADD = ./libcoot-geometry.la

coot_compare_dictionaries_SOURCES = compare-dictionaries.cc

coot_compare_dictionaries_LDADD = ./libcoot-geometry.la 

coot_make_shelx_restraints_SOURCES = make-shelx-restraints.cc

coot_make_shelx_restraints_LDADD = ./libcoot-geometry.la 

coot_available_comp_id_SOURCES = available-comp-id.cc

coot_available_comp_id_LDADD = ./libcoot-geometry.la 

