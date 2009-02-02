/* mini-mol/min-mol.cc
 * 
 * Copyright  2003, 2004, The University of York
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

#include <string.h>   // for str(n)cpy
#include <algorithm>  // for sort

#include "mini-mol.hh"
#include "coot-coord-utils.hh"

#include "coot-sysdep.h"

coot::minimol::molecule::molecule(CMMDBManager *mol) {
   setup(mol);
}


coot::minimol::molecule::molecule(const std::vector<clipper::Coord_orth> &atom_list,
				  const std::string& residue_type,
				  std::string atom_name,
				  std::string chain_id) {

   // Constructing a fragment from a chain_id sets residues_offset to 0
   // but doesn't add any residues
   fragments.push_back(chain_id);
   std::string element(" O");

   // Each atom goes in its own residue (residue number offset by one
   // c.f. the atom vector index)
   //
   for (unsigned int i=0; i<atom_list.size(); i++) {
      fragments[0][i+1] = coot::minimol::residue(i+1); // atoms start at 0, residues at 1.
      fragments[0][i+1].name = residue_type;  // not "WAT" says EJD - 030624
      fragments[0][i+1].addatom(atom_name,
				element,
				atom_list[i].x(),
				atom_list[i].y(),
				atom_list[i].z(), std::string(""),
				30.0); // pass this? 20090201
   }
   have_cell = 0;
   have_spacegroup = 0;
}

// Return status.  If good, return 0 else (if bad) return 1.
//
short int
coot::minimol::molecule::setup(CMMDBManager *mol) {

   short int istat = 0;
   if (mol == NULL) {
      std::cout << "ERROR:: NULL molecule in minimol::molecule::setup!\n";
      istat = 1;
   } else { 

      have_cell = 0;
      have_spacegroup = 0;

      // fill molecule etc from mmdb_mol_in;
      if (fragments.size() > 0) {
	 delete_molecule();
      }
      InitMatType();

      int imod = 1;
      // for (int imod=1; imod<=n_models; imod++) {
      if (imod == 1) { 
      
	 CModel *model_p = mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol 
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);

	       // int ifrag = fragment_for_chain(chain_p->GetChainID()); old
	       int ifrag = ichain;
	       std::string fragment_id = chain_p->GetChainID();
	       fragments.push_back(coot::minimol::fragment(fragment_id));
	       ifrag = fragments.size() -1;
	       
	       if (chain_p == NULL) {  
		  // This should not be necessary. It seem to be a
		  // result of mmdb corruption elsewhere - possibly
		  // DeleteChain in update_molecule_to().
		  std::cout << "NULL chain in ... minimol setup" << std::endl;
	       } else { 
		  int nres = chain_p->GetNumberOfResidues();
		  std::pair<short int, int> min_info =
		     coot::util::min_resno_in_chain(chain_p);
		  if (min_info.first) { 
		     fragments[ifrag].resize_for(nres, min_info.second);
		     PCResidue residue_p;
		     CAtom *at;
		     for (int ires=0; ires<nres; ires++) { 
			residue_p = chain_p->GetResidue(ires);
			coot::minimol::residue r(residue_p->seqNum); 
			int n_atoms = residue_p->GetNumberOfAtoms();
			r.name = residue_p->name;

			for (int iat=0; iat<n_atoms; iat++) {
			   at = residue_p->GetAtom(iat);
			   clipper::Coord_orth p(at->x, at->y, at->z);
			   coot::minimol::atom mat(std::string(at->name),
						   std::string(at->element),
						   p,
						   std::string(at->altLoc),
						   at->occupancy,
						   at->tempFactor);
			   r.addatom(mat);
			}
			fragments[ifrag].addresidue(r, 0);
		     }
		  }
	       }
	    }
	 }
      }
      realtype cell[6];
      realtype vol;
      int orthcode;
      mol->GetCell(cell[0], cell[1], cell[2],
		   cell[3], cell[4], cell[5], vol, orthcode);
      mmdb_cell.resize(0);
      for (int i=0; i<6; i++) 
	 mmdb_cell.push_back(cell[i]);
      char *spacegroup_str = mol->GetSpaceGroup();
      have_cell = 1; // always, I guess?

      if (spacegroup_str) {
	 have_spacegroup = 1;
	 mmdb_spacegroup = spacegroup_str;
// 	 std::cout << "INFO:: setup minimol has spacegroup: "
// 		   << mmdb_spacegroup << std::endl;
      } else {
	 std::cout << "INFO:: setup minimol from mol: no spacegroup"
		   << std::endl;
      }
   }
   return istat;
}

coot::minimol::molecule
coot::minimol::molecule::fragmentize() const {

   coot::minimol::molecule m;
   // For each fragment of this molecule, find the regions of
   // consequtive residues.
   //
   // Where there is a break, end this fragment, copy all the residues
   // in that segment to the running fragment and the running fragment
   // to m (the returned fragmentized molecule).
   // 
   for (unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {

      coot::minimol::fragment f;
      int UNSET = -9999;
      int fragment_start_resno       = UNSET;
      int fragment_running_end_resno = UNSET;
      int mr = (*this)[ifrag].max_residue_number();
      int min_r = (*this)[ifrag].min_res_no();
      for (int ires=min_r; ires<=mr; ires++) {
// 	 std::cout << "residue " << ires << " has "
// 		   << (*this)[ifrag][ires].atoms.size()
// 		   << " atoms" << std::endl; 
	 if ((*this)[ifrag][ires].atoms.size() == 0) {
	    // we have found a residue with no atoms.
	    // So this is this is the end of a fragment/segment.
	    //
	    // So, if there were residues in this fragment, copy all
	    // the residues in the segment range to f;
	    if (fragment_start_resno <= fragment_running_end_resno && 
		fragment_start_resno != UNSET &&
		fragment_running_end_resno != UNSET) {
	       for(int i=fragment_start_resno; i<=fragment_running_end_resno; i++) {
		  f.addresidue((*this)[ifrag][i], 0);
// 		  std::cout << " fragend: after addresidue f's residue " << i << " has "
// 			    << f[i].atoms.size() << " atoms" << std::endl;
	       }
	       f.fragment_id = (*this)[ifrag].fragment_id;
	       m.fragments.push_back(f);
	       f = coot::minimol::fragment(); 
	       fragment_running_end_resno = UNSET; 
	       fragment_start_resno       = UNSET;
// 	       std::cout << "at resno " << ires << " fragment_start_resno set to "
// 			 << fragment_start_resno << std::endl;
// 	       std::cout << "at resno " << ires << " fragment_running_end_resno to "
// 			 << fragment_running_end_resno << std::endl;
	    }
	 } else {
	    // it *does* have atoms, the normal case.
	    if (fragment_start_resno == UNSET) {
	       fragment_start_resno = ires;
// 	       std::cout << "at Resno " << ires << " fragment_start_resno set to "
// 			 << fragment_start_resno << std::endl;
	    }
	    fragment_running_end_resno = ires;
// 	    std::cout << "At Resno " << ires << " fragment_running_end_resno to "
// 		      << fragment_running_end_resno << std::endl;
	    if (ires == mr) { // the last residues in the chain, so push
			    // back any running partial fragment
	       f.fragment_id = (*this)[ifrag].fragment_id;
	       for(int i=fragment_start_resno; i<=fragment_running_end_resno; i++) {
		  f.addresidue((*this)[ifrag][i], 0);
// 		  std::cout << " chainend: after addresidue f's residue " << i << " has "
// 			    << f[i].atoms.size() << " atoms" << std::endl;
	       }
	       m.fragments.push_back(f);
	       f = coot::minimol::fragment(); 
	    }
	 }
      }
   }
   std::cout << "INFO:: Fragmentize input  had " << fragments.size() << " fragments\n"
	     << "INFO:: Fragmentize output had " << m.fragments.size() << " fragments\n";

   for (unsigned int ifrag=0; ifrag<m.fragments.size(); ifrag++) {
//       std::cout << "DEBUG:: output mol fragment " << ifrag << " " 
// 		<< m[ifrag].fragment_id << " from "
// 		<< m[ifrag].min_res_no() << " to " << m[ifrag].max_residue_number()
// 		<< ", " << m[ifrag].residues.size() << " residues" << std::endl;
//       for (int ires=m[ifrag].min_res_no(); ires<=m[ifrag].max_residue_number(); ires++)
// 	 std::cout << "DEBUG:: output residue " << m[ifrag].fragment_id << " "
// 		   << ires << " has " << m[ifrag][ires].atoms.size()
// 		   << " atom " << std::endl;
   }
   return m;
}

int
coot::minimol::fragment::resize_for(int nres, int min_resno) {

   int ires = nres;

   residues.resize(nres + 1);
   residues_offset = min_resno - 1;

   return ires;

}

void
coot::minimol::molecule::set_cell(float a[6]) {

   mmdb_cell = std::vector<float>(6);
   for (int i=0; i<6; i++)
      mmdb_cell[i] = a[i];
   have_cell = 1;
}

void
coot::minimol::molecule::set_cell(const clipper::Cell &cell) { 

   mmdb_cell = std::vector<float>(6);
   mmdb_cell[0] = cell.a();
   mmdb_cell[1] = cell.b();
   mmdb_cell[2] = cell.c();
   mmdb_cell[3] = clipper::Util::rad2d(cell.alpha());
   mmdb_cell[4] = clipper::Util::rad2d(cell.beta());
   mmdb_cell[5] = clipper::Util::rad2d(cell.gamma());
   have_cell = 1;
}

void
coot::minimol::molecule::set_cell(std::vector<realtype> c) {

   if (c.size() == 6) {
      have_cell = 1; 
      mmdb_cell = std::vector<float>(6);
      for (int i=0; i<6; i++)
	 mmdb_cell[i] = c[i];
   } 
} 

void
coot::minimol::molecule::set_spacegroup(const std::string &spacegroup_in) {
   mmdb_spacegroup = spacegroup_in;
   have_spacegroup = 1;
} 


// return 0 on success.
int
coot::minimol::molecule::read_file(std::string pdb_filename) {

   CMMDBManager mol;
   int ierr = mol.ReadCoorFile((char *)pdb_filename.c_str());
   if (ierr) {
      std::cout << "There was an error reading " << pdb_filename << ". \n";
      std::cout << "ERROR " << ierr << " READ: "
		<< GetErrorDescription(ierr) << std::endl;
      int  error_count;
      char error_buf[500];
      mol.GetInputBuffer(error_buf, error_count);
      if (error_count >= 0) { 
	 std::cout << "         LINE #" << error_count << "\n     "
	      << error_buf << std::endl << std::endl;
      } else {
	 if (error_count == -1) { 
	    std::cout << "       CIF ITEM: " << error_buf << std::endl << std::endl;
	 }
      }
   } else {
      setup(&mol);
   }
   return ierr;
}

// return 0 on success.
int
coot::minimol::molecule::write_file(std::string pdb_filename, float atoms_b_factor) const {
   
   // std::cout << "\nDEBUG:: write_file " << pdb_filename << std::endl;
   PCMMDBManager newmol = pcmmdbmanager();

   int ierr = newmol->WritePDBASCII((char *)pdb_filename.c_str());
   // std::cout << "DEBUG:: write_file " << pdb_filename << " done\n " << std::endl;
   delete newmol;
   return ierr;
}

// if chain_id is not amongst the set of chain ids that we have already,
// then push back a new fragment and return its index.
int
coot::minimol::molecule::fragment_for_chain(const std::string &chain_id) {

   int imatch = -1; // no real fragment -1
   
   for (unsigned int i=0; i<fragments.size(); i++) {
//       std::cout << "comparing :" << fragments[i].fragment_id << ": with :" 
// 		<< chain_id << ":" << std::endl;
      if (fragments[i].fragment_id == chain_id) {
	 imatch = i;
	 break;
      }
   }
   if (imatch == -1) {
      fragments.push_back(coot::minimol::fragment(chain_id));
      imatch = fragments.size() -1;  // the top filled molecule
//       std::cout << "creating new fragment - index: " << imatch << " for chain_id "
// 		<< chain_id << std::endl; 
   }
   return imatch;
}

// Make a residue from a CResidue *:
// 
coot::minimol::residue::residue(const CResidue* residue_p) {

   CResidue *tres = (CResidue *) residue_p; // casting horriblness.
   seqnum = tres->seqNum;
   ins_code = tres->GetInsCode();
   name = tres->name;
   int nResidueAtoms;
   PPCAtom residue_atoms;
   tres->GetAtomTable(residue_atoms, nResidueAtoms);
   for (int i=0; i<nResidueAtoms; i++) {
      addatom(std::string(residue_atoms[i]->name),
	      std::string(residue_atoms[i]->element),
	      residue_atoms[i]->x,
	      residue_atoms[i]->y,
	      residue_atoms[i]->z,
	      std::string(residue_atoms[i]->altLoc),
	      residue_atoms[i]->tempFactor);
   }
}

bool
coot::minimol::atom::is_hydrogen_p() const {

   if (element == " H" ||
       element == " D") {
      return 1;
   } else {
      return 0;
   }

}

const coot::minimol::atom& 
coot::minimol::residue::operator[](const std::string &atname) const { 

   for (unsigned int i=0; i<atoms.size(); i++) { 
      if (atoms[i].name == atname) { 
	 return atoms[i];
      } 
   }
   std::cout << "ERROR!  DISASTER! Atom name lookup failed atom \"" 
	     << atname << "\" amongst " << atoms.size() << " atoms: not found in residue "
	     << seqnum << std::endl;
   return atoms[0]; 
}

coot::minimol::residue&
coot::minimol::fragment::operator[](int i) {

   // if in range that we have, no adjustment required...

//    std::cout << "   at start of operator[] residues.size() " << residues.size()
// 	     << " and offset: " << residues_offset << " and i: " << i << std::endl;

   int itmp = residues.size() + residues_offset;
   // This fails:
   // if ((i > residues_offset) && (i < (residues.size() + residues_offset))) {
   // This works: 
   // if ((i > residues_offset) && (i < itmp)) {
   // Why?  Ghod knows!
   
   if ((i > residues_offset) && (i < itmp)) {
      return residues[i-residues_offset];
   } else {

      if (i <= residues_offset) {
	 // adding to the N terminus:
	 // e.g. adding residue 0 to residues 1..4
	 //
	 int new_offset = i - 1;
	 int offset_diff = new_offset - residues_offset;
	 std::vector<residue> new_residues(residues.size() - offset_diff);
	 // copy across the current residues...
	 if (residues.size() > 0) { 
	    for (int ires=1; ires<=int(residues.size()-1); ires++) {
// 	       std::cout << "copying from "
// 			 << ires << " of " << residues.size()
// 			 << " to new residues "
// 			 << ires-offset_diff << " of " << new_residues.size()
// 			 << std::endl;
	       new_residues[ires-offset_diff] = residues[ires];
	    }
	 }

	 // now transfer new residues to residues:
	 //
	 residues = new_residues;
	 residues_offset = new_offset;
	 
      } else {
	 // adding to C terminus;
	 residues.resize(i+1-residues_offset);
      }
//       std::cout << "   at   end of operator[] residues.size() " << residues.size()
// 		<< " and offset: " << residues_offset << " and i: " << i << std::endl;
//       std::cout << "      returing with residues index " << i-residues_offset
// 		<< " which has " << residues[i-residues_offset].atoms.size()
// 		<< " atoms " << std::endl;
      return residues[i-residues_offset];
   }
}

// coot::minimol::residue&
// void
// coot::minimol::residue::operator=(const coot::minimol::residue &res_in) {

//    // std::cout << "in operator= atoms in.size() " << res_in.atoms.size() << std::endl;
//    seqnum = res_in.seqnum;
//    name = res_in.name;
//    atoms = res_in.atoms;

// }


void
coot::minimol::residue::delete_atom_indices(const std::vector<int> &atom_indices) {
   
   std::vector<coot::minimol::atom> new_atoms;
   for (unsigned int iat=0; iat<atoms.size(); iat++) {
      bool found = 0;
      for (unsigned int iind=0; iind<atom_indices.size(); iind++) {
	 if (iat == atom_indices[iind]) {
	    found = 1;
	    break;
	 }
      }
      if (found == 0) {
	 new_atoms.push_back(atoms[iat]);
      }
   }
   atoms = new_atoms;
}


// Perhaps this should also return a status flag?
//
clipper::Coord_orth
coot::minimol::fragment::midpoint() const {

   clipper::Coord_orth p(0, 0, 0);
   int minr = min_res_no();
   int maxr = max_residue_number();
   int n_at = 0;
   for (int ires=minr; ires<=maxr; ires++)
      for (unsigned int iat=0; iat<(*this)[ires].atoms.size(); iat++) {
	 n_at++; 
	 p += (*this)[ires][iat].pos;
      }

   if (n_at>0) {
      float one_over_nat = 1/float(n_at);
      p = clipper::Coord_orth(p.x()*one_over_nat,
			      p.y()*one_over_nat,
			      p.z()*one_over_nat);
   }
   return p;
} 

void
coot::minimol::residue::addatom(std::string atom_name,
				std::string element,
				float x, float y, float z, const std::string &altloc,
				float bf) {
   atoms.push_back(atom(atom_name, element, x, y, z, altloc, bf));
}


void 
coot::minimol::residue::addatom(std::string atom_name, std::string element,
				const clipper::Coord_orth &pos, const std::string &altloc, float bf) { 
   atoms.push_back(atom(atom_name, element, pos.x(), pos.y(), pos.z(), altloc, bf));
}


std::vector<coot::minimol::atom*>
coot::minimol::residue::select_atoms_serial() const {
   std::vector<coot::minimol::atom*> a;
   int natoms = atoms.size();
   for (unsigned int iatom=0; iatom<natoms; iatom++) {
      a.push_back((coot::minimol::atom *) &(atoms[iatom]));
   }
   return a;
}

void
coot::minimol::residue::addatom(const atom &at) {
   atoms.push_back(at); 
} 

coot::minimol::atom::atom(std::string atom_name,
			  std::string ele, float x, float y, float z, const std::string &altloc, float dbf) {
   name = atom_name;
   element = ele;
   altLoc = altloc;
   pos = clipper::Coord_orth(x,y,z);
   occupancy = 1.0;
   temperature_factor = dbf;
} 

coot::minimol::atom::atom(std::string atom_name,
			  std::string ele, 
			  const clipper::Coord_orth &pos_in, const std::string &altloc,  float dbf) {
   name = atom_name;
   element = ele;
   pos = pos_in;
   altLoc = altloc;
   occupancy = 1.0;
   temperature_factor = dbf;
} 

coot::minimol::atom::atom(std::string atom_name,
			  std::string ele, 
			  const clipper::Coord_orth &pos_in, const std::string &altloc,
			  float occupancy_in,
			  float dbf) {
   name = atom_name;
   element = ele;
   pos = pos_in;
   altLoc = altloc;
   occupancy = occupancy_in;
   temperature_factor = dbf;
} 

void
coot::minimol::molecule::delete_molecule() {

   fragments.clear();

}


coot::minimol::zone_info_t
coot::minimol::molecule::zone_info() const {

   coot::minimol::zone_info_t zi;
   if (fragments.size() == 1) {
      int r1 = fragments[0].min_res_no();
      int r2 = fragments[0].max_residue_number();
      if (r1 < r2) 
	 zi = coot::minimol::zone_info_t(fragments[0].fragment_id, r1, r2);
   }
   return zi;
}


// We create (with new) a full mmdb CMMDBManager and pass back the
// pointer to it.  You are responsible for deleting it.
//
PCMMDBManager 
coot::minimol::molecule::pcmmdbmanager() const {

   PCMMDBManager mol = new CMMDBManager;
   InitMatType();

   // we have to add to the mmdb mol atom by atom

   CModel *model_p = new CModel;
   CChain *chain_p;
   CResidue *res_p;
   CAtom    *atom_p;
   int i_add; // error flag
   for (unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
//       std::cout << "fragment_id for fragment " << ifrag << " is "
// 		<< fragments[ifrag].fragment_id << std::endl;
      // if this fragment was uninitialised, residues size is 0.  We
      // don't want a mesage saying that we are writing out "0 - 1" (=
      // 4294967295) residues.
      int n_residues = fragments[ifrag].residues.size();
      if (n_residues != 0) n_residues = n_residues -1;
//       std::cout << "   writing out " << n_residues
// 		<< " residues for " << fragments[ifrag].fragment_id << std::endl; 
      chain_p = new CChain;
      chain_p->SetChainID((char *)fragments[ifrag].fragment_id.c_str());
      model_p->AddChain(chain_p);
      // see that residues are not zero index (fragments and atoms *are*).
      for (int ires=fragments[ifrag].min_res_no(); ires<=fragments[ifrag].max_residue_number(); ires++) {
	 if (fragments[ifrag][ires].atoms.size() > 0) {
	    res_p = new CResidue;
	    res_p->SetResID((*this)[ifrag][ires].name.c_str(),
			    (*this)[ifrag][ires].seqnum,
			    (*this)[ifrag][ires].ins_code.c_str());
	    chain_p->AddResidue(res_p);
	    if (fragments[ifrag][ires].atoms.size() > 0) { 
	       // 	    std::cout << "      residue " << ires << " "
	       // 		      << fragments[ifrag].fragment_id << " has "
	       // 		      << fragments[ifrag][ires].atoms.size()
	       // 		      << " atoms " << std::endl;
	       for (unsigned int iatom=0; iatom<fragments[ifrag][ires].atoms.size(); iatom++) {
		  atom_p = new CAtom;
		  atom_p->SetCoordinates((*this)[ifrag][ires][iatom].pos.x(),
					 (*this)[ifrag][ires][iatom].pos.y(),
					 (*this)[ifrag][ires][iatom].pos.z(),
					 (*this)[ifrag][ires][iatom].occupancy,
					 (*this)[ifrag][ires][iatom].temperature_factor);
		  atom_p->SetAtomName((*this)[ifrag][ires][iatom].name.c_str());
		  strncpy(atom_p->element,(*this)[ifrag][ires][iatom].element.c_str(),3);
		  strncpy(atom_p->altLoc, (*this)[ifrag][ires][iatom].altLoc.c_str(), 2);
		  i_add = res_p->AddAtom(atom_p);
		  if (i_add < 0) 
		     std::cout << "addatom addition error" << std::endl;
	       }
	    }
	 }
      }
   }
	 
   mol->AddModel(model_p);
   
   if (have_cell) { 
      // std::cout << "DEBUG:: pcmmdbmanager: using cell "
      //       << mmdb_cell[0] << " " << mmdb_cell[1]
      //    		<< " " << mmdb_cell[2] << " " << mmdb_cell[3]
      //    		<< " " << mmdb_cell[4] << " " << mmdb_cell[4] << "\n";
      mol->SetCell(mmdb_cell[0], mmdb_cell[1], mmdb_cell[2],
		   mmdb_cell[3], mmdb_cell[4], mmdb_cell[5], 1);
      realtype cell[6];
      realtype vol;
      int orthcode;
      mol->GetCell(cell[0], cell[1], cell[2], cell[3],
		   cell[4], cell[5], vol, orthcode); 
//       std::cout << "DEBUG:: Cell post set: "
//  		<< cell[0] << " " << cell[1] << " " << cell[2] << " " 
//  		<< cell[3] << " " << cell[4] << " " << cell[5] << " " << "\n";
      
//    } else {
//       std::cout << "DEBUG:: pcmmdbmanager: no cell for this molecule\n";
      
   }
   if (have_spacegroup) { 
//       std::cout << "DEBUG:: pcmmdbmanager: spacegroup for this molecule: " 
// 		<< mmdb_spacegroup << "\n";
      mol->SetSpaceGroup((char *)mmdb_spacegroup.c_str());
//    } else {
//       std::cout << "DEBUG:: pcmmdbmanager: no spacegroup for this molecule\n";
   }

   mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
   return mol;
}


std::vector<coot::minimol::atom*>
coot::minimol::molecule::select_atoms_serial() const {

   std::vector<coot::minimol::atom*> a;
//    std::cout << "DEBUG in --- select_atoms_serial: " << fragments.size()
// 	     << " fragments" << std::endl;
   for (unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) { 
//       std::cout << "   DEBUG in --- select_atoms_serial: from " << fragments[ifrag].min_res_no()
// 		<< " to " << fragments[ifrag].max_residue_number()
// 		<< "  residues " << std::endl;
      for (int ires=fragments[ifrag].min_res_no(); ires<=fragments[ifrag].max_residue_number(); ires++) { 
// 	 std::cout << "      DEBUG in --- select_atoms_serial: " << fragments[ifrag][ires].atoms.size()
// 		   << "  atoms " << std::endl;
	 for (unsigned int iatom=0; iatom<fragments[ifrag][ires].atoms.size(); iatom++) { 
	    // cast away constness
	    a.push_back((coot::minimol::atom *) &(*this)[ifrag][ires][iatom]);
	 }
      }
   }

//    std::cout << "DEBUG in --- select_atoms_serial: " << a.size()
// 	     << "atoms added" << std::endl;
   return a;
}

coot::minimol::molecule
coot::minimol::molecule::molecule_of_atom_types(std::string at_type) const {

   coot::minimol::molecule a;
   
   for (unsigned int ifr=0; ifr<fragments.size(); ifr++) {
      a.fragments.push_back(minimol::fragment(fragments[ifr].fragment_id));
      //       std::cout << "fragment id: "
      // 		<< (*this)[ifrag].fragment_id << std::endl; 
      for (unsigned int ires=fragments[ifr].min_res_no(); ires<=fragments[ifr].max_residue_number(); ires++)
	 for (unsigned int iatom=0; iatom<fragments[ifr][ires].atoms.size(); iatom++)
	    if (fragments[ifr][ires][iatom].name == at_type)
	       a[ifr][ires].addatom(fragments[ifr][ires][iatom]);
   }

   return a;
}

// Sometime one doesn't want to add the residue if it is empty.
// Sometimes one does.
void
coot::minimol::fragment::addresidue(const coot::minimol::residue &res,
				    short int add_if_empty_flag) {

   if (res.atoms.size() > 0 || add_if_empty_flag) {
      (*this)[res.seqnum] = res;
   }
}

void
coot::minimol::fragment::transform(const clipper::RTop_orth &rtop) {

   for (int ires=min_res_no(); ires<=max_residue_number(); ires++)
      for (unsigned int iatom=0; iatom<(*this)[ires].atoms.size(); iatom++)
	 (*this)[ires][iatom].pos = (*this)[ires][iatom].pos.transform(rtop);
}


void
coot::minimol::molecule::transform(const clipper::RTop_orth &rtop) {
   for (unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
      fragments[ifrag].transform(rtop);
   }
}


std::vector<coot::minimol::atom*>
coot::minimol::fragment::select_atoms_serial() const {

   std::vector<coot::minimol::atom*> a;
   
   // check();
   for (int ires=min_res_no(); ires<=max_residue_number(); ires++) { 
      for (unsigned int iatom=0; iatom<(*this)[ires].atoms.size(); iatom++) { 
	 // caste away constness, note we don't just index into the
	 // residues array, (that give bad offsetting).
	 a.push_back((coot::minimol::atom *) &(*this)[ires][iatom]);
      }
   }

//     for (int iat=0; iat<a.size(); iat++)
//        std::cout << "select_atoms_serial: " << iat << " " << a[iat]->pos.format() << std::endl;

   return a;
} 



std::ostream&
coot::minimol::operator<<(std::ostream& s, coot::minimol::atom at) {

   s << at.name << " :" << at.altLoc << ": " << at.pos.format();
   return s;
}

std::ostream&
coot::minimol::operator<<(std::ostream& s, coot::minimol::residue res) {

   s << res.name << " contains " << res.atoms.size() << " atoms";
   return s;
}

std::ostream&
coot::minimol::operator<<(std::ostream& s, coot::minimol::fragment frag) {

   s << frag.fragment_id << " contains " << frag.residues.size()
     << " residues";
   return s;
}

bool
coot::minimol::molecule::is_empty() const {

   bool ival = 1;
   if (fragments.size() != 0)
      ival = 0;
   return ival;
}

int
coot::minimol::molecule::count_atoms() const {

   int n_atoms = 0;
   for(unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
      for(int ires=(*this)[ifrag].min_res_no();
	  ires<=(*this)[ifrag].max_residue_number();
	  ires++) {
	 for (unsigned int iat=0; iat<(*this)[ifrag][ires].atoms.size(); iat++) {
	    n_atoms++;
	 }
      }
   }
   return n_atoms;
}

void
coot::minimol::molecule::check() const {

   for(unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
      for(int ires=(*this)[ifrag].min_res_no(); ires<=(*this)[ifrag].max_residue_number(); ires++) {
	 for (unsigned int iat=0; iat<(*this)[ifrag][ires].atoms.size(); iat++) {
	    std::cout << " "  << (*this)[ifrag].fragment_id << " " << (*this)[ifrag][ires].seqnum
		      << " "  << (*this)[ifrag][ires].name
		      << " "  << (*this)[ifrag][ires][iat].name
		      << " :" << (*this)[ifrag][ires][iat].altLoc
		      << ": " << (*this)[ifrag][ires][iat].pos.format() << std::endl;
	 }
      }
   }
}

void
coot::minimol::fragment::check() const {

    std::cout << " check:: residues.size() is " << residues.size() << std::endl;
    std::cout << " check:: checking residues " << min_res_no() << " to "
	      << max_residue_number() << " inclusive" << std::endl;
   
   for(int ires=min_res_no(); ires<=max_residue_number(); ires++) {
      for (unsigned int iat=0; iat<(*this)[ires].atoms.size(); iat++) {
	 std::cout << " " << "residue index " << ires << " "
		   << (*this).fragment_id << " " << (*this)[ires].seqnum
		   << " " << (*this)[ires][iat].name
		   << " " << (*this)[ires][iat].pos.format() << std::endl;
      }
   }
   std::cout << "check done." << std::endl;
}


int
coot::minimol::fragment::n_filled_residues() const {

   int ifr = 0;
   for (int ires=min_res_no(); ires<=max_residue_number(); ires++) {
      if ( (*this)[ires].atoms.size() > 0 )
	 ifr++;
   }
   return ifr;
} 

// Don't use the atomic weight.  I.e. all atoms are equally weighted.
// FIXME
// 
clipper::Coord_orth
coot::minimol::molecule::centre() const {

   clipper::Coord_orth running(0.0,0.0,0.0);
   int n_atom = 0;
   
   for(unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
      for (int ires=fragments[ifrag].min_res_no(); ires<=fragments[ifrag].max_residue_number(); ires++) {
	 for (unsigned int iatom=0; iatom< fragments[ifrag][ires].atoms.size(); iatom++) {
	    running += fragments[ifrag][ires][iatom].pos;
	    n_atom++;
	 }
      }
   }
   if (n_atom > 0) {
      float scale = 1/float(n_atom);
      running = clipper::Coord_orth(running.x()*scale,
				    running.y()*scale,
				    running.z()*scale);
   }
   return running;
}


std::string
coot::minimol::molecule::unused_chain_id(const std::string &pref_chain) const {

   std::string r(pref_chain);
   r += "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   int r_i = 0;
   std::string t;
   std::string::size_type idx;

   for (unsigned int ifrag=0; ifrag<fragments.size(); ifrag++) {
      t = fragments[ifrag].fragment_id;
      if (t != "") { 
	 idx = r.find(t);
	 while (idx != std::string::npos) { 
	    r = r.substr(0,idx) + r.substr(idx+1);
	    idx = r.find(fragments[ifrag].fragment_id);
	 }
      }
   }
   std::string s = "";
   s += r[r_i];
   // std::cout << "debug: minimol unused_chain_id: " << s << std::endl;
   return s;
}

std::vector<float> 
coot::minimol::molecule::get_cell() const { 

   std::vector<float> c;

   if (have_cell) {
      c = std::vector<float>(6);
      for (int i=0; i<6; i++) 
	 c[i] = mmdb_cell[i];
   }
   return c;
}
	

std::string
coot::minimol::molecule::get_spacegroup() const { 

   std::string r;

   if (have_spacegroup) 
      r = mmdb_spacegroup;

   return r;
} 


short int
coot::minimol::molecule::set_cell_symm(const coot::minimol::molecule &mol) { 

   short int istat = 1;

   std::vector<float> c = mol.get_cell();
   if (c.size() > 0) { 
      mmdb_cell = c;
      have_cell = 1;
      
      std::string s = mol.get_spacegroup();
      if (s.length() > 0) { 
	 set_spacegroup(s);
      } else { 
	 std::cout << "WARNING: no spacegroup in set_cell_symm\n";
	 istat = 0;
      } 
   } else { 
      std::cout << "WARNING: no cell in set_cell_symm\n";
      istat = 0;
   }
   return istat;
}


// add arbitary atom somewhere.
void
coot::minimol::molecule::addatom(const std::string &chain_id_in, int resno,
				 const atom &at, short int is_water_flag) {
   
   int frag = fragment_for_chain(chain_id_in);
   fragments[frag][resno].addatom(at);
   if (fragments[frag][resno].name == "") {
      if (is_water_flag) 
	 fragments[frag][resno].name = "HOH";
      else 
	 fragments[frag][resno].name = "ALA";
   }
}


// Set all the atoms that match the given name to the given occupancy.
// 
// Return the number of atoms adjusted
// 
int
coot::minimol::molecule::set_atom_occ(const std::string &atom_name, float occ) {

   int natoms = 0;
   for (unsigned int ifr=0; ifr<fragments.size(); ifr++) {
      for (unsigned int ires=fragments[ifr].min_res_no(); ires<=fragments[ifr].max_residue_number(); ires++)
	 for (unsigned int iatom=0; iatom<fragments[ifr][ires].atoms.size(); iatom++)
	    if (fragments[ifr][ires][iatom].name == atom_name) { 
	       fragments[ifr][ires][iatom].occupancy = occ;
	       natoms++;
	    }
      
   }
   return natoms;
}
	 

// sorting chains lexographically.
void coot::minimol::molecule::sort_chains() {

   std::sort(fragments.begin(), fragments.end());

} 
