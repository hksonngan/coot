
%module coot
%{
#include <cstdio>
#include "globjects.h"  //includes gtk/gtk.h
#include "coot-utils/coot-coord-utils.hh"
#include "c-interface.h"
#include "c-interface-preferences.h"
#include "c-interface-database.hh"
#include "c-interface-python.hh"
#include "c-interface-ligands-swig.hh"
#include "c-interface-mogul.hh"
#include "c-interface-sequence.hh"
#include "cc-interface.hh"
#include "cc-interface-scripting.hh"
#include "cc-interface-ncs.hh"
#include "manipulation-modes.hh"
#include "rotamer-search-modes.hh"
#include "lbg-interface.hh"
#include "sdf-interface.hh"
#include "probe-clash-score.hh"
#include "cmtz-interface.hh"
#include "coot-version.hh"
#include "get-monomer.hh"
#include "globularize.hh"
%}


%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"

namespace std {
%template(vector_string) vector<std::string>;
%template(pairbf) pair<bool, float>;
%template(IntVector) vector<int>;
%template(vector_atom_spec)      vector<coot::atom_spec_t>;
%template(vector_mtz_type_label) vector<coot::mtz_type_label>;
}


#include "globjects.h"  //includes gtk/gtk.h
#include "coot-utils/coot-coord-utils.hh"
/* actually we should ignore all GtkWidgets or similar.... */
%ignore main_window();
%ignore main_menubar();
%ignore main_statusbar();
%ignore main_toolbar();
/* conflicts with redefinition */
%ignore list_nomenclature_errors(int);

%include "c-interface.h"
%include "c-interface-preferences.h"
%include "cc-interface-ncs.hh"
%include "cc-interface.hh"
%include "cc-interface-scripting.hh"
%include "c-interface-database.hh"
%include "c-interface-python.hh"
%include "c-interface-ligands-swig.hh"
%include "c-interface-mogul.hh"
%include "c-interface-sequence.hh"
%include "cmtz-interface.hh"
%include "manipulation-modes.hh"
%include "rotamer-search-modes.hh"
%include "lbg-interface.hh"
%include "sdf-interface.hh"
%include "coot-utils/residue-and-atom-specs.hh" // for atom_spec_t
%include "probe-clash-score.hh"
%include "coot-version.hh"
%include "get-monomer.hh"
%include "globularize.hh"
%include "../coords/phenix-geo.hh"
