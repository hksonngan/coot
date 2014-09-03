/*
     pygl/symmetry.cc: CCP4MG Molecular Graphics Program
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


#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <string.h>
#include <mman_manager.h>
#include <mmdb_atom.h>
#include <mmdb_cryst.h>
#include "cartesian.h"
#include "plane.h"
#include "volume.h"
#include "symmetry.h"
#include "matrix.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#define PIBY2 (M_PI * 2)
#endif

#ifdef _WIN32
#define rint(X) (((X-floor(X))<0.5)?floor(X):ceil(X))
#if !defined (__GNUC__)
#define snprintf _snprintf
#endif
#endif

mmdb::PPAtom molecule_extents_t::trans_sel(CMMDBCryst *my_cryst, symm_trans_t symm_trans)  const{

   mmdb::Atom atom;
   mmdb::PPAtom trans_selection = new mmdb::PAtom[6];
   mat44 my_matt;
   
   // Modify my_matt so that it is a coordinate transformation
   // matrix.
   //
   my_cryst->GetTMatrix(my_matt, symm_trans.isym(), symm_trans.x(),
		       symm_trans.y(), symm_trans.z());
   // mmdb::Atom tmp[10];

   for (int ii=0; ii<6; ii++) {
      trans_selection[ii] = new mmdb::Atom;
      trans_selection[ii]->SetCoordinates(extents_selection[ii]->x,
					  extents_selection[ii]->y,
					  extents_selection[ii]->z,
					  1.0, 99.9);
      trans_selection[ii]->Transform(my_matt);
   }
   return trans_selection;
}

void Symmetry::AddSymmetry(float symm_distance) {

   if (nSelAtoms>0) {
      mmdb::PAtom point_atom_p = new mmdb::Atom;
      point_atom_p->SetCoordinates(point.get_x(), point.get_y(),
                                   point.get_z(), 1.0, 99.9);

      for(unsigned int ii=0; ii<symm_trans.size(); ii++) {

         mmdb::PPAtom trans_selection = trans_sel(symm_trans[ii]);
	 symmetries.push_back(trans_selection);
      }
      delete point_atom_p;
   }
}


Cartesian molecule_extents_t::get_centre() {

   return centre;
} 

Cartesian molecule_extents_t::get_top() {

   return top;
} 

Cartesian molecule_extents_t::get_bottom() {

   return bottom;
} 

Cartesian molecule_extents_t::get_left() {

   return left;
} 

Cartesian molecule_extents_t::get_right() {

   return front;
} 

Cartesian molecule_extents_t::get_front() {

   return front;
} 

Cartesian molecule_extents_t::get_back() {
   // to where you one belonged.

   return back;
} 

molecule_extents_t::molecule_extents_t(mmdb::PPAtom SelAtoms, int nSelAtoms) {

   float atom_x, atom_y, atom_z;
   float max_x, max_y, max_z, min_x, min_y, min_z;

   max_x = -99999999;
   max_y = -99999999;
   max_z = -99999999;
   
   min_x = 99999999;
   min_y = 99999999;
   min_z = 99999999;
   

   if (nSelAtoms > 0 ) {

      // we need to reset these, lets not rely on what they used to be.

      for (int i=0; i< nSelAtoms; i++) {

	 atom_x = SelAtoms[i]->x;
	 atom_y = SelAtoms[i]->y;
	 atom_z = SelAtoms[i]->z;

	 // if there is only one atom, it will be all the limits,
	 // so we don't use the else.
	 //
	 if (atom_x > max_x) {
	    max_x = atom_x;
	    right = Cartesian(atom_x, atom_y, atom_z);
	 }

	 if (atom_x < min_x) {
	    min_x = atom_x; 
	    left = Cartesian(atom_x, atom_y, atom_z);
	 } 

	 if (atom_y > max_y) { 
	    max_y = atom_y;
	    top = Cartesian(atom_x, atom_y, atom_z);
	 } 

	 if (atom_y < min_y) {
	    min_y = atom_y;
	    bottom = Cartesian(atom_x, atom_y, atom_z);
	 } 

	 if (atom_z > max_z) { // back is at max_z;
	    max_z = atom_z;
	    back = Cartesian(atom_x, atom_y, atom_z);
	 } 

	 if (atom_z < min_z) {  // front is at min_z;
	    min_z = atom_z;
	    front = Cartesian(atom_x, atom_y, atom_z);
	 }
      }
   }

   float mid_x, mid_y, mid_z;
   mid_x = (left.get_x()+right.get_x())*0.5;
   mid_y = (bottom.get_y()+top.get_y())*0.5;
   mid_z = (front.get_z()+back.get_z())*0.5;

   // Adjust the extents so that they are on the mippoints of the other axes.
   //
   left   = Cartesian( left.get_x() - 5.0, mid_y, mid_z);
   right  = Cartesian(right.get_x() + 5.0, mid_y, mid_z);
   front  = Cartesian(mid_x, mid_y, front.get_z() - 5.0);
   back   = Cartesian(mid_x, mid_y,  back.get_z() + 5.0);
   bottom = Cartesian(mid_x, bottom.get_y() - 5.0, mid_z);
   top    = Cartesian(mid_x, top.get_y()    + 5.0, mid_z);

   // now make the centre for the above coordinates
   // just for reference
   centre = front + back + left + right + back + front;
   centre = Cartesian(centre.get_x()*.16666666,centre.get_y()*.16666666,centre.get_z()*.16666666);

   // std::cout << "centre at: " << centre << std::endl;
   
   extents_selection = new mmdb::PAtom[6];

   extents_selection[0] = new mmdb::Atom;
   extents_selection[0]->SetCoordinates(front.get_x(), front.get_y(),
				       front.get_z(), 1.0 ,99.9);

   extents_selection[1] = new mmdb::Atom; // back is at max_z;
   extents_selection[1]->SetCoordinates(back.get_x(), back.get_y(),
					back.get_z(), 1.0 ,99.9);

   extents_selection[2] = new mmdb::Atom;
   extents_selection[2]->SetCoordinates(left.get_x(), left.get_y(),
					left.get_z(), 1.0 ,99.9);

   extents_selection[3] = new mmdb::Atom;
   extents_selection[3]->SetCoordinates(right.get_x(), right.get_y(),
					right.get_z(), 1.0 ,99.9);

   extents_selection[4] = new mmdb::Atom;
   extents_selection[4]->SetCoordinates(bottom.get_x(), bottom.get_y(),
					bottom.get_z(), 1.0 ,99.9);

   extents_selection[5] = new mmdb::Atom;
   extents_selection[5]->SetCoordinates(top.get_x(), top.get_y(),
					top.get_z(), 1.0 ,99.9);

}

Cell_Translation::Cell_Translation(int a, int b, int c) {
   //
   us = a;
   vs = b;
   ws = c;

}

bool molecule_extents_t::point_is_in_box(Cartesian point, mmdb::PPAtom TransSel) const { 

   // front back left right bottom top
   //      z         x            y
   // 
   Cartesian  front(TransSel[0]->x, TransSel[0]->y, TransSel[0]->z);
   Cartesian   back(TransSel[1]->x, TransSel[1]->y, TransSel[1]->z);
   Cartesian   left(TransSel[2]->x, TransSel[2]->y, TransSel[2]->z);
   Cartesian  right(TransSel[3]->x, TransSel[3]->y, TransSel[3]->z);
   Cartesian bottom(TransSel[4]->x, TransSel[4]->y, TransSel[4]->z);
   Cartesian    top(TransSel[5]->x, TransSel[5]->y, TransSel[5]->z);

   Cartesian back_to_front = front - back;
   Cartesian left_to_right = right - left;
   Cartesian bottom_to_top = top - bottom;
   
   Cartesian back_to_point   = point - back;
   Cartesian left_to_point   = point - left;
   Cartesian bottom_to_point = point - bottom;

   Cartesian front_to_point = point - front;
   Cartesian right_to_point = point - right;
   Cartesian top_to_point   = point - top;

   if (front.DotProduct(back_to_front, back_to_point) >= 0.0) {
      if (front.DotProduct(left_to_right, left_to_point) >=0) { 
	 if (front.DotProduct(bottom_to_top, bottom_to_point) >=0) {

	    //
	    if (front.DotProduct(back_to_front, front_to_point) <= 0.0) {
	       if (front.DotProduct(left_to_right, right_to_point) <= 0.0) {
		  if (front.DotProduct(bottom_to_top, top_to_point) <= 0.0) {

		     return 1;
		  }
	       }
	    }
	 }
      }
   }
   return 0;
}
std::vector<Cartesian> Symmetry::GetUnitCell() const {

  std::vector<Cartesian> unit_cell;

  double array[16] = {my_cryst_p->RO[0][0], my_cryst_p->RO[0][1], my_cryst_p->RO[0][2], my_cryst_p->RO[0][3],
                      my_cryst_p->RO[1][0], my_cryst_p->RO[1][1], my_cryst_p->RO[1][2], my_cryst_p->RO[1][3],
                      my_cryst_p->RO[2][0], my_cryst_p->RO[2][1], my_cryst_p->RO[2][2], my_cryst_p->RO[2][3],
                      my_cryst_p->RO[3][0], my_cryst_p->RO[3][1], my_cryst_p->RO[3][2], my_cryst_p->RO[3][3] };

  matrix mat(4,4,array);

  unit_cell.push_back(mat*Cartesian(1,0,0));
  unit_cell.push_back(mat*Cartesian(0,1,0));
  unit_cell.push_back(mat*Cartesian(0,0,1));

  return unit_cell;

}

std::vector<int> Symmetry::GetSymmetryMatrixNumbers() const {
  std::vector<int> nos;
  for(unsigned int i=0;i<symm_trans.size();i++)
    nos.push_back(symm_trans[i].isym());
  return nos;
}

std::vector<matrix> Symmetry::GetSymmetryMatrices()  const{
  std::vector<matrix> symm_mats;
   mat44 my_matt;

   for(unsigned int i=0;i<symm_trans.size();i++){
     //std::cout << "GetSymmetryMatrices " << symm_trans[i].isym() << " " << symm_trans[i].x() << " " << symm_trans[i].y() << " " << symm_trans[i].z() << "\n";
     int err = my_cryst_p->GetTMatrix(my_matt, symm_trans[i].isym(), symm_trans[i].x(),
                                 symm_trans[i].y(), symm_trans[i].z());
     symm_mats.push_back(matrix(4,4));

     for(int j=0;j<4;j++)
       for(int k=0;k<4;k++)
           symm_mats.back()(k,j) = my_matt[k][j];

     if (err != 0)
        std::cout << "!!!!!!!!!!!!!! something BAD with CMMDBCryst.GetTMatrix" << std::endl;
   }
   
   return symm_mats;

}

mmdb::PPAtom Symmetry::trans_sel(const symm_trans_t &symm_tran) const{
   mat44 my_matt;
   int err = my_cryst_p->GetTMatrix(my_matt, symm_tran.isym(), symm_tran.x(),
                                 symm_tran.y(), symm_tran.z());
   if (err != 0) {
      std::cout << "!!!!!!!!!!!!!! something BAD with CMMDBCryst.GetTMatrix"
	   << std::endl;
   }

   mmdb::PPAtom trans_selection = new mmdb::PAtom[nSelAtoms];
   for (int ii=0; ii<nSelAtoms; ii++) {

      trans_selection[ii] = new mmdb::Atom;
      trans_selection[ii]->Copy(SelAtoms[ii]);
      trans_selection[ii]->residue = SelAtoms[ii]->residue; // Is this OK?
      trans_selection[ii]->Transform(my_matt);

   }
   return trans_selection;
}
std::vector<symm_trans_t> molecule_extents_t::GetUnitCellOps(PCMMANManager molhnd, int xshifts, int yshifts, int zshifts) {

   std::vector<symm_trans_t> symm_trans;

   mmdb::realtype u, v, w;
   PCMMDBCryst my_cryst_p = (CMMDBCryst *) &(molhnd->get_cell());
   my_cryst_p->Orth2Frac(0,0,0, u, v, w);
   Cell_Translation c_t = Cell_Translation(int (rint (u)), int (rint (v)), int (rint (w)));
   int n = my_cryst_p->GetNumberOfSymOps();

   if(xshifts>0||yshifts>0||zshifts>0){
     for(int ii=0; ii<n; ii++) {
       for(int x_shift = c_t.us-xshifts; x_shift<(1+c_t.us+xshifts); x_shift++) { 
         for(int y_shift = c_t.vs-yshifts; y_shift<(1+c_t.vs+yshifts); y_shift++) { 
           for(int z_shift = c_t.ws-zshifts; z_shift<(1+c_t.ws+zshifts); z_shift++) {
                  symm_trans_t s_t(ii, x_shift, y_shift, z_shift);
                  symm_trans.push_back(s_t);
           }
         }
       }
     }
   }else{
     std::vector<Cartesian> unit_cell;

     double array[16] = {my_cryst_p->RO[0][0], my_cryst_p->RO[0][1], my_cryst_p->RO[0][2], my_cryst_p->RO[0][3],
                         my_cryst_p->RO[1][0], my_cryst_p->RO[1][1], my_cryst_p->RO[1][2], my_cryst_p->RO[1][3],
                         my_cryst_p->RO[2][0], my_cryst_p->RO[2][1], my_cryst_p->RO[2][2], my_cryst_p->RO[2][3],
                         my_cryst_p->RO[3][1], my_cryst_p->RO[3][1], my_cryst_p->RO[3][2], my_cryst_p->RO[3][3] };

     matrix mat(4,4,array);

     unit_cell.push_back(mat*Cartesian(1,0,0));
     unit_cell.push_back(mat*Cartesian(0,1,0));
     unit_cell.push_back(mat*Cartesian(0,0,1));
     Volume vol;
     Plane p1(Cartesian(0,0,0),unit_cell[0],unit_cell[1]);
     Plane p2(Cartesian(0,0,0),unit_cell[1],unit_cell[2]);
     Plane p3(Cartesian(0,0,0),unit_cell[2],unit_cell[0]);
     Plane p4(unit_cell[2],unit_cell[1]+unit_cell[2],unit_cell[0]+unit_cell[2]);
     Plane p5(unit_cell[0],unit_cell[0]+unit_cell[2],unit_cell[0]+unit_cell[1]+unit_cell[2]);
     Plane p6(unit_cell[1],unit_cell[0]+unit_cell[1],unit_cell[0]+unit_cell[1]+unit_cell[2]);
     vol.AddPlane(p1);
     vol.AddPlane(p2);
     vol.AddPlane(p3);
     vol.AddPlane(p4);
     vol.AddPlane(p5);
     vol.AddPlane(p6);

     Cartesian bbl(left.get_x(),bottom.get_y(),back.get_z());
     Cartesian bbr(right.get_x(),bottom.get_y(),back.get_z());
     Cartesian fbl(left.get_x(),bottom.get_y(),front.get_z());
     Cartesian fbr(right.get_x(),bottom.get_y(),front.get_z());
     Cartesian btl(left.get_x(),top.get_y(),back.get_z());
     Cartesian btr(right.get_x(),top.get_y(),back.get_z());
     Cartesian ftl(left.get_x(),top.get_y(),front.get_z());
     Cartesian ftr(right.get_x(),top.get_y(),front.get_z());
     Cartesian centre = (top+bottom)*0.5;

     mat44 my_matt;
     for(int ii=0; ii<n; ii++) {
       for(int x_shift = (c_t.us-2); x_shift<(3+c_t.us); x_shift++) { 
         for(int y_shift = (c_t.vs-2); y_shift<(3+c_t.vs); y_shift++) { 
           for(int z_shift = (c_t.ws-2); z_shift<(3+c_t.ws); z_shift++) {
             if(1){
               /*int err = */my_cryst_p->GetTMatrix(my_matt, ii, x_shift, y_shift, z_shift);
               matrix pygl_mat(4,4);
               for(int j=0;j<4;j++)
                 for(int k=0;k<4;k++)
                    pygl_mat(k,j) = my_matt[k][j];
               if(vol.PointInVolume(pygl_mat*bbl)||vol.PointInVolume(pygl_mat*bbr)||vol.PointInVolume(pygl_mat*fbl)||vol.PointInVolume(pygl_mat*fbr)
                ||vol.PointInVolume(pygl_mat*btl)||vol.PointInVolume(pygl_mat*btr)||vol.PointInVolume(pygl_mat*ftl)||vol.PointInVolume(pygl_mat*ftr)
                ||vol.PointInVolume(pygl_mat*front)||vol.PointInVolume(pygl_mat*back)||vol.PointInVolume(pygl_mat*top)||vol.PointInVolume(pygl_mat*bottom)
                ||vol.PointInVolume(pygl_mat*left)||vol.PointInVolume(pygl_mat*right)||vol.PointInVolume(pygl_mat*centre)){
                  symm_trans_t s_t(ii, x_shift, y_shift, z_shift);
                  symm_trans.push_back(s_t);
               }else if(vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(bbl,bbr)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(fbl,fbr)))
                  ||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(btl,btr)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(ftl,ftr)))
                  ||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(bbl,btl)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(fbl,ftl)))
                  ||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(bbr,btr)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(fbr,ftr)))
                  ||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(bbl,fbl)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(btl,ftl)))
                  ||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(bbr,fbr)))||vol.PointInVolume(pygl_mat*(Cartesian::MidPoint(btr,ftr)))
                  ){
                  symm_trans_t s_t(ii, x_shift, y_shift, z_shift);
                  symm_trans.push_back(s_t);
               }
             }
           }
         }
       }
     }
   }

   return symm_trans;

}

std::vector<symm_trans_t> molecule_extents_t::which_box(Cartesian point, PCMMANManager molhnd, mmdb::PPAtom SelAtoms, int nSelAtoms, Cartesian tl, Cartesian tr, Cartesian br, Cartesian bl) {

   std::vector<symm_trans_t> symm_trans;
   Cartesian p;
   
   mmdb::realtype u, v, w;
   PCMMDBCryst my_cryst_p = (CMMDBCryst *) &(molhnd->get_cell());

   my_cryst_p->Orth2Frac(point.get_x(), point.get_y(), point.get_z(), u, v, w);
   Cell_Translation c_t = Cell_Translation(int (rint (u)), int (rint (v)), int (rint (w)));

   int n = my_cryst_p->GetNumberOfSymOps();

   for (int ii=0; ii<n; ii++) {
	       
      for(int x_shift = -1+c_t.us; x_shift<(2+c_t.us); x_shift++) { 
	 for(int y_shift = -1+c_t.vs; y_shift<(2+c_t.vs); y_shift++) { 
	    for(int z_shift = -1+c_t.ws; z_shift<(2+c_t.ws); z_shift++) {

	       // don't check for symmetry where the model is.
	       // 
	       if ( ! (x_shift == 0 && y_shift == 0 && z_shift == 0 && ii==0)) {

		  symm_trans_t s_t(ii, x_shift, y_shift, z_shift);
		  
		  mmdb::PPAtom trans_selection = trans_sel(my_cryst_p, s_t);
		  p = point;
		  bool in = point_is_in_box(p, trans_selection);
		  if (in == 1) {
		    symm_trans.push_back(s_t);
		  }
                  for (int ii=0; ii<6; ii++) {
		    delete trans_selection[ii];
                  }
		  delete [] trans_selection;
	       }
	    }
	 }
      }
   }

   return symm_trans;
}

bool symm_trans_t::is_identity() {

   if ( (symm_no == 0) && (x_shift_ == 0) &&
	(y_shift_ == 0) && (z_shift_ == 0)) {

      return 1;
   } else {

      return 0;
   }
}

// This is an utter mess
//
std::string symm_trans_t::str() {

   //
   char *t, *t_start;
   int i;
   t = new char[30];
   t_start = t;
   
   snprintf(t,20,"%-4d", symm_no);
   for (int a=0; a<20; a++) if (t[a] == ' ') t[a] = '\0';
   i = strlen(t);
   t[i] = ':';
   t += i+1;

   snprintf(t,20,"%-4d", x_shift_);
   for (int a=0; a<20; a++) if (t[a] == ' ') t[a] = '\0';
   i = strlen(t);
   t[i] = ':';
   t += i+1;
   
   snprintf(t,20,"%-4d", y_shift_);
   for (int a=0; a<20; a++) if (t[a] == ' ') t[a] = '\0';
   i = strlen(t);
   t[i] = ':';
   t += i+1;
   
   snprintf(t,20,"%-4d", z_shift_);
   for (int a=0; a<20; a++) if (t[a] == ' ') t[a] = '\0';
   i = strlen(t);
   t[i] = '\0';

   // delete t_start;
   //
   std::string b = std::string(" #s ") + t_start;
   delete t_start;
   
   return b;
} 

molecule_extents_t::~molecule_extents_t(){
  delete extents_selection[0];
  delete extents_selection[1];
  delete extents_selection[2];
  delete extents_selection[3];
  delete extents_selection[4];
  delete extents_selection[5];
  delete [] extents_selection;
}


void Symmetry::clear_symmetries(){
  for(unsigned int i=0;i<symmetries.size();i++){
    for (int ii=0; ii<nSelAtoms; ii++) {
      delete symmetries[i][ii];
    }
    delete [] symmetries[i];
  }
  symmetries.clear();
}

Symmetry::~Symmetry(){
  clear_symmetries();
  symm_trans.clear();
}

Symmetry::Symmetry(PCMMANManager molhnd_in, mmdb::PPAtom SelAtoms_in, int nSelAtoms_in, Cartesian point_in, Cartesian tl_in, Cartesian tr_in, Cartesian br_in, Cartesian bl_in, int draw_unit_cell, int xshifts, int yshifts, int zshifts){
  molhnd = molhnd_in;
  SelAtoms = SelAtoms_in;
  clear_symmetries();
  nSelAtoms = nSelAtoms_in;

  point = point_in;

  tl = tl_in;
  tr = tr_in;
  br = br_in;
  bl = bl_in;

  symm_trans.clear();
  my_cryst_p = (CMMDBCryst *) &(molhnd->get_cell());

  molecule_extents_t extents(SelAtoms, nSelAtoms);

  if(draw_unit_cell)
    symm_trans =  extents.GetUnitCellOps(molhnd, xshifts, yshifts, zshifts);
  else
    symm_trans =  extents.which_box(point, molhnd, SelAtoms, nSelAtoms,tl,tr,br,bl);

}

std::vector<mmdb::PPAtom> Symmetry::GetSymmetries(){

  if(symm_trans.size()>0){
      AddSymmetry(100.0);
  }

  return symmetries;
}

unsigned int Symmetry::GetNumSymmetries(){
  if(symmetries.size()==0)
    symmetries = GetSymmetries();
  return symmetries.size();
}

mmdb::PPAtom Symmetry::GetSymmetry(int nsym){
  if(symmetries.size()==0)
    symmetries = GetSymmetries();
  return symmetries[nsym];
}



