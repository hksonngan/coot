
#ifndef CARTESIAN_H
#define CARTESIAN_H

#include <iostream>
#include <fstream>

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif

// #include "cos-sin.h"



enum surface_face_data { NO_FACE = 0, X_FACE = 1, Y_FACE = 2, Z_FACE = 3 };

namespace coot { 

   class Cartesian { 

      float x_, y_, z_; 

   public: 

      // inlines (of disaproved of funtions). 
      //
      float get_x() const { return x_;};
      float get_y() const { return y_;};
      float get_z() const { return z_;};
   
      // actually, I don't like the gets part of the variable name.
      float x() const { return x_;};
      float y() const { return y_;};
      float z() const { return z_;};

      Cartesian(float a, float b, float c); 
      Cartesian(void);

      void set_them(float a, float b, float c) {   // tmp function
	 x_ = a; y_ = b; z_ = c; }


      float amplitude(void) const;
      float amplitude_squared(void) const;
      float length(void) const { return amplitude(); }
      short int  normalize();  // return success status 0: fails, 1: OK

      Cartesian operator+(const Cartesian &) const;
      Cartesian operator-(const Cartesian &) const; 

      void operator+=(const Cartesian &);
      void operator-=(const Cartesian &);
      void operator*=(float scale);
      void operator/=(float scale);

      Cartesian by_scalar(float scale); 

      Cartesian operator=(const Cartesian &in);
   
      void unit_vector_yourself() {
	 float length = (*this).amplitude();
	 (*this) /= length; 
      } 

      float distance_to_line(const Cartesian &front, const Cartesian &back) const;
      int  within_box (const Cartesian &front, const Cartesian &back) const;

      Cartesian mid_point(const Cartesian &other) const; 

      // The following functions needed for use in mgtree
      // 
      static float LineLength(const Cartesian &a,
			      const Cartesian &b);
      static double Angle(const Cartesian &a,
			  const Cartesian &b,
			  const Cartesian &c);  // radians
      static double DihedralAngle(const Cartesian &a,
				  const Cartesian &b,
				  const Cartesian &c,
				  const Cartesian &d); // radians

      /*    static Cartesian GetCartFrom3Carts(const Cartesian &Atom1, */
      /* 				      const Cartesian &Atom2, */
      /* 				      const Cartesian &Atom3, */
      /* 				      double blength, */
      /* 				      double angle1, */
      /* 				      double angle2, */
      /* 				      int chiral=0); */

      static Cartesian GetCartFrom3Carts(const Cartesian &Atom1, double blength, 
					 const Cartesian &Atom2, double angle1, 
					 const Cartesian &Atom3, double angle2, 
					 int chiral=0);

      static Cartesian GetCartFrom3Carts_intermediate(const Cartesian &Atom1,
						      const Cartesian &Atom2,
						      const Cartesian &Atom3,
						      double blength,
						      double angle1,
						      double angle2,
						      int chiral=0); 

      static Cartesian position_by_torsion(const Cartesian &Atom_1, 
					   const Cartesian &Atom_2, 
					   const Cartesian &Atom_3,
					   float theta_2, float torsion, float dist); 

      static Cartesian CrossProduct(const Cartesian &Atom_1, 
				    const Cartesian &Atom_2); 

      std::vector<Cartesian> third_points(const Cartesian &other) const;

   

      // should be static members?
      friend double  cos_angle_btwn_vecs(const Cartesian &a, const Cartesian &b);
      friend float           dot_product(const Cartesian &a, const Cartesian &b);
      friend Cartesian     cross_product(const Cartesian &a, const Cartesian &b);
      friend surface_face_data on_a_face(const Cartesian &a, const Cartesian &b);

      friend std::ostream&  operator<<(std::ostream&, Cartesian);
      friend std::ofstream& operator<<(std::ofstream&, Cartesian);

   };

        
   class CartesianPair { 

      Cartesian start, finish;

   public:
      CartesianPair(const Cartesian &start_, const Cartesian &finish_);
      CartesianPair(void);
      void extentsBox(Cartesian centre, float dist) { 
	 start.set_them(centre.get_x() - dist, 
			centre.get_y() - dist, 
			centre.get_z() - dist); 
	 finish.set_them(centre.get_x() + dist, 
			 centre.get_y() + dist, 
			 centre.get_z() + dist);
      }

      friend std::ostream&  operator<<(std::ostream&,  CartesianPair);
      friend std::ofstream& operator<<(std::ofstream&, CartesianPair);

      // We don't approve of get functions, but the alternative is that
      // Cartesian, CartesianPair and Bond_lines get merged - and I 
      // don't want to do that.
      // 
      Cartesian getStart()  const { return start; }
      Cartesian getFinish() const { return finish;}
   };

   short int is_an_in_triangle(surface_face_data face,  const Cartesian &b,
			       const Cartesian &c); 
   // declared as friends above, now declare for real
   double  cos_angle_btwn_vecs(const Cartesian &a, const Cartesian &b);
   float           dot_product(const Cartesian &a, const Cartesian &b);
   Cartesian     cross_product(const Cartesian &a, const Cartesian &b);


   class CartesianPairInfo { 

   public:

      CartesianPair *data;
      int size;
      
   };


} // namespace coot

#endif // CARTESIAN_H
