/* src/lbg-interface.cc
 * 
 * Copyright 2011, 2012 by The University of Oxford
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
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

#ifdef MAKE_ENTERPRISE_TOOLS

#if defined (USE_PYTHON)
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif


#include <cstring>
#define ENABLE_NLS // fix dcgettext() header problems on including
		   // libintl.h (via RDKitBase.h etc (including boost
		   // stuff).

#include "rdkit-interface.hh"
#include "lbg-interface.hh"
#include "graphics-info.h"
#include "lbg.hh"

#include "c-interface.h"
#include "cc-interface.hh" // for coot_get_url()

void
residue_to_ligand_builder(int imol, const char *chain_id, int res_no, const char *ins_code,
			  double weight_for_3d_distances) {

   graphics_info_t g;
   if (g.is_valid_model_molecule(imol)) {
      CResidue *residue_p =
	 graphics_info_t::molecules[imol].get_residue(chain_id, res_no, ins_code);
      if (residue_p) {
	 try {
	    RDKit::RWMol rdkm = coot::rdkit_mol(residue_p, *g.Geom_p());
	    RDKit::ROMol *rdk_mol_with_no_Hs_ro = RDKit::MolOps::removeHs(rdkm);
	    RDKit::RWMol rdk_mol_with_no_Hs = *rdk_mol_with_no_Hs_ro;

	    // clear out any cached properties
	    rdk_mol_with_no_Hs.clearComputedProps();
	    // clean up things like nitro groups
 	    RDKit::MolOps::cleanUp(rdk_mol_with_no_Hs);
 	    // update computed properties on atoms and bonds:
 	    rdk_mol_with_no_Hs.updatePropertyCache();
 	    RDKit::MolOps::Kekulize(rdk_mol_with_no_Hs);
 	    RDKit::MolOps::assignRadicals(rdk_mol_with_no_Hs);
	    
	    // then do aromaticity perception
	    // RDKit::MolOps::setAromaticity(rdkm);
    
	    // set conjugation
	    RDKit::MolOps::setConjugation(rdk_mol_with_no_Hs);
	       
	    // set hybridization
	    RDKit::MolOps::setHybridization(rdk_mol_with_no_Hs); // non-linear ester bonds, yay!

	    // remove bogus chirality specs:
	    RDKit::MolOps::cleanupChirality(rdk_mol_with_no_Hs);


	    if (0) {  // debugging
	       // what are the bond types after sanitization/kekulization?
	       std::cout << "after sanitization/kekulization:: " << std::endl;
	       int n_bonds = rdkm.getNumBonds();
	       for (unsigned int ib=0; ib<n_bonds; ib++) {
		  const RDKit::Bond *bond_p = rdk_mol_with_no_Hs.getBondWithIdx(ib);
		  std::cout << ib << "bond:  " << *bond_p << std::endl;
	       }
	    }

	    CMMDBManager *mol =
	       coot::util::create_mmdbmanager_from_residue(g.molecules[imol].atom_sel.mol,
							   residue_p);
	    if (!mol) {
	       std::cout << "ERROR:: failed to make mol for lbg" << std::endl;
	    } else { 
	       int mol_2d_depict_conformer = coot::add_2d_conformer(&rdk_mol_with_no_Hs,
								    weight_for_3d_distances);
	       lig_build::molfile_molecule_t m =
		  coot::make_molfile_molecule(rdk_mol_with_no_Hs, mol_2d_depict_conformer);

	       std::string view_name = "Molecule ";
	       view_name += coot::util::int_to_string(imol);
	       view_name += " ";
	       view_name += chain_id;
	       view_name += coot::util::int_to_string(res_no);
	       view_name += ins_code;
	       view_name += "    code: ";
	       view_name += residue_p->GetResName();
	       
	       std::pair<bool, coot::residue_spec_t> p(1, coot::residue_spec_t(residue_p));
	       bool use_graphics_flag = graphics_info_t::use_graphics_interface_flag;
	       bool stand_alone_flag = 0; // no, it isn't from here.
	       
	       int (*get_url_func_ptr) (const char *s1, const char *s2) = NULL;
#ifdef USE_LIBCURL
	       get_url_func_ptr = coot_get_url;
#endif      
	       lbg(m, p, mol, view_name, "", imol, use_graphics_flag, stand_alone_flag,
		   get_url_func_ptr,
		   prodrg_import_function,
		   sbase_import_function,
		   get_drug_mdl_via_wikipedia_and_drugbank
		   );
	    }
	    delete mol;
	 }
	 catch (std::runtime_error coot_error) {
	    std::cout << coot_error.what() << std::endl;
	    std::string m = "Residue type ";
	    m += residue_p->GetResName();
	    m += " not found in dictionary.";
	    add_status_bar_text(m.c_str());
	 }
	 catch (std::exception rdkit_error) {
	    std::cout << rdkit_error.what() << std::endl;
	 }
      } 
   } 
}

void smiles_to_ligand_builder(const char *smiles_string) {

   try {
      
      RDKit::RWMol *rdk_mol = RDKit::SmilesToMol(smiles_string);
      // if SmilesToMol() fails on parsing the string, then that
      // (can?) results in rdk_mol returning NULL (rather than an
      // exception being thrown).
      if (rdk_mol) { 
	 // clear out any cached properties
	 rdk_mol->clearComputedProps();
	 // clean up things like nitro groups
	 RDKit::MolOps::cleanUp(*rdk_mol);
	 // update computed properties on atoms and bonds:
	 rdk_mol->updatePropertyCache();
	 RDKit::MolOps::Kekulize(*rdk_mol);
	 RDKit::MolOps::assignRadicals(*rdk_mol);
	 // set conjugation
	 RDKit::MolOps::setConjugation(*rdk_mol);
	 // set hybridization
	 RDKit::MolOps::setHybridization(*rdk_mol); 
	 // remove bogus chirality specs:
	 RDKit::MolOps::cleanupChirality(*rdk_mol);
	 int iconf = RDDepict::compute2DCoords(*rdk_mol, 0, true);
	 lig_build::molfile_molecule_t m = coot::make_molfile_molecule(*rdk_mol, iconf);
   
	 std::cout << " molfile contains " << m.atoms.size() << " atoms " << std::endl;
	 for (unsigned int i=0; i<m.atoms.size(); i++) {
	    std::cout << "   " << i << "   name: " << m.atoms[i].name << " element: "
		      << m.atoms[i].element << " position: "
		      << m.atoms[i].atom_position.format() << " " << std::endl;
	 }
	 std::cout << " molfile contains " << m.bonds.size() << " atoms " << std::endl;
	 for (unsigned int i=0; i<m.bonds.size(); i++) { 
	    std::cout << "   " << m.bonds[i].index_1 << " " << m.bonds[i].index_2 << " type: "
		      << m.bonds[i].bond_type << std::endl;
	 }
   
	 CMMDBManager *mol = NULL;
	 std::pair<bool, coot::residue_spec_t> dummy_spec(0, coot::residue_spec_t());
	 bool use_graphics_flag = graphics_info_t::use_graphics_interface_flag;
	 bool stand_alone_flag = 0; // no, it isn't from here.
	 int (*get_url_func_ptr) (const char *s1, const char *s2) = NULL;
#ifdef USE_LIBCURL
	 get_url_func_ptr = coot_get_url;
#endif

	 lbg(m, dummy_spec, mol, "", "", -1, use_graphics_flag, stand_alone_flag,
	     get_url_func_ptr,
	     prodrg_import_function,
	     sbase_import_function,
	     get_drug_mdl_via_wikipedia_and_drugbank);
      }
   }
   catch (std::exception e) {
      std::cout << "WARNING:: in generating molecule from SMILES: "
		<< e.what() << std::endl;
   } 
   catch (...) {
      std::cout << "WARNING:: some other exception in  SMILES generation..."
		<< std::endl;
   } 
} 


#endif // MAKE_ENTERPRISE_TOOLS
