
#include "Python.h"
#include <string>
#include <iostream>
#include <vector>

#include <geometry/protein-geometry.hh>
#include <analysis/mogul-interface.hh>

#include <GraphMol/GraphMol.h>
#include <GraphMol/MolOps.h>
#include <GraphMol/Bond.h>
#include "restraints-private.hh"



namespace coot {

   class quartet_set {
   public:
      quartet_set() {
	 idx.resize(4,0);
      }
      quartet_set(const std::vector<unsigned int> &q_in) {
	 unsigned int n_max = 4;
	 idx.resize(4,0);
	 if (q_in.size() < n_max)
	    n_max = q_in.size();
	 for (unsigned int i=0; i<n_max; i++)
	    idx[i] = q_in[i];
      } 
      std::vector<unsigned int> idx;
      const unsigned int & operator[](unsigned int i) const { return idx[i]; }
   };

   void mogul_out_to_mmcif_dict(const std::string &mogul_file_name,
				const std::string &comp_id,
				const std::string &compound_name,
				const std::vector<std::string> &atom_names,
				int n_atoms_all,
				int n_atoms_non_hydrogen,
				PyObject *bond_order_restraints_py,
				const std::string &mmcif_out_file_name,
				bool quartet_planes, bool quartet_hydrogen_planes);

   // return restraints
   PyObject *mogul_out_to_mmcif_dict_by_mol(const std::string &mogul_file_name,
					    const std::string &comp_id,
					    const std::string &compound_name,
					    PyObject *rdkit_mol,
					    PyObject *bond_order_restraints_py,
					    const std::string &mmcif_out_file_name,
					    bool quartet_planes, bool quartet_hydrogen_planes,
					    bool replace_with_mmff_b_a_restraints=true);

   PyObject *
   mmcif_dict_from_mol(const std::string &comp_id,
		       const std::string &compound_name,
		       PyObject *rdkit_mol,
		       const std::string &mmcif_out_file_name,
		       bool quartet_planes, bool quartet_hydrogen_planes,
		       bool replace_with_mmff_b_a_restraints=true);
   
   // which is a wrapper for:
   coot::dictionary_residue_restraints_t
   mmcif_dict_from_mol_using_energy_lib(const std::string &comp_id,
					const std::string &compound_name,
					PyObject *rdkit_mol,
					bool quartet_planes, bool quartet_hydrogen_planes);
   
   void write_restraints(PyObject *restraints_py,
			 const std::string &monomer_type,
			 const std::string &file_name);

   void regularize(PyObject *rdkit_mol, PyObject *restraints_py,
		   const std::string &res_name);

   //
   void regularize_and_write_pdb(PyObject *rdkit_mol, PyObject *restraints_py,
				 const std::string &res_name,
				 const std::string &pdb_file_name);

   void write_pdb_from_mol(PyObject *rdkit_mol_py,
			   const std::string &res_name,
			   const std::string &file_name);

   PyObject *types_from_mmcif_dictionary(const std::string &file_name);

   PyObject *test_tuple();
   PyObject *match_restraints_to_dictionaries(PyObject *restraints_in,
					      PyObject *template_comp_id_list,
					      PyObject *template_cif_dict_file_names);
   void write_restraints(PyObject *restraints_py, const std::string &file_name);

}

