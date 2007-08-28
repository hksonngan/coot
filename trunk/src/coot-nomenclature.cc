/* src/main.cc
 * 
 * Copyright 2005 by The University of York
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


#include <iostream>
#include "coot-nomenclature.hh"
#include "simple-restraint.hh"


#ifdef USE_DUNBRACK_ROTAMERS
#include "dunbrack.hh"
#else 
#include "richardson-rotamer.hh"
#endif 


// Here we rename atoms to fix nomeclature errors. Note ILEs are not fixed
// by renaming atoms.
// 
std::vector<CResidue *>
coot::nomenclature::fix(coot::protein_geometry *Geom_p) {

   std::vector<CResidue *> vr;
   if (mol_) { 

      int n_models = mol_->GetNumberOfModels();
      for (int imod=1; imod<=n_models; imod++) { 
      
	 CModel *model_p = mol_->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p == NULL) {  
		  // This should not be necessary. It seem to be a
		  // result of mmdb corruption elsewhere - possibly
		  // DeleteChain in update_molecule_to().
		  std::cout << "NULL chain in fix_nomenclature_errors "
			    << std::endl;
	       } else { 
		  int nres = chain_p->GetNumberOfResidues();
		  PCResidue residue_p;
		  for (int ires=0; ires<nres; ires++) { 
		     residue_p = chain_p->GetResidue(ires);
		     std::string residue_name(residue_p->GetResName());

		     if ((residue_name == "PHE") ||
			 (residue_name == "TYR")) {

			int isw = test_and_fix_PHE_TYR_nomenclature_errors(residue_p);
			if (isw)
			   vr.push_back(residue_p);
		     }
		     
		     if ((residue_name == "ASP") ||
			 (residue_name == "GLU")) {

			// we want a function that tests with changed
			// atom names and then changes the atom names
			// in residue_p if needed.  We want to know if
			// this has happened.

#ifdef USE_DUNBRACK_ROTAMERS			
			coot::dunbrack d(residue_p);
#else
			coot::richardson_rotamer d(residue_p);
#endif // USE_DUNBRACK_ROTAMERS			

			int swapped = d.optimize_rotamer_by_atom_names();
			if (swapped) {
			   std::cout << "INFO:: residue " << residue_p->GetChainID()
				     << " " << residue_p->GetSeqNum() << " "
				     << residue_p->GetResName()
				     << " had its atoms swapped\n";
			   vr.push_back(residue_p);
			}
		     }

		     if (residue_name == "THR") { 

			// This is assigned a sign in the refmac
			// dictionary, (unlike LEU and VAL).

			std::vector<coot::dict_chiral_restraint_t> chiral_restraints = 
			   Geom_p->get_monomer_chiral_volumes(std::string(residue_p->name));
			coot::dict_chiral_restraint_t chiral_restraint;
			for (unsigned int irestr=0; irestr<chiral_restraints.size(); irestr++) { 
			   chiral_restraint = chiral_restraints[irestr];
			   
			   // now what is the chiral centre for that restraint?
			    
			   if (chiral_restraint.atom_id_c_4c() == " CB ") {
			      
			      // make a synthetic restraint from
			      // chiral_restraint because we want to
			      // impose a sign on the CB chiral
			      // centre - in the chiral_restraint the 

			      std::cout << "For residue " << residue_p->GetSeqNum() << " "
					<< residue_p->GetResName() << " chiral centre: "
					<< " for restraint: " << irestr << " :" 
					<<  chiral_restraint.atom_id_c_4c() << ":\n";

#ifdef HAVE_GSL			      
			      std::vector<std::pair<short int, coot::atom_spec_t> > c = 
				 coot::is_bad_chiral_atom_p(chiral_restraint, residue_p);
			      for (unsigned int ibad=0; ibad<c.size(); ibad++) { 
				 if (c[ibad].first) {
				    std::cout << "INFO:: found bad THR chiral atom: " 
					      << chain_p->GetChainID() << " " 
					      << residue_p->GetSeqNum() << " "
					      << residue_p->GetInsCode() << " "
					      << chiral_restraint.atom_id_c_4c() << " "
					      << c[ibad].second.alt_conf << std::endl;

				    // swap the OG1 and CG2 atoms of
				    // the residue for the given
				    // alt_conf
				    std::string alt_conf_bad = c[ibad].second.alt_conf;
				    PPCAtom residue_atoms;
				    int n_residue_atoms;
				    residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
				    CAtom *og1 = 0;
				    CAtom *cg2 = 0;
				    for (int iat=0; iat<n_residue_atoms; iat++) {
				       std::string alt_conf = residue_atoms[iat]->altLoc;
				       std::string atom_name = residue_atoms[iat]->name;
				       if (atom_name == " OG1" )
					  if (alt_conf == alt_conf_bad)
					     og1 = residue_atoms[iat];
				       if (atom_name == " CG2" )
					  if (alt_conf == alt_conf_bad)
					     cg2 = residue_atoms[iat];
				    }
				    if (og1 && cg2) {
				       og1->SetAtomName(" CG2");
				       cg2->SetAtomName(" OG1");
				       std::cout << "        CG2 and OG1 atoms swapped\n";
				       vr.push_back(residue_p);
				    } else {
				       // This can't happen:
				       std::cout << "ERROR:: Bizarre missing atom scenario "
						 << "in fix_nomenclature_errors\n";
				    }
				 }
			      }
#endif // HAVE_GSL			      
			   }
			}
		     }
		     
		     if ((residue_name == "LEU") ||
			 (residue_name == "VAL")) {
			
			int volume_sign = -1;
			coot::dict_chiral_restraint_t synthetic_restraint;
			
			if (residue_name == "VAL")
			   synthetic_restraint =
			      coot::dict_chiral_restraint_t(residue_name,
							    " CB ", " CA ", " CG1", " CG2",
							    volume_sign);
			if (residue_name == "LEU")
			   synthetic_restraint =
			      coot::dict_chiral_restraint_t(residue_name,
							    " CG ", " CB ", " CD1", " CD2",
							    volume_sign);
			   
#ifdef HAVE_GSL			
			std::vector<std::pair<short int, coot::atom_spec_t> > c = 
			   coot::is_bad_chiral_atom_p(synthetic_restraint,
						      residue_p);
			
			for (unsigned int ibad=0; ibad<c.size(); ibad++) {
			   if (c[ibad].first) {
			      std::cout << "INFO:: found bad " << residue_name
					<< " chiral atom: " 
					<< chain_p->GetChainID() << " " 
					<< residue_p->GetSeqNum() << " "
					<< residue_p->GetInsCode() << " "
					<< synthetic_restraint.atom_id_c_4c() << " "
					<< c[ibad].second.alt_conf << std::endl;

			      // swap the CG1 and CG2 atoms of
			      // the residue for the given
			      // alt_conf
			      std::string alt_conf_bad = c[ibad].second.alt_conf;
			      std::string target_atom_1 = " CG1";
			      std::string target_atom_2 = " CG2";
			      if (residue_name == "LEU") {
				 target_atom_1 = " CD1";
				 target_atom_2 = " CD2";
			      }
			      PPCAtom residue_atoms;
			      int n_residue_atoms;
			      residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
			      CAtom *cg1 = 0; // cd1 and cd2 for LEU of course
			      CAtom *cg2 = 0;
			      for (int iat=0; iat<n_residue_atoms; iat++) {
				 std::string alt_conf = residue_atoms[iat]->altLoc;
				 std::string atom_name = residue_atoms[iat]->name;
				 if (atom_name == target_atom_1 )
				    if (alt_conf == alt_conf_bad)
				       cg1 = residue_atoms[iat];
				 if (atom_name == target_atom_2 )
				    if (alt_conf == alt_conf_bad)
				       cg2 = residue_atoms[iat];
			      }
			      if (cg1 && cg2) {
				 cg1->SetAtomName(target_atom_2.c_str());
				 cg2->SetAtomName(target_atom_1.c_str());
				 std::cout << "        " << target_atom_1 << " and "
					   << target_atom_2 << " atoms swapped\n";
				 vr.push_back(residue_p);
			      } else {
				 // This can't happen:
				 std::cout << "ERROR:: Bizarre missing atom scenario "
					   << "in fix_nomenclature_errors, residue type:"
					   << residue_name << "\n";
			      }
			   }
			}
#endif // HAVE_GSL			
		     }
		  }
	       }
	    }
	 }
      }
   }
   std::cout << "INFO:: " << vr.size() << " residues had their atoms swapped\n";
   return vr;
}


// test chi2 (and potentially fix) so that chi2 is the range -90 -> +90.
int
coot::nomenclature::test_and_fix_PHE_TYR_nomenclature_errors(CResidue *residue_p) {

   int iswapped = 0; // number of alt confs swapped in this residue
   PPCAtom residue_atoms;
   int n_residue_atoms;
   residue_p->GetAtomTable(residue_atoms, n_residue_atoms);

   // We have to test all the altconfs, so what are the altconfs of the CD1 atoms?
   
   std::vector<std::string> alt_conf_list;
   // first get the altconfs in the residue:
   for (int i=0; i<n_residue_atoms; i++) {
      std::string atom_name = residue_atoms[i]->name;
      if(atom_name == " CD1") {
	 alt_conf_list.push_back(residue_atoms[i]->altLoc);
      }
   }

   // so we need to test (and potentially fix) each altconf
   //
   // We first try to find all chi2 atoms with the same altconf as the
   // CD1 atom, failing that we find the atom with alt conf "".  And
   // failing that, there are missing atoms and we can't determine the
   // torsion
   for (unsigned int ialtconf=0; ialtconf<alt_conf_list.size(); ialtconf++) {
      CAtom *CA  = 0;
      CAtom *CB  = 0;
      CAtom *CG  = 0;
      CAtom *CD1 = 0;
      CAtom *CD2 = 0;
      for (int i=0; i<n_residue_atoms; i++) {
	 std::string atom_name = residue_atoms[i]->name;
	 std::string atom_altconf = residue_atoms[i]->altLoc;
	 if (atom_altconf == alt_conf_list[ialtconf]) {
	    if (atom_name == " CA ")
	       CA = residue_atoms[i];
	    if (atom_name == " CB ")
	       CB = residue_atoms[i];
	    if (atom_name == " CG ")
	       CG = residue_atoms[i];
	    if (atom_name == " CD1")
	       CD1 = residue_atoms[i];
	    if (atom_name == " CD2")
	       CD2 = residue_atoms[i];
	 }
      }
      if (CA==0 || CB==0 || CG==0) { // no need for CD1, it will be set
	 for (int i=0; i<n_residue_atoms; i++) {
	    std::string atom_name = residue_atoms[i]->name;
	    std::string atom_altconf = residue_atoms[i]->altLoc;
	    if (atom_altconf == "") {
	       if (atom_name == " CA ")
		  CA = residue_atoms[i];
	       if (atom_name == " CB ")
		  CB = residue_atoms[i];
	       if (atom_name == " CG ")
		  CG = residue_atoms[i];
	       if (atom_name == " CD2")
		  CD2 = residue_atoms[i];
	    }
	 }
      }
//       std::cout << "DEBUG:: Pointers: " << CA << " " << CB << " " << CG
// 		<< " " << CD1 << std::endl;
      
      if (CA && CB && CG && CD1) {
	 
	 clipper::Coord_orth a1(CA->x,  CA->y,  CA->z);
	 clipper::Coord_orth a2(CB->x,  CB->y,  CB->z);
	 clipper::Coord_orth a3(CG->x,  CG->y,  CG->z);
	 clipper::Coord_orth a4(CD1->x, CD1->y, CD1->z);

	 double tors = clipper::Util::rad2d(clipper::Coord_orth::torsion(a1, a2, a3, a4));

	 // std::cout << "DEBUG:: CD1 torsion: " << tors << std::endl;
	 if (tors < -90.0 || tors > 90.0) {
	    // ooops, there was a problem with this torsion.

	    if (CD2) {
	       clipper::Coord_orth a4_o(CD2->x, CD2->y, CD2->z);
	       double cg2_tors =
		  clipper::Util::rad2d(clipper::Coord_orth::torsion(a1, a2, a3, a4_o));
	       // if cg2_tors is in range, then we swap atom names
	       if (cg2_tors > -90.0 && cg2_tors < 90.0) {
		  // find CE1 and CE2 and swap if both sets exists.
		  CAtom *CE1 = 0;
		  CAtom *CE2 = 0;
		  for (int ie=0; ie<n_residue_atoms; ie++) {
		     std::string e_atom_name = residue_atoms[ie]->name;
		     std::string e_atom_altconf = residue_atoms[ie]->altLoc;
		     if (e_atom_altconf == alt_conf_list[ialtconf]) {
			if (e_atom_name == " CE1") 
			   CE1 = residue_atoms[ie];
			if (e_atom_name == " CE2") 
			   CE2 = residue_atoms[ie];
		     }
		  }
		  if (CE1 && CE2) {
		     CD1->SetAtomName(" CD2");
		     CD2->SetAtomName(" CD1");
		     CE1->SetAtomName(" CE2");
		     CE2->SetAtomName(" CE1");
		     iswapped++;
		  }
	       }
	    }
	 }
						    
      } else {
	 std::cout << "WARNING:: missing atoms in " << residue_p->GetChainID() << " "
		   << residue_p->GetSeqNum() << " " << residue_p->GetResName()
		   << std::endl;
      }
   }
   return iswapped;
}
