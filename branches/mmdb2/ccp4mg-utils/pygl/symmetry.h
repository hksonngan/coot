/*
     pygl/symmetry.h: CCP4MG Molecular Graphics Program
     Copyright (C) 2001-2008 University of York, CCLRC

     This library is free software: you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public License
     version 3, modified in accordance with the provisions of the 
     license to address the requirements of UK law.
 
     You should have received a copy of the modified GNU Lesser General 
     Public License along with this library.  If not, copies may be 
     downloaded from http://www.ccp4.ac.uk/ccp4license.php
 
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU Lesser General Public License for more details.
*/


#ifndef _CCP4MG_SYM_
#define _CCP4MG_SYM_

#include <vector>
#include <string>
#include <math.h>
#include <mman_manager.h>
#include <mmdb_atom.h>
#include <mmdb_cryst.h>
#include "cartesian.h"

#include "matrix.h"

class symm_trans_t { 

   int symm_no, x_shift_, y_shift_, z_shift_;

 public:
   symm_trans_t(int n, int x, int y, int z) 
      { symm_no = n; x_shift_ = x; y_shift_ = y; z_shift_ = z;};

   int isym() const { return symm_no;};
   int x()    const { return x_shift_;};
   int y()    const { return y_shift_;};
   int z()    const { return z_shift_;};

   bool is_identity();

   //
   std::string str();

};

class molecule_extents_t { 

   // coordinates of the most limitting atoms in the faces. 
   //
   Cartesian front, back, left, right, top, bottom, centre;
   // front, back, minimum and maximum in z;
   // left, right, minimum and maximum in x;
   // top, bottom, minimum and maximum in y;

   mmdb::PPAtom extents_selection;

 public:

   molecule_extents_t(mmdb::PPAtom SelAtoms, int nSelAtoms); 
   ~molecule_extents_t();
   Cartesian get_front(); 
   Cartesian get_back(); 
   Cartesian get_left(); 
   Cartesian get_right(); 
   Cartesian get_top(); 
   Cartesian get_bottom(); 
   Cartesian get_centre(); 

   //Cell_Translation 
      //coord_to_unit_cell_translations(Cartesian point,
				      //atom_selection_container_t AtomSel); 

   std::vector<symm_trans_t> which_box(Cartesian point,PCMMANManager molhnd, mmdb::PPAtom SelAtoms, int nSelAtoms, Cartesian tl, Cartesian tr, Cartesian br, Cartesian bl);
   std::vector<symm_trans_t> GetUnitCellOps(PCMMANManager molhnd, int xshifts, int yshifts, int zshifts) ;

   mmdb::PPAtom trans_sel(CMMDBCryst *my_cryst, symm_trans_t symm_trans) const;

   bool point_is_in_box(Cartesian point, mmdb::PPAtom TransSel) const;

};


class Cell_Translation { 

 public: 

   int us, vs, ws; 
   
   Cell_Translation(int a, int b, int c);
 
}; 

class Symmetry {
    std::vector<symm_trans_t> symm_trans;
    PCMMANManager molhnd;
    mmdb::PPAtom SelAtoms;
    int nSelAtoms;
    Cartesian point;
    Cartesian tl;
    Cartesian tr;
    Cartesian br;
    Cartesian bl;
    PCMMDBCryst my_cryst_p;
    std::vector<mmdb::PPAtom> symmetries;
    void clear_symmetries();
  public:
    Symmetry(CMMANManager *molhnd_in, mmdb::PPAtom SelAtoms_in, int nSelAtoms_in, Cartesian point_in, Cartesian tl_in, Cartesian tr_in, Cartesian br_in, Cartesian bl_in, int draw_unit_cell=0, int xshifts=0, int yshifts=0, int zshifts=0);
    ~Symmetry();
    mmdb::PPAtom trans_sel(const symm_trans_t &symm_tran) const;
    void AddSymmetry(float symm_distance);
    std::vector<mmdb::PPAtom> GetSymmetries();
    std::vector<symm_trans_t> GetSymmTrans(void){return symm_trans;};

    std::vector<matrix> GetSymmetryMatrices() const;
    std::vector<int> GetSymmetryMatrixNumbers() const;
    std::vector<Cartesian> GetUnitCell() const;
    mmdb::PPAtom GetSymmetry(int nsym);
    unsigned int GetNumSymmetries();
    
};

#endif
