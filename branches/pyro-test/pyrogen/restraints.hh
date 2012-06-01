
#include "Python.h"
#include <string>
#include <iostream>
#include <vector>

#include <GraphMol/GraphMol.h>
#include <GraphMol/MolOps.h>
#include <GraphMol/Bond.h>

#include "protein-geometry.hh"
#include "mogul-interface.hh"


namespace coot { 

   void mogul_out_to_mmcif_dict(const std::string &mogul_file_name,
				const std::string &comp_id,
				const std::string &compound_name,
				const std::vector<std::string> &atom_names,
				int n_atoms_all,
				int n_atoms_non_hydrogen,
				PyObject *bond_order_restraints_py,
				const std::string &mmcif_out_file_name);

   // return restraints
   PyObject *mogul_out_to_mmcif_dict_by_mol(const std::string &mogul_file_name,
					    const std::string &comp_id,
					    const std::string &compound_name,
					    PyObject *rdkit_mol,
					    PyObject *bond_order_restraints_py,
					    const std::string &mmcif_out_file_name);

   void 
   mmcif_dict_from_mol(const std::string &comp_id,
		       const std::string &compound_name,
		       PyObject *rdkit_mol,
		       const std::string &mmcif_out_file_name);
   // which is a wrapper for:
   coot::dictionary_residue_restraints_t
   mmcif_dict_from_mol_inner(const std::string &comp_id,
			     const std::string &compound_name,
			     PyObject *rdkit_mol);
   
   void write_restraints(PyObject *restraints_py,
			 const std::string &monomer_type,
			 const std::string &file_name);

   // return a new rdkit molecule
   PyObject *regularize(PyObject *rdkit_mol, PyObject *restraints_py,
			const std::string &res_name);


   // alter restraints
   int assign_chirals(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_atoms(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_aromatic_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_deloc_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);


   // alter restraints
   void fill_with_energy_lib_bonds(const RDKit::ROMol &mol,
				   const coot::energy_lib_t &energy_lib,
				   coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void fill_with_energy_lib_angles(const RDKit::ROMol &mol,
				    const coot::energy_lib_t &energy_lib,
				    coot::dictionary_residue_restraints_t *restraints);

   // alter restraints
   void fill_with_energy_lib_torsions(const RDKit::ROMol &mol,
				      const coot::energy_lib_t &energy_lib,
				      coot::dictionary_residue_restraints_t *restraints);

   std::string convert_to_energy_lib_bond_type(RDKit::Bond::BondType bt);

   void write_pdb_from_mol(PyObject *rdkit_mol_py,
			   const std::string &res_name,
			   const std::string &file_name);

   // private
   bool is_const_torsion(const RDKit::ROMol &mol,
			 const RDKit::Atom *at_2,
			 const RDKit::Atom *at_3);


}

