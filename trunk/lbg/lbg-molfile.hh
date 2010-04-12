
#ifndef LBG_MOLFILE_HH
#define LBG_MOLFILE_HH

#include "clipper/core/coords.h"

#include "lig-build.hh"


namespace lig_build {

   // throw an exception on unable to convert
   int string_to_int(const std::string &s);
   // throw an exception on unable to convert
   float string_to_float(const std::string &s);
   

   class molfile_atom_t {
   public:
      clipper::Coord_orth atom_position;
      std::string name;
      std::string element;
      bool aromatic;
      bool chiral;
      molfile_atom_t(const clipper::Coord_orth &pos_in,
		     const std::string &element_in,
		     const std::string &name_in) {
	 atom_position = pos_in;
	 element = element_in;
	 name = name_in;
      }
      molfile_atom_t(float x, float y, float z, std::string ele_in) {
	 atom_position = clipper::Coord_orth(x,y,z);
	 element = ele_in;
	 name = ele_in;
      }
   };

   class molfile_bond_t {
   public:
      int index_1;
      int index_2;
      bond_t::bond_type_t bond_type;
      molfile_bond_t(int i1, int i2, bond_t::bond_type_t bt) {
	 index_1 = i1;
	 index_2 = i2;
	 bond_type = bt;
      } 
   };

   class molfile_molecule_t {
   public:
      std::vector<molfile_atom_t> atoms;
      std::vector<molfile_bond_t> bonds;
      void read(const std::string &file_name);
   };
}

#endif // LBG_MOLFILE_HH

