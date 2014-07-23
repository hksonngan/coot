
#include <stdexcept>

#include <sys/types.h> // for stating
#include <sys/stat.h>

#include "protein-geometry.hh"

void
coot::energy_lib_t::add_energy_lib_atom(    const coot::energy_lib_atom    &atom) {
   atom_map[atom.type] = atom;
} 

void
coot::energy_lib_t::add_energy_lib_bond(const coot::energy_lib_bond &bond) {
   bonds.push_back(bond);
}


void
coot::energy_lib_t::add_energy_lib_angle(   const coot::energy_lib_angle   &angle) {
   angles.push_back(angle);
}

void
coot::energy_lib_t::add_energy_lib_torsion(const coot::energy_lib_torsion &torsion) {
   torsions.push_back(torsion);
}

void
coot::protein_geometry::read_energy_lib(const std::string &file_name) {

   energy_lib.read(file_name);
}


void
coot::energy_lib_t::read(const std::string &file_name, bool print_info_message_flag) {

   struct stat buf;
   int istat = stat(file_name.c_str(), &buf);
   if (istat != 0) {
      std::cout << "WARNING:: energy lib " << file_name << " not found.\n";
      return;
   }
   
   CMMCIFFile ciffile;
   int ierr = ciffile.ReadMMCIFFile(file_name.c_str());
   if (ierr!=CIFRC_Ok) {
      std::cout << "dirty mmCIF file? " << file_name.c_str() << std::endl;
      std::cout << "    Bad CIFRC_Ok on ReadMMCIFFile" << std::endl;
      std::cout << "    " << GetErrorDescription(ierr) << std::endl;
      char        err_buff[1000];
      std::cout <<  "CIF error rc=" << ierr << " reason:" << 
	 GetCIFMessage (err_buff,ierr) << std::endl;
   } else {
      if (print_info_message_flag)
	 std::cout << "There are " << ciffile.GetNofData() << " data in "
		   << file_name << std::endl;
      for(int idata=0; idata<ciffile.GetNofData(); idata++) { 
	 PCMMCIFData data = ciffile.GetCIFData(idata);
	 // if (std::string(data->GetDataName()).substr(0,5) == "_lib_atom") {
	 // energy_lib_atoms(mmCIFLoop);

	 // std::cout << "read_energy_lib: " << data->GetDataName() << std::endl;

	 if (std::string(data->GetDataName()) == "energy") {
	    for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 
      
	       PCMMCIFCategory cat = data->GetCategory(icat);
	       std::string cat_name(cat->GetCategoryName());
	       
	       // std::cout << "DEBUG:: init_link is handling " << cat_name << std::endl;
	       
	       PCMMCIFLoop mmCIFLoop = data->GetLoop(cat_name.c_str());
	       
	       if (mmCIFLoop == NULL) { 
		  std::cout << "null loop" << std::endl; 
	       } else {
		  int n_chiral = 0;
		  if (cat_name == "_lib_atom")
		     add_energy_lib_atoms(mmCIFLoop);
		  if (cat_name == "_lib_bond")
		     add_energy_lib_bonds(mmCIFLoop);
		  if (cat_name == "_lib_angle")
		     add_energy_lib_angles(mmCIFLoop);
		  if (cat_name == "_lib_tors")
		     add_energy_lib_torsions(mmCIFLoop);
	       }
	    }
	 } 
      }
   }
}


void
coot::energy_lib_t::add_energy_lib_atoms(PCMMCIFLoop mmCIFLoop) {

   // note that that:
   // if (ierr) {
   //    xxx = -1
   // }
   // 
   // construction is necessary because if the GetXXX() fails then the
   // int/float is set to 0 or some such by the function.

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
      std::string type;
      realtype weight = -1;
      int hb_type = coot::energy_lib_atom::HB_UNASSIGNED;
      realtype vdw_radius = -1;
      realtype vdwh_radius = -1; // with implicit hydrogen, I presume
      realtype ion_radius = -1;
      std::string element;
      int valency = -1;
      int sp_hybridisation = -1;
      int ierr;
      int ierr_tot = 0;

      char *s;

      s = mmCIFLoop->GetString("type", j, ierr);
      ierr_tot += ierr;
      if (s) type = s;
         
      // This can fail (to set weight - we still have a useful atom description).
      //
      ierr = mmCIFLoop->GetReal(weight, "weight", j);
      if (ierr) {
	 weight = -1;
      }

      s = mmCIFLoop->GetString("hb_type", j, ierr);
      ierr_tot += ierr;
      if (s) {
	 std::string ss(s);
	 if (ss == "D")
	    hb_type = coot::energy_lib_atom::HB_DONOR;
	 if (ss == "A")
	    hb_type = coot::energy_lib_atom::HB_ACCEPTOR;
	 if (ss == "B")
	    hb_type = coot::energy_lib_atom::HB_BOTH;
	 if (ss == "N")
	    hb_type = coot::energy_lib_atom::HB_NEITHER;
	 if (ss == "H")
	    hb_type = coot::energy_lib_atom::HB_HYDROGEN;
      }

      // This can fail (to set vdw_radius - we still have a useful atom description).
      //
      ierr = mmCIFLoop->GetReal(vdw_radius, "vdw_radius",j);
      if (ierr) {
	 vdw_radius = -1;
      }

      // This can fail (to set vdwh_radius - we still have a useful atom description).
      //
      ierr = mmCIFLoop->GetReal(vdwh_radius, "vdwh_radius",j);
      if (ierr) {
	 vdwh_radius = -1;
      }

      // This can fail (to set ion_radius - we still have a useful atom description).
      //
      ierr = mmCIFLoop->GetReal(ion_radius, "ion_radius", j);
      if (ierr) {
	 ion_radius = -1;
      }

      s = mmCIFLoop->GetString("element", j, ierr);
      ierr_tot += ierr;
      if (s)
	 element = s;
      
      ierr = mmCIFLoop->GetInteger(valency, "valency", j);
      ierr_tot += ierr;
      
      // This can fail (to set the hybridisation - we still have a useful atom description).
      //
      ierr = mmCIFLoop->GetInteger(sp_hybridisation, "sp", j);
      if (ierr) {
	 sp_hybridisation = -1;
      }

      if (ierr_tot == 0) {
	 coot::energy_lib_atom at(type, hb_type, weight, vdw_radius, vdwh_radius,
				  ion_radius, element, valency, sp_hybridisation);
	 // std::cout << "DEBUG:: adding energy atom: " << at << std::endl;
	 add_energy_lib_atom(at);
      } 
      
   }
}

std::ostream&
coot::operator<<(std::ostream &s, const energy_lib_atom &at) {

   s << "[type: " << at.type << " weight: " << at.weight << " vdw_radius: " << at.vdw_radius
     << " vdwh_radius: " << at.vdwh_radius << " ion_radius: " << at.ion_radius
     << " element: " << at.element
     << " valency: " << at.valency << " sp_hybridisation: " << at.sp_hybridisation
     << "]";
   return s;
}

std::ostream&
coot::operator<<(std::ostream &s, const energy_lib_bond &bond) {

   s << "[type: " << bond.type << " atom-types: \""
     << bond.atom_type_1 << "\" \"" << bond.atom_type_2 << "\" "
     << bond.length << " " << bond.esd << "]";
   return s;
}

std::ostream&
coot::operator<<(std::ostream &s, const energy_lib_torsion &torsion) {

   s << "[" << " atom-types: \""
     << torsion.atom_type_1 << "\" \"" << torsion.atom_type_2 << "\" "
     << torsion.atom_type_3 << "\" \"" << torsion.atom_type_4 << "\" spring-k "
     << torsion.spring_constant << " angle: " << torsion.angle << " per: " << torsion.period
     << "]";
   return s;
} 


void
coot::energy_lib_t::add_energy_lib_bonds(PCMMCIFLoop mmCIFLoop) {

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
      
      std::string atom_type_1;
      std::string atom_type_2;
      std::string type;
      realtype spring_const;
      realtype length;
      realtype value_esd; 
      int ierr;
      int ierr_tot = 0;

      char *s;

      s = mmCIFLoop->GetString("atom_type_1", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_1 = s;

      s = mmCIFLoop->GetString("atom_type_2", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_2 = s;

      s = mmCIFLoop->GetString("type", j, ierr);
      ierr_tot += ierr;
      if (s) type = s;
      
      ierr = mmCIFLoop->GetReal(spring_const, "const", j);
      if (ierr) {
	 spring_const = 420;
      }
      
      ierr = mmCIFLoop->GetReal(length, "length", j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetReal(value_esd, "value_esd", j);
      if (ierr) {
	 value_esd = 0.02;
      }

      if (ierr_tot == 0) {
	 coot::energy_lib_bond bond(atom_type_1, atom_type_2, type, spring_const,
				    length, value_esd);
	 add_energy_lib_bond(bond);
      } else {
	 // FIXME, allow bond definitions with no distances?
	 if (0)
	    std::cout << "WARNING reject energy lib bond \"" << atom_type_1
		      << "\" \"" << atom_type_2 << "\" \"" << type << "\"" << std::endl;
      } 
   } 
}

void
coot::energy_lib_t::add_energy_lib_angles(PCMMCIFLoop mmCIFLoop) {

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
      
      std::string atom_type_1;
      std::string atom_type_2;
      std::string atom_type_3;
      std::string type;
      realtype spring_const;
      realtype value = 90.0;
      realtype value_esd = 1.8; // some arbitrary default!
      realtype ktheta = 45;
      int ierr;
      int ierr_tot = 0;

      char *s;

      s = mmCIFLoop->GetString("atom_type_1", j, ierr);
      if (s) atom_type_1 = s;

      s = mmCIFLoop->GetString("atom_type_2", j, ierr);
      if (ierr) std::cout << "error reading atom_type_1" << std::endl;
      ierr_tot += ierr;
      if (s) atom_type_2 = s;

      s = mmCIFLoop->GetString("atom_type_3", j, ierr);
      if (s) atom_type_3 = s;

      // F, V, Br, Cd, I all have no value.... Hmmm..
      ierr = mmCIFLoop->GetReal(value, "value", j);
      // if (ierr) std::cout << "error reading value" << std::endl;
      // ierr_tot += ierr;

      ierr = mmCIFLoop->GetReal(ktheta, "const", j);
      if (ierr != 0)
	 value_esd = 2.8; // dummy value
      else 
	 value_esd = ktheta * 0.04;

      if (ierr_tot == 0) {

	 if (0) // debug
	    std::cout << "DEBUG:: adding energy lib angle " 
		      << "\"" << atom_type_1 << "\" "
		      << "\"" << atom_type_2 << "\" "
		      << "\"" << atom_type_3 << "\" "
		      << "ktheta " << ktheta << " value " << value << " value_esd: " << value_esd
		      << std::endl;
	 
	 energy_lib_angle angle(atom_type_1, atom_type_2, atom_type_3, ktheta, value, value_esd);
	 angles.push_back(angle);
      } else {
	 std::cout << "  reject energy lib angle " 
		   << "\"" << atom_type_1 << "\" "
		   << "\"" << atom_type_2 << "\" "
		   << "\"" << atom_type_3 << "\" "
		   << std::endl;
      } 
   }
}

void
coot::energy_lib_t::add_energy_lib_torsions(PCMMCIFLoop mmCIFLoop) {

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      std::string atom_type_1;
      std::string atom_type_2;
      std::string atom_type_3;
      std::string atom_type_4;
      std::string label;
      realtype constant = 0;
      realtype angle;
      int period;
      int ierr;
      int ierr_tot = 0;

      char *s;

      s = mmCIFLoop->GetString("atom_type_1", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_1 = s;

      s = mmCIFLoop->GetString("atom_type_2", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_2 = s;

      s = mmCIFLoop->GetString("atom_type_3", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_3 = s;

      s = mmCIFLoop->GetString("atom_type_4", j, ierr);
      ierr_tot += ierr;
      if (s) atom_type_4 = s;

      ierr = mmCIFLoop->GetReal(constant, "const", j);

      ierr = mmCIFLoop->GetReal(angle, "angle", j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(period, "period", j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 energy_lib_torsion tors(atom_type_1, atom_type_2, atom_type_3, atom_type_4,
				 constant, angle, period);
	 torsions.push_back(tors);
      }
   }
}




int
coot::protein_geometry::get_h_bond_type(const std::string &atom_name, const std::string &monomer_name) const {

   bool debug = 0;  // before debugging this, is ener_lib.cif being
		    // read correctly?
   
   // this is heavy!
   // 
   std::pair<bool, coot::dictionary_residue_restraints_t> r =
      get_monomer_restraints(monomer_name);

   int hb_type = coot::energy_lib_atom::HB_UNASSIGNED;

   if (! r.first) {
      std::string m = "No dictionary for monomer_type: ";
      m += monomer_name;
      std::cout << m << std::endl;
   } else {
      for (unsigned int i=0; i<r.second.atom_info.size(); i++) {
	 if (r.second.atom_info[i].atom_id_4c == atom_name) { 
	    std::string type = r.second.atom_info[i].type_energy;
	    if (type.length()) {
	       std::map<std::string, coot::energy_lib_atom>::const_iterator it = 
		  energy_lib.atom_map.find(type);
	       if (it != energy_lib.atom_map.end()) { 
		  hb_type = it->second.hb_type;
		  if (debug)
		     std::cout << "DEBUG:: found hb_type " << hb_type << " for " << atom_name
			       << " given energy type " << type << std::endl;
	       }
	    }
	    break;
	 }
      }
   } 

   if (debug)
      if (hb_type == coot::energy_lib_atom::HB_UNASSIGNED)
	 std::cout << " failed to get_h_bond_type for " << atom_name << " in " << monomer_name
		   << std::endl;
	 
   return hb_type;

} 


// Find the non_bonded_contact distance, get_nbc_dist()
// 
// Return a pair, if not found the first is 0.  Look up in the energy_lib.
// 
std::pair<bool, double>
coot::protein_geometry::get_nbc_dist(const std::string &energy_type_1,
				     const std::string &energy_type_2) const {

   std::pair<bool, double> r(false, 0);
   std::map<std::string, coot::energy_lib_atom>::const_iterator it_1 = energy_lib.atom_map.find(energy_type_1);
   std::map<std::string, coot::energy_lib_atom>::const_iterator it_2 = energy_lib.atom_map.find(energy_type_2);

   if (it_1 != energy_lib.atom_map.end()) { 
      if (it_2 != energy_lib.atom_map.end()) {
	 r.first = true;
	 r.second = it_1->second.vdw_radius + it_2->second.vdw_radius;

	 // hack in a correction factor - that makes atoms fit into density  :-/
	 // 
	 // 0.9 is too much.

	 // There is an algorithm problem.
	 // 
	 // I need to reject atoms that have bond (done), angle and
	 // torsion interactions from NBC interactions, I think.  Then
	 // I can set r.second multiplier to 1.0 maybe.
	 // 
	 r.second *= 0.84;

	 // ring atoms should not be NBCed to each other.  Not sure
	 // that 5 atom rings need to be excluded in this manner.
	 // 
	 if (it_1->first == "CR15" || it_1->first == "CR16" || it_1->first == "CR1"  ||
	     it_1->first == "CR6"  || it_1->first == "CR5"  || it_1->first == "CR5"  ||
	     it_1->first == "CR56" || it_1->first == "CR5"  || it_1->first == "CR66" ||
	     it_1->first == "NPA"  || it_1->first == "NPB"  || it_1->first == "NRD5" ||
	     it_1->first == "NRD6" || it_1->first == "NR15" || it_1->first == "NR16" ||
	     it_1->first == "NR6"  || it_1->first == "NR5") {

	    if (it_2->first == "CR15" || it_2->first == "CR16" || it_2->first == "CR1"  ||
		it_2->first == "CR6"  || it_2->first == "CR5"  || it_2->first == "CR5"  ||
		it_2->first == "CR56" || it_2->first == "CR5"  || it_2->first == "CR66" ||
		it_2->first == "NPA"  || it_2->first == "NPB"  || it_2->first == "NRD5" ||
		it_2->first == "NRD6" || it_2->first == "NR15" || it_2->first == "NR16" ||
		it_2->first == "NR6"  || it_2->first == "NR5") {
	       r.second = 2.2;
	    }
	 }


	 // hydrogen bonds can be closer
	 // 
	 if ((it_1->second.hb_type == coot::energy_lib_atom::HB_DONOR ||
	      it_1->second.hb_type == coot::energy_lib_atom::HB_BOTH  ||
	      it_1->second.hb_type == coot::energy_lib_atom::HB_HYDROGEN) &&
	     (it_2->second.hb_type == coot::energy_lib_atom::HB_ACCEPTOR ||
	      it_2->second.hb_type == coot::energy_lib_atom::HB_BOTH)) { 
	    r.second -= 0.7;
	    // actual hydrogens to aceptors can be shorter still 
	    if (it_1->second.hb_type == coot::energy_lib_atom::HB_HYDROGEN)
	       r.second -=0.3;
	 } 

	 if ((it_2->second.hb_type == coot::energy_lib_atom::HB_DONOR ||
	      it_2->second.hb_type == coot::energy_lib_atom::HB_BOTH  ||
	      it_2->second.hb_type == coot::energy_lib_atom::HB_HYDROGEN) &&
	     (it_1->second.hb_type == coot::energy_lib_atom::HB_ACCEPTOR ||
	      it_1->second.hb_type == coot::energy_lib_atom::HB_BOTH)) { 
	    r.second -= 0.7;
	    // as above
	    if (it_1->second.hb_type == coot::energy_lib_atom::HB_HYDROGEN)
	       r.second -=0.3;
	 }
      }
   }
   return r;
} 

// throw a std::runtime_error if bond not found
// 
// Order dependent.
coot::energy_lib_bond
coot::energy_lib_t::get_bond(const std::string &energy_type_1,
			     const std::string &energy_type_2,
			     const std::string &bond_type) const {

   try {
      return get_bond(energy_type_1, energy_type_2, bond_type, false);
   }
   catch (std::runtime_error rte) {
      try { 
	 return get_bond(energy_type_2, energy_type_1, bond_type, false);
      } 
      catch (std::runtime_error rte) {
	 try { 
	    return get_bond(energy_type_1, energy_type_2, bond_type, true);
	 }
	 catch (std::runtime_error rte) {
	    return get_bond(energy_type_2 , energy_type_1, bond_type, true);
	 }
      }
   }
}


// throw a std::runtime_error if bond not found
// 
// Order dependent.
coot::energy_lib_bond
coot::energy_lib_t::get_bond(const std::string &energy_type_1,
			     const std::string &energy_type_2,
			     const std::string &bond_type, // refmac energy lib format
			     bool permissive_atom_type) const {

   bool found = false;
   std::string mess; // for exception
   
   energy_lib_bond bond;
   std::map<std::string, energy_lib_atom>::const_iterator it_1 = atom_map.find(energy_type_1);
   std::map<std::string, energy_lib_atom>::const_iterator it_2 = atom_map.find(energy_type_2);

   if (it_1 != atom_map.end() && it_2 != atom_map.end()) {
      bool permissive_bond_order = false;
      for (unsigned int ibond=0; ibond<bonds.size(); ibond++) {
	 if (bonds[ibond].matches(energy_type_1, energy_type_2,
				  bond_type, 
				  permissive_atom_type)) {
	    bond = bonds[ibond];
	    found = true;
	    break;
	 }
      }

      if (! found) {
	 permissive_bond_order = true;
	 for (unsigned int ibond=0; ibond<bonds.size(); ibond++) {
	    if (bonds[ibond].matches(energy_type_1, energy_type_2,
				     bond_type, 
				     permissive_atom_type)) {
	       bond = bonds[ibond];
	       found = true;
	       break;
	    }
	 }
      } 
      
      if (! found) {
	 mess = "in get_bond() failed to find bond for energy types ";
	 mess += energy_type_1;
	 mess += " and ";
	 mess += energy_type_2;
	 mess += " with bond-order ";
	 mess += bond_type;
	 throw std::runtime_error(mess);
      }
   } else {
      mess = "in get_bond() failed to find energy types given "; 
      mess += energy_type_1;
      mess += " and ";
      mess += energy_type_2;
      mess += " with bond-order ";
      mess += bond_type;
      throw std::runtime_error(mess);
   }
   return bond;
} 

coot::energy_lib_angle
coot::energy_lib_t::get_angle(const std::string &atom_type_1,
			      const std::string &atom_type_2,
			      const std::string &atom_type_3) const {

   coot::energy_lib_angle default_angle;
   energy_angle_info_t angle_info;

   angle_info = get_angle(atom_type_1, atom_type_2, atom_type_3, false, false);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_3, atom_type_2, atom_type_1, false, false);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_1, atom_type_2, atom_type_3, true, false);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_3, atom_type_2, atom_type_1, true, false);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_1, atom_type_2, atom_type_3, false, true);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_3, atom_type_2, atom_type_1, false, true);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_1, atom_type_2, atom_type_3, true, true);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;
   angle_info = get_angle(atom_type_3, atom_type_2, atom_type_1, true, true);
   if (angle_info.status == energy_angle_info_t::OK)
      return angle_info.angle;

   // hack in some execptions
   if (atom_type_2 == "S2") {
      default_angle.angle = 90.0;
      return default_angle; }
   
   std::cout << "WARNING:: returning default angle for "
	     << atom_type_1 << " " << atom_type_2 << " " << atom_type_3 << std::endl;
   return default_angle;
}


// 
coot::energy_lib_t::energy_angle_info_t
coot::energy_lib_t::get_angle(const std::string &energy_type_1,
			      const std::string &energy_type_2,
			      const std::string &energy_type_3,
			      bool permissive_atom_1,
			      bool permissive_atom_3) const {

   coot::energy_lib_t::energy_angle_info_t angle_info;

   std::map<std::string, energy_lib_atom>::const_iterator it_1 = atom_map.find(energy_type_1);
   std::map<std::string, energy_lib_atom>::const_iterator it_2 = atom_map.find(energy_type_2);
   std::map<std::string, energy_lib_atom>::const_iterator it_3 = atom_map.find(energy_type_3);

   if (it_1 == atom_map.end() || it_2 == atom_map.end() || it_3 == atom_map.end()) {
      std::string mess;
      mess = "in get_angle() failed to find energy types given "; 
      mess += energy_type_1;
      mess += " and ";
      mess += energy_type_2;
      mess += " and ";
      mess += energy_type_3;

      angle_info.message = mess;
      angle_info.status  = energy_angle_info_t::ENERGY_TYPES_NOT_FOUND;
      return angle_info;

   } else { 
      for (unsigned int iangle=0; iangle<angles.size(); iangle++) {
// 	 std::cout << "searching for " << energy_type_1 << " " << energy_type_2 << " " << energy_type_3
// 		   << " " << permissive_atom_1 << " "  << permissive_atom_3 << "\n";
	 if (angles[iangle].matches(energy_type_1,
				    energy_type_2,
				    energy_type_3, permissive_atom_1, permissive_atom_3)) {
	    angle_info.message = "OK";
	    angle_info.status  = energy_angle_info_t::OK;
	    angle_info.angle = angles[iangle];
	    // 	    std::cout << "found " << angle_info.angle.angle << std::endl;
	    return angle_info;
	 }
      }
      std::string mess;
      mess = "in get_angle() failed to find matching angle given "; 
      mess += energy_type_1;
      mess += " and ";
      mess += energy_type_2;
      mess += " and ";
      mess += energy_type_3;
      angle_info.message = mess;
      angle_info.status  = energy_angle_info_t::ANGLE_NOT_FOUND;
      return angle_info;
   } 
} 


// Pass teh types of the 2 middle atoms
// Will throw an std::runtime_error if not found.
// 
coot::energy_lib_torsion
coot::energy_lib_t::get_torsion(const std::string &energy_type_2,
				const std::string &energy_type_3) const {

   std::map<std::string, energy_lib_atom>::const_iterator it_1 = atom_map.find(energy_type_2);
   std::map<std::string, energy_lib_atom>::const_iterator it_2 = atom_map.find(energy_type_3);

   if (it_1 == atom_map.end() || it_2 == atom_map.end()) {
      // std::cout << "providing default torsion - bad types! "
      // << energy_type_2 << " " << energy_type_3 << std::endl;
      throw std::runtime_error("bad types");
   } else {
      for (unsigned int itor=0; itor<torsions.size(); itor++) { 
	 if (torsions[itor].matches(energy_type_2, energy_type_3)) {
	    return torsions[itor];
	 } 
	 if (torsions[itor].matches(energy_type_3, energy_type_2)) { 
	    return torsions[itor];
	 }
      }
   }
   throw std::runtime_error("torsion for types not found in dictionary");
}
