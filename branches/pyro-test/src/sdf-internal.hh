
#include "rdkit-interface.hh"
#include <GraphMol/MolChemicalFeatures/MolChemicalFeature.h>
#include "generic-display-object.hh"

namespace chemical_features { 

   // not for public access 
   void show(const RDKit::ROMol &rdkm, std::string name);
   
   std::pair<bool, clipper::Coord_orth> get_normal_info(RDKit::MolChemicalFeature *feat,
							const RDKit::ROMol &mol,
							const RDKit::Conformer &conf);
   std::pair<bool, clipper::Coord_orth> get_normal_info_aromatic(RDKit::MolChemicalFeature *feat,
								 const RDKit::Conformer &conf);
   std::pair<bool, clipper::Coord_orth> get_normal_info_donor(RDKit::MolChemicalFeature *feat,
							      const RDKit::ROMol &mol,
							      const RDKit::Conformer &conf);
   // return null on inability to make factory.
   RDKit::MolChemicalFeatureFactory *get_feature_factory();
   
}
