
#ifndef RESIDUE_AND_ATOM_SPEC_HH
#define RESIDUE_AND_ATOM_SPEC_HH

#include <sstream>
#ifndef __MMDB_Defs__
#include <mmdb2/mmdb_manager.h>
#endif

namespace coot {
   
   class atom_spec_t {
   public:
      std::string chain_id;
      int res_no;
      std::string ins_code;
      std::string atom_name;
      std::string alt_conf;
      int int_user_data;
      float float_user_data;
      std::string string_user_data;
      int model_number;
      atom_spec_t() {
	 chain_id = "unset";
	 res_no = mmdb::MinInt4;
	 ins_code = "";
	 model_number = -1;
	 int_user_data = -1;
      }
      atom_spec_t(const std::string &chain_in,
		  int resno_in,
		  const std::string &insertion_code_in,
		  const std::string &atom_name_in,
		  const std::string &alt_conf_in) {
	 chain_id = chain_in;
	 res_no = resno_in;
	 ins_code = insertion_code_in;
	 atom_name = atom_name_in;
	 alt_conf = alt_conf_in;
	 model_number = 1;
	 int_user_data = -1;
      }
      // This presumes at is a member of a coordinate hierarchy.
      atom_spec_t(mmdb::Atom *at) {
	 if (at) { 
	    chain_id       = at->GetChainID();
	    res_no         = at->GetSeqNum();
	    ins_code       = at->GetInsCode();
	    model_number   = at->GetModelNum();
	    atom_name      = at->name;
	    alt_conf       = at->altLoc;
	 } else {
	    chain_id = "unset";
	    res_no = mmdb::MinInt4;
	    ins_code = "";
	    model_number = -1;
	    int_user_data = -1;
	 }
	 int_user_data = -1; // mark as "unset" (better than not setting it)
      }
      // This presumes at is a member of a coordinate hierarchy.
      atom_spec_t(mmdb::Atom *at, const std::string &user_data_string) {
	 model_number = at->GetModelNum();
	 chain_id = at->GetChainID();
	 res_no = at->GetSeqNum();
	 ins_code = at->GetInsCode();
	 atom_name = at->name;
	 alt_conf = at->altLoc;
	 string_user_data = user_data_string;
      }

      bool empty() const {
	 if (res_no == mmdb::MinInt4)
	    return true;
	 else
	    return false;
      }

      void selectatoms(mmdb::Manager *mol, int SelHnd) {
	 const char *chainid = chain_id.c_str();
	 const char *inscode = ins_code.c_str();
	 const char *atname  = atom_name.c_str(); // atom name
	 const char *altconf = alt_conf.c_str();
	 
	 mol->SelectAtoms(SelHnd, 0, chainid, res_no, inscode, res_no, inscode,
			  "*", atname, "*", altconf);
      }

      // Presumes that atom can get to SeqNum() and InsCode()? Need
      // tested against a residue not in a hierarchy.
      bool matches_spec(mmdb::Atom *atom) const;

#ifndef SWIG
      bool operator==(const atom_spec_t &matcher) const {
	 bool r = false;
	 if (matcher.model_number == model_number) { 
	    if (matcher.chain_id == chain_id) {
	       if (matcher.res_no == res_no) {
		  if (matcher.ins_code == ins_code) {
		     if (matcher.atom_name == atom_name) {
			if (matcher.alt_conf == alt_conf) {
			   r = true;
			}
		     }
		  }
	       }
	    }
	 }
	 return r;
      }
#endif 

#ifndef SWIG
      // we need this if atom_spec_t are used in a std::map.
      bool operator<(const atom_spec_t &matcher) const {
	 if (matcher.empty())
	    return false; 
	 if (empty())
	    return true; 
	 if (matcher.model_number < model_number) {
	    return true;
	 } else {
	    if (matcher.chain_id < chain_id) {
	       return true;
	    } else { 
	       if (matcher.res_no < res_no) {
		  return true;
	       } else { 
		  if (matcher.ins_code < ins_code) {
		     return true;
		  } else { 
		     if (matcher.atom_name < atom_name) {
			return true;
		     } else { 
			if (matcher.alt_conf < alt_conf) {
			   return true; 
			}
		     }
		  }
	       }
	    }
	 }
	 return false;
      }
#endif // SWIG      
      
#ifndef SWIG      
      friend std::ostream& operator<< (std::ostream& s, const atom_spec_t &spec);
#endif // SWIG      
   };
   
   bool compare_atom_specs_user_float(const atom_spec_t &a1,
				      const atom_spec_t &a2);
   bool compare_atom_specs_user_float_in_pair(const std::pair<atom_spec_t, std::string> &a,
					      const std::pair<atom_spec_t, std::string> &b);
   std::pair<atom_spec_t, atom_spec_t> link_atoms(mmdb::Link *link);

   class residue_spec_t {
   public:
      int model_number;
      std::string chain_id;
      int res_no;
      std::string ins_code;
      int int_user_data;
      residue_spec_t(int r) {
	 res_no = r;
	 chain_id = "";
	 ins_code = "";
	 int_user_data = -1;
      }
      residue_spec_t(const std::string &chain_in, int r) {
	 model_number = mmdb::MinInt4;
	 res_no = r;
	 chain_id = chain_in;
	 ins_code = "";
	 int_user_data = -1;
      }
      residue_spec_t(int model_number_in,
		     const std::string &chain_in, int r,
		     const std::string &ins_code_in) {
	 model_number = model_number_in;
	 res_no = r;
	 chain_id = chain_in;
	 ins_code = ins_code_in;
	 int_user_data = -1;
      }
      residue_spec_t(const std::string &chain_in, int r,
		     const std::string &ins_code_in) {
	 model_number = mmdb::MinInt4;
	 res_no = r;
	 chain_id = chain_in;
	 ins_code = ins_code_in;
	 int_user_data = -1;
      }
      residue_spec_t(mmdb::Residue *res) {
	 if (! res) {
	    chain_id = "";
	    model_number = mmdb::MinInt4;
	    res_no = mmdb::MinInt4;
	    ins_code = "";
	 } else { 
	    chain_id = res->GetChainID();
	    model_number = res->GetModelNum();
	    res_no = res->GetSeqNum();
	    ins_code = res->GetInsCode();
	 }
	 int_user_data = -1;
      } 
      residue_spec_t(const atom_spec_t &atom_spec) { 
	 model_number = mmdb::MinInt4;
         chain_id = atom_spec.chain_id;
         res_no = atom_spec.res_no;
         ins_code = atom_spec.ins_code;
	 int_user_data = -1;
      }
      // This one for coot_wrap_guile
      residue_spec_t() {
	 model_number = mmdb::MinInt4;
	 res_no = mmdb::MinInt4;
	 chain_id = "";
	 ins_code = "";
	 int_user_data = -1;
      }
      bool unset_p() const {
	 bool u = true;
	 if (res_no != mmdb::MinInt4)
	    u = false;
	 return u;
      }
      residue_spec_t next() const {
	 residue_spec_t r = *this;
	 if (res_no != mmdb::MinInt4)
	    r.res_no += 1;
	 return r;
      }
      residue_spec_t previous() const {
	 residue_spec_t r = *this;
	 if (res_no != mmdb::MinInt4)
	    r.res_no -= 1;
	 return r;
      }
#ifndef SWIG
      bool operator==(const residue_spec_t &matcher) const {
	 if (matcher.chain_id == chain_id) {
	    if (matcher.res_no == res_no) {
	       if (matcher.ins_code == ins_code) {
		  return 1; 
	       }
	    }
	 }
	 return 0;
      }
#endif // SWIG      
      
#ifndef SWIG
      bool operator<(const residue_spec_t &matcher) const{
	 if (matcher.chain_id == chain_id) {
	    if (matcher.res_no == res_no) {
	       if (matcher.ins_code == ins_code) {
		  return 0; 
	       } else {
		  if (matcher.ins_code < ins_code)
		     return 0;
		  else
		     return 1;
	       }
	    } else {
	       if (matcher.res_no < res_no)
		  return 0;
	       else
		  return 1;
	    } 
	 } else {
	    if (matcher.chain_id < chain_id)
	       return 0;
	    else
	       return 1;
	 } 
	 return 0;
      }
#endif // SWIG      

      std::string format() const {
	 std::ostringstream s;
	 if (!(s << *this))
	    return "";
	 return s.str();
      }

      // return an atom selection handle for the selection in the mol
      // that matches the spec.  Caller is responsible for deleting
      // the atom selection.
      //
      // selection_key_type is typically either mmdb::SKEY_NEW or mmdb::SKEY_OR
      // 
      int select_atoms(mmdb::Manager *mol, int selhnd,
		       mmdb::SELECTION_KEY selection_key);

#ifndef SWIG      
      friend std::ostream& operator<< (std::ostream& s, const residue_spec_t &spec);
#endif // SWIG      
   };

}

#endif // RESIDUE_AND_ATOM_SPEC_HH
