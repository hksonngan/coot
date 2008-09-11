/* mini-mol/min-mol.cc
 * 
 * Copyright  2003, 2004, 2006, 2007 The University of York
 * Author: Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef HAVE_MINIMOL
#define HAVE_MINIMOL

#include <iostream>
#include <string>
#include <vector>

#include "mmdb_manager.h"
#include "clipper/core/coords.h"

namespace coot {

   namespace minimol {

      class zone_info_t {
      public:
	 bool is_simple_zone;
	 std::string chain_id;
	 int resno_1;
	 int resno_2;
	 zone_info_t() { is_simple_zone = 0; }
	 zone_info_t(const std::string &chain_id_in, int r1, int r2) {
	    is_simple_zone = 1;
	    chain_id = chain_id_in;
	    resno_1 = r1;
	    resno_2 = r2;
	 }
      };

      class atom { 
      public:
	 atom(std::string atom_name, std::string ele, float x, float y, float z, const std::string &altloc);
	 atom(std::string atom_name, std::string ele, const clipper::Coord_orth &pos_in, const std::string &altloc);
	 atom(std::string atom_name, std::string ele, const clipper::Coord_orth &pos_in, const std::string &altloc, float occupancy);
	 atom() {}
	 std::string altLoc;
	 float occupancy;
	 clipper::Coord_orth pos;
	 std::string name;
	 std::string element; // " H", " N", " O" etc.
	 bool is_hydrogen_p() const;
	 friend std::ostream&  operator<<(std::ostream&, atom);
      };

      class residue { 
      public:
	 residue(int i){ seqnum = i;}
	 residue(int i, const std::string &resname) {
	    seqnum = i;
	    name = resname;}
	 residue(const CResidue *residue_p);
	 residue(){}; // for resizing the residues in fragment
	 int seqnum;
	 std::string name;
	 std::vector<atom> atoms;
	 // void operator=(const coot::minimol::residue &res_in);
	 const atom& operator[](int i) const {return atoms[i];}
	 const atom& operator[](const std::string &atname) const; // look it up, return atom 
                                            	 // with name "FAIL" if the atom is not there.
	 atom&       operator[](int i) {return atoms[i];}
	 void addatom(std::string atom_name, std::string element,
		      float x, float y, float z, const std::string &altloc);
	 void addatom(std::string atom_name, std::string element,
		      const clipper::Coord_orth &pos, const std::string &altloc);
	 void addatom(const atom &at); 
	 friend std::ostream&  operator<<(std::ostream&, residue);
	 int n_atoms() const { return atoms.size(); }
	 std::vector<atom *> select_atoms_serial() const;
	 void delete_atom_indices(const std::vector<int> &atom_indices);
      };

      class fragment {
	 int residues_offset; // offset is 0 when we start at 1.  //
         	              //  and -2 when we start at -1 (etc).
         	              //  operator[] takes into account
         	              //  residues_offset now.
	                      // Basically offset is the (minimum_resno) + 1.
	 // The resizing in operator[] and addresidue() needs some clear thinking.
      public:
	 fragment() {
	    residues_offset = 0;
	    residues.resize(1, residue(1)); }
	 fragment(const std::string &frag_id_in) {
	    fragment_id = frag_id_in;
	    residues_offset = 0;
	    residues.resize(1, residue(1));
	 }
	 std::string fragment_id;
	 std::vector<residue> residues;
	 friend std::ostream&  operator<<(std::ostream&, fragment);
	 const residue& operator[](int i) const {
	    int itmp = residues.size() + residues_offset;
	    if (i>= itmp) {
	       std::cout << "ERROR can't resize const residues: request for " << i
			 << " with residues size: " << residues.size()
			 << " and offset: " << residues_offset << std::endl; 
	    } 
	    return residues[i-residues_offset];
	 }
	 residue&       operator[](int i);
	 void addresidue(const residue &res, short int add_if_empty_flag);
	 std::vector<atom *> select_atoms_serial() const;
	 int max_residue_number() const { return (residues.size() + residues_offset -1); }
	 int min_res_no() const { return residues_offset + 1; }
	 int n_filled_residues() const;
	 int resize_for(int nres, int min_resno);
	 void check() const;
	 clipper::Coord_orth midpoint() const;
	 // transform all coordinates in the fragment by rtop:
	 void transform(const clipper::RTop_orth &rtop);
      };

      class molecule {
	 // Return status.  If good, return 0 else (if bad) return 1.
	 // 
	 short int setup(CMMDBManager *mmdb_mol_in);
	 short int have_spacegroup;
	 short int have_cell;
      public:
	 molecule() {have_cell = 0; have_spacegroup = 0;};
	 // 
	 // residue_type is usually, "HOH" or "DUM".
	 molecule(const std::vector<clipper::Coord_orth> &atom_list,
		  const std::string &residue_type, std::string atom_name,
		  std::string chain_id);
	 molecule(CMMDBManager *mmdb_mol_in);
	 short int init(CMMDBManager *mmdb_mol_in) {return setup(mmdb_mol_in);}

	 // for setting the mmdb cell and symm
	 std::string mmdb_spacegroup;
	 std::vector<float> mmdb_cell;
	 // this is mmdb, use the cell angles in degrees.
	 void set_cell(float a[6]);
	 // cell angles in degrees:
	 void set_cell(std::vector<realtype> c);
	 void set_cell(const clipper::Cell &cell);
	 void set_spacegroup(const std::string &spacegroup);

	 // add arbitary atom somewhere.
	 void addatom(const std::string &chain_id_in, int resno, const atom &at,
		      short int is_water_flag);

	 // Set all the atoms that match the given name to the given occupancy.
	 // 
	 // Return the number of atoms adjusted
	 // 
	 int set_atom_occ(const std::string &atom_name, float occ);
	 
	 std::string name;
	 std::vector<fragment> fragments;

	 // We create (with new) a full mmdb CMMDBManager and pass
	 // back the pointer to it.  You are responsible for deleting
	 // it.
	 //
	 // Note that the b-factor is not an attribute of a minimol
	 // atom, so we need to pass it. 
	 // 
	 PCMMDBManager pcmmdbmanager(float b_factor_new_atoms) const;
	 void delete_molecule();
	 const fragment& operator[](int i) const {return fragments[i];}
	 fragment&       operator[](int i)       {return fragments[i];}
	 
	 // if chain_id is not amongst the set of chain ids that we have already,
	 // then push back a new fragment and return its index.
	 //
	 int fragment_for_chain(const std::string &chain_id);
	 friend std::ostream&  operator<<(std::ostream&, molecule);
	 // return 0 on success.
	 int read_file(std::string pdb_filename); // use mmdb to read.
	 // return 0 on success
	 int write_file(std::string pdb_filename, float new_atom_b_factor) const; // use mmdb to write.
	 // possibly expensive/large return value:
	 std::vector<atom *> select_atoms_serial() const;
	 // ditto
	 molecule molecule_of_atom_types(std::string at_type) const;

	 // Can we use fragments[0]?
	 // 
	 bool is_empty() const;

	 // Don't use the atomic weight.  I.e. all atoms are equally weighted.
	 // FIXME
	 // 
	 clipper::Coord_orth centre() const; // return centre of molecule

	 // So that we can write to "W" if it doesn't already exist in (another)
	 // pdb:
	 std::string unused_chain_id(const std::string &pref_chain) const;

	 // return success status, 0 is fail
	 // 
	 short int set_cell_symm(const coot::minimol::molecule &mol);
	 
	 // return a vector of cell, vector of length 0 if no cell
	 // 
	 std::vector<float> get_cell() const;
	 std::string get_spacegroup() const;

	 void transform(const clipper::RTop_orth &rtop);

	 void check() const;
	 int count_atoms() const;

	 molecule fragmentize() const;

	 // Can this molecule be described as a simple zone?  If so,
	 // return the parameters.
	 zone_info_t zone_info() const;

      };
      std::ostream& operator<<(std::ostream& s, coot::minimol::atom at);
      std::ostream& operator<<(std::ostream& s, coot::minimol::residue res);
      std::ostream& operator<<(std::ostream& s, coot::minimol::fragment frag);
   }

}

/* Need this construction?
   for(int ifrag=0; ifrag<fragments.size(); ifrag++) {
      for(int ires=(*this)[ifrag].min_res_no(); ires<=(*this)[ifrag].max_residue_number(); ires++) {
	 for (int iat=0; iat<(*this)[ifrag][ires].atoms.size(); iat++) {
	    std::cout << " " << (*this)[ifrag].fragment_id << " " << (*this)[ifrag][ires]
		      << " " << (*this)[ifrag][ires][iat].name
		      << " " << (*this)[ifrag][ires][iat].pos.format() << std::endl;
	 }
      }
   }
*/

#endif // HAVE_MINIMOL
