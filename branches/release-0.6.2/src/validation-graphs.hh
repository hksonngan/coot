
namespace coot {

   class validation_graphs_t {
   public:
      GtkWidget *dynarama_is_displayed;
      GtkWidget *sequence_view_is_displayed;
      GtkWidget *geometry_graph;
      GtkWidget *b_factor_variance_graph;
      GtkWidget *residue_density_fit_graph;
      GtkWidget *omega_distortion_graph;
      GtkWidget *rotamer_graph; 
      GtkWidget *ncs_diffs_graph;
      validation_graphs_t() {
	 init();
      }
      
      void init() {
	 dynarama_is_displayed      = NULL;
	 sequence_view_is_displayed = NULL;
	 geometry_graph             = NULL;
	 b_factor_variance_graph    = NULL;
	 residue_density_fit_graph  = NULL;
	 omega_distortion_graph     = NULL;
	 rotamer_graph              = NULL;
	 ncs_diffs_graph            = NULL;
      }
   };
}
