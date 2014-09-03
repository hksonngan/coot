

#ifndef ELASTIC_HH
#define ELASTIC_HH

#include <mmdb2/mmdb_manager.h>
#include <string>
#include <vector>

namespace coot { 
   class elastic_network_item_t {
   public:
      mmdb::Atom *at_1;
      mmdb::Atom *at_2;
      double spring_constant;
      elastic_network_item_t(mmdb::Atom *at1, mmdb::Atom *at2, double c) {
	 at_1 = at1;
	 at_2 = at2;
	 spring_constant = c;
      }
      elastic_network_item_t() {
	 at_1 = NULL;
	 at_2 = NULL;
      }
   };
   
   class elastic_network_model_t {
      std::vector<elastic_network_item_t> d;
   public:
      elastic_network_model_t() {}
      elastic_network_model_t(CMMDBManager *mol, int atom_selection_handle,
			      realtype min_dist,
			      realtype max_dist,
			      int max_n_distances);
   };

   void test_elastic();
}


#endif // ELASTIC_HH
