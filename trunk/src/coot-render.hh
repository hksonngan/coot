
#ifndef COOT_RENDER
#define COOT_RENDER

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif // HAVE_VECTOR

#include "Cartesian.h"
#include "gl-matrix.h"

#include "generic-display-object.hh"

namespace coot { 
// info class for raster3d/povray

   class colour_t {
   public:
      std::vector<float> col;
   };
   //
   class ray_trace_molecule_info {
      float bond_thickness; 
      float density_thickness;
      float zoom;
   public:

      colour_t bones_colour;
      std::vector<std::pair<Cartesian, Cartesian> > density_lines;
      colour_t density_colour;
      // bond_lines and bond_colour have the same length
      std::vector<std::pair<Cartesian, Cartesian> > bond_lines;
      std::vector<std::pair<Cartesian, Cartesian> > bone_lines;
      std::vector<colour_t> bond_colour;
      std::vector<std::pair<Cartesian, colour_t> > atom;
      void render_molecule(std::ofstream &render_stream,
			   float bond_thickness,
			   float atom_radius,
			   float density_thickness,
			   float bone_thickness);
      void renderman_molecule(std::ofstream &render_stream,
			      float bond_thickness,
			      float atom_radius,
			      float density_thickness,
			      float bone_thickness);
      void povray_molecule(std::ofstream &render_stream,
			   float bond_thickness,
			   float density_thickness,
			   float atom_radius,
			   float zoom,
			   const Cartesian &view_centre,
			   const Cartesian &front_clipping_plane_point);
   };

   class raytrace_info_t {
      GL_matrix view_matrix;
      int window_width, window_height;
      int quality;
      void render_molecules(std::ofstream &render_stream);
      void renderman_molecules(std::ofstream &render_stream);
      void povray_molecules(std::ofstream &render_stream);
      void render_generic_objects(std::ofstream &render_stream) const;
      float bond_thickness; 
      float bone_thickness; 
      float density_thickness;
      float clipping;
      float atom_radius;
      std::vector<coot::generic_display_object_t> display_objects;

   public:
      std::vector<ray_trace_molecule_info> rt_mol_info;
      colour_t background_colour;
      float zoom;
      Cartesian view_centre;
      Cartesian camera_location;
      // front_clipping_plane_point: point on front clipping plane
      // that is shortest distance to view_centre
      Cartesian front_clipping_plane_point;
      colour_t background;
      bool raster3d_enable_shadows;
      
      raytrace_info_t(Cartesian centre_view_in, float zoom_in,
		      const colour_t &background_in,
		      int window_width_in,
		      int window_height_in,
		      float clipping_in,
		      float bond_thickness_in,
		      float bone_thickness_in,
		      float atom_radius_in,
		      float density_thickness_in) {
	 view_centre = centre_view_in;
	 zoom = zoom_in;
	 background = background_in;
	 window_width = window_width_in;
	 window_height = window_height_in;
	 quality = 8;
	 bond_thickness = bond_thickness_in;
	 bone_thickness = bone_thickness_in;
	 density_thickness = density_thickness_in;
	 clipping = clipping_in;
	 atom_radius = atom_radius_in;
	 raster3d_enable_shadows = 1;
      }
      void set_view_matrix(GL_matrix view_matrix_in) {
	 view_matrix = view_matrix_in;
      }
      void set_camera_location(const Cartesian &camera_in) {
	 camera_location = camera_in;
      }

      void set_front_clipping_plane_point(const Cartesian &fcpp) {
	 front_clipping_plane_point = fcpp;
      }

      void set_raster3d_enable_shadows(bool state) {
	 raster3d_enable_shadows = state;
      } 
      
      void add_display_objects(const std::vector<coot::generic_display_object_t> &display_objects_in) {
	 for (unsigned int i=0; i<display_objects_in.size(); i++) {
	    display_objects.push_back(display_objects_in[i]);
	 } 
      }
      void add_display_object(const coot::generic_display_object_t &display_object_in) {
	 display_objects.push_back(display_object_in);
      } 
      
      // iq is either 8 (normal) 16 (good) 24 (best)
      // 
      void set_quality(int iq) { quality = iq;}

      int povray_ray_trace(std::string filename);
      int render_ray_trace(std::string filename);
      int renderman_render(std::string filename);
      
   };
}



#endif // have COOT_RENDER_HH
