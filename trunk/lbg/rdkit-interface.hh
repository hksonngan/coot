
#ifndef ENTERPRISE_HH
#define ENTERPRISE_HH

#include "use-rdkit.hh"

#include "mmdb_manager.h"
#include "protein-geometry.hh"
#include "lbg-molfile.hh"

namespace coot { 

   // can throw an runtime_error exception (residue not in dictionary)
   // 
   RDKit::RWMol rdkit_mol(CResidue *residue_p, const protein_geometry &geom);
   int add_2d_conformer(RDKit::ROMol *rdkmol_in); // tweak rdkmol_in
   RDKit::Bond::BondType convert_bond_type(const std::string &t);

   lig_build::molfile_molecule_t make_molfile_molecule(const RDKit::ROMol &rdkm, int iconf);

   // Returns NULL on fail.
   //
   // Caller deletes.
   //
   // resulting residues has chain id of "" and residue number 1.
   //
   CResidue *make_residue(const RDKit::ROMol &rdkm, int iconf);
   
   // lig_build::molfile_molecule_t make_molfile_molecule(const RDKit::RWMol &rdkm);
   lig_build::bond_t::bond_type_t convert_bond_type(const RDKit::Bond::BondType &type);
   
   // This can throw a std::exception
   void remove_non_polar_Hs(RDKit::RWMol *rdkm); // fiddle with rdkm

   //
   void undelocalise(RDKit::RWMol *rdkm); // fiddle with rdkm

   // try to add (for instance) a +1 to Ns with 4 bonds.
   //
   enum { FORMAL_CHARGE_UNKNOWN = -1 };
   
   // account for Ns with "too many" hydrogens by assigning a formal
   // charge to the N.
   void assign_formal_charges(RDKit::RWMol *rdkm);

   // account for Ns with "too many" hydrogens by assigning deleting a
   // hydrogen.
   // 
   void delete_excessive_hydrogens(RDKit::RWMol *rdkm);
   
} 

#endif // ENTERPRISE_HH
