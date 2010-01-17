/* src/molecule-class-info-ribbons.cc
 * 
 * Copyright 2008 by The University of York
 * Author: Bernhard Lohkamp
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <iostream>
#include <vector>

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

// guessing for now as I lost the orig file....
#include "mattype_.h"
#include "mman_manager.h"
#include "splineinfo.h"
#include "atom_util.h"
 
#include "cbuild.h"
#include "cdisplayobject.h"

#include "CParamsManager.h"
#include "mg_colour.h"
#include "mmut_connectivity.h"

#include "graphics-info.h"

#include "molecule-class-info.h"
#include "sphere.h"


void
molecule_class_info_t::make_ribbons() {

  InitMatType();
  graphics_info_t g;
  std::string mon_lib_dir = g.master_mon_lib_dir;
  std::cout <<"BL DEBUG:: mon_lib_dir is " << mon_lib_dir<<std::endl;
  // here we dont know if it contains "data/monomers/", so if it doesnt
  // we add it -> move to coot-util at some point
  // and add windows "\" although maybe done already!?
  std::string mon_lib_stub;
  std::pair<std::string, std::string> tmp_pair;
  mon_lib_stub = mon_lib_dir;
  if (mon_lib_dir.substr(mon_lib_dir.size()-1) == "/") {
     mon_lib_stub = mon_lib_dir.substr(0, mon_lib_dir.size()-1);
  }
  tmp_pair = coot::util::split_string_on_last_slash(mon_lib_stub);
  if (tmp_pair.second != "monomers") {
    // we dont have monomers, but do we have at least data?
    if (tmp_pair.second != "data") {
      // we dont have data and monomers
      mon_lib_dir = mon_lib_stub;
      mon_lib_dir += "/data/monomers/";
    } else {
      // no monomers but data
      mon_lib_dir = mon_lib_stub;
      mon_lib_dir += "/monomers/";
    }
  }
  // now mon_lib_dir should be wit data and monomers

  std::string ener_lib = coot::util::append_dir_file(mon_lib_dir, "ener_lib.cif");
  if (!coot::file_exists(ener_lib)) {
    ener_lib = "";
    std::cout << "BL WARNING:: couldnt find ener_lib.cif, ribbons may not work" <<std::endl;
  }

  std::string mon_lib_list = coot::util::append_dir_file(mon_lib_dir, "mon_lib.list");
  if (!coot::file_exists(mon_lib_list)) {
    mon_lib_list = "";
    std::cout << "BL WARNING:: couldnt find mon_lib.list, ribbons may not work" <<std::endl;
  }

  std::string elements = coot::util::append_dir_file(mon_lib_dir, "elements.cif");
  if (!coot::file_exists(elements)) {
    elements = "";
    std::cout << "BL INFO:: couldnt find elements.cif, ribbons may not work but should [SMc]" <<std::endl;
  }

  CMMANManager* molHnd=0;
  CMGSBase *s_base = new CMGSBase((char *)mon_lib_dir.c_str(),
                                  (char *)mon_lib_dir.c_str(),
                                  "",
                                  (char *)ener_lib.c_str(),
                                  (char *)mon_lib_list.c_str(),
                                  (char *)elements.c_str());

  CMolBondParams *bond_params = new CMolBondParams(s_base);

  molHnd = new CMMANManager(s_base, bond_params);
  g_print("BL DEBUG:: reading file %s\n", name_.c_str());
  molHnd->ReadCoorFile(name_.c_str());
  int selHnd = molHnd->NewSelection();

  molHnd->SelectAtoms(selHnd, 0,"*",ANY_RES,"*",ANY_RES,"*","*","*","*","*",SKEY_OR );
  molHnd->GetMolBonds();
  molHnd->GetModel(1)->CalcSecStructure(0);
  int nSelAtoms;
  PPCAtom selAtoms=0;

  molHnd->GetSelIndex(selHnd,selAtoms,nSelAtoms);

  AtomColourVector* cv = new AtomColourVector();

  PCColourSchemes schemes = new CColourSchemes();
  CMolColour cmc = CMolColour(molHnd, -1, schemes);
  cmc.SetMode (1, SECSTR, SECSTR, -1, 0);
  //cmc.SetMode (1, BYATOMTYPE, BYATOMTYPE, -1, 0);
  //cmc.SetOneColour (5);
  //cmc.SetMode (1, ONECOLOUR, ONECOLOUR, -1, 0);
  cv = cmc.GetAtomColourVector();

  int spline_accu = 4 + ccp4mg_global_params.GetInt("solid_quality") * 4;
  SplineInfo sinfo = GetSplineInfo(molHnd, selHnd, cv,
				   spline_accu, -1, -1,
                                   ribbon_params.GetInt("flatten_beta"),
                                   ribbon_params.GetInt("flatten_loop"),
                                   ribbon_params.GetInt("smooth_helix"));

  int mode=SPLINE;
  Ribbons.clear_prims();
  build_spline(sinfo, Ribbons, mode, ribbon_params, ccp4mg_global_params, "", "");

}

void
molecule_class_info_t::draw_ribbons() {

 if (cootribbons) {
//   Ribbons.set_transparent(1);
//   Ribbons.SetAlpha(0.5);
   // Maybe should be somewhere else?!
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
   glEnable(GL_NORMALIZE);
   glEnable(GL_LIGHTING);
   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST);
   glPolygonMode(GL_FRONT, GL_FILL);
   glPolygonMode(GL_BACK,GL_FILL);
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glHint(GL_FOG_HINT, GL_NICEST);


   Ribbons.draw_solids();
   glDisable(GL_LIGHTING);
 }

}

void
molecule_class_info_t::set_ribbon_param(const std::string &name,
					int value) {
   
  ribbon_params.SetInt(name.c_str(), value);
  make_ribbons(); //refresh
  graphics_info_t::graphics_draw();

}

void
molecule_class_info_t::set_ribbon_param(const std::string &name,
					float value) {

   ribbon_params.SetFloat(name.c_str(), value);
   make_ribbons(); //refresh
   graphics_info_t::graphics_draw();

}

void
molecule_class_info_t::set_global_ccp4mg_param(const std::string &name,
					       int value) {

  ccp4mg_global_params.SetInt(name.c_str(), value);
  make_ribbons(); // refresh?!
  graphics_info_t::graphics_draw();

}
 
void
molecule_class_info_t::set_global_ccp4mg_param(const std::string &name,
					       float value) {

  ccp4mg_global_params.SetFloat(name.c_str(), value);
  make_ribbons(); // refresh?!
  graphics_info_t::graphics_draw();

}


// for Aniso spheroids
// probably shoudlnt be here.... will move at some point
void
molecule_class_info_t::make_aniso_spheroids() {

  InitMatType();
  graphics_info_t g;
  std::string mon_lib_dir = g.master_mon_lib_dir;
  std::cout <<"BL DEBUG:: mon_lib_dir is " << mon_lib_dir<<std::endl;
  // here we dont know if it contains "data/monomers/", so if it doesnt
  // we add it -> move to coot-util at some point
  // and add windows "\" although maybe done already!?
  std::string mon_lib_stub;
  std::pair<std::string, std::string> tmp_pair;
  mon_lib_stub = mon_lib_dir;
  if (mon_lib_dir.substr(mon_lib_dir.size()-1) == "/") {
     mon_lib_stub = mon_lib_dir.substr(0, mon_lib_dir.size()-1);
  }
  tmp_pair = coot::util::split_string_on_last_slash(mon_lib_stub);
  if (tmp_pair.second != "monomers") {
    // we dont have monomers, but do we have at least data?
    if (tmp_pair.second != "data") {
      // we dont have data and monomers
      mon_lib_dir = mon_lib_stub;
      mon_lib_dir += "/data/monomers/";
    } else {
      // no monomers but data
      mon_lib_dir = mon_lib_stub;
      mon_lib_dir += "/monomers/";
    }
  }
  // now mon_lib_dir should be wit data and monomers

  std::string ener_lib = coot::util::append_dir_file(mon_lib_dir, "ener_lib.cif");
  if (!coot::file_exists(ener_lib)) {
    ener_lib = "";
    std::cout << "BL WARNING:: couldnt find ener_lib.cif, ribbons may not work" <<std::endl;
  }

  std::string mon_lib_list = coot::util::append_dir_file(mon_lib_dir, "mon_lib.list");
  if (!coot::file_exists(mon_lib_list)) {
    mon_lib_list = "";
    std::cout << "BL WARNING:: couldnt find mon_lib.list, ribbons may not work" <<std::endl;
  }

  std::string elements = coot::util::append_dir_file(mon_lib_dir, "elements.cif");
  if (!coot::file_exists(elements)) {
    elements = "";
    std::cout << "BL INFO:: couldnt find elements.cif, ribbons may not work but should [SMc]" <<std::endl;
  }

  AnisoSpheroids.clear_prims();
  CMMANManager* molHnd=0;
  CMGSBase *s_base = new CMGSBase((char *)mon_lib_dir.c_str(),
                                  (char *)mon_lib_dir.c_str(),
                                  "",
                                  (char *)ener_lib.c_str(),
                                  (char *)mon_lib_list.c_str(),
                                  (char *)elements.c_str());

  CMolBondParams *bond_params = new CMolBondParams(s_base);

  molHnd = new CMMANManager(s_base, bond_params);
  g_print("BL DEBUG:: reading file %s\n", name_.c_str());
  molHnd->ReadCoorFile(name_.c_str());
  //molHnd = (CMMANManager*)atom_sel.mol;
  int selHnd = molHnd->NewSelection();

  molHnd->SelectAtoms(selHnd, 0,"*",ANY_RES,"*",ANY_RES,"*","*","*","*","*",SKEY_OR );
  int nSelAtoms;
  PPCAtom selAtoms=0;

  molHnd->GetSelIndex(selHnd,selAtoms,nSelAtoms);

  AtomColourVector* cv;

  PCColourSchemes schemes = new CColourSchemes();
  CMolColour cmc = CMolColour(molHnd, selHnd, schemes);
  //cmc.SetMode (1, BYATOMTYPE, BYATOMTYPE,-1,0);
  //cmc.SetMode (1, BYRESTYPE, BYRESTYPE,-1,0);
  int RC;
  CColourScheme *AtomType = new CColourScheme();
  char *atmtyps[7] = {"*"," C"," O", " N", " S"," H"," P" };
  char *atmcols[7] = { "grey","yellow", "red", "blue" , "green", "grey","magenta" };
  AtomType->SetSchemeString(7, atmtyps, atmcols);
  cmc.SetMode (1, BYATOMTYPE, BYATOMTYPE,-1,0);

  std::vector<double> atom_radii;
  atom_radii = std::vector<double> (nSelAtoms);
  for(int i=0;i<nSelAtoms;i++)
    atom_radii[i] = 1;
  //atom_radii = molHnd->GetAtomRadii(selHnd, VDWRADIUS, 1.);
  cv = cmc.GetAtomColourVector();

  // 1.2 is arbitrary scale, appears to be needed to get to same level... FIXME
  float spheroid_scale = graphics_info_t::show_aniso_atoms_probability / 50. * 1.2;
  
  g_print ("BL DEBUG:: no of atoms: %i\n", nSelAtoms);
  int aniso_style;
  aniso_style = SPHEROID_SOLID;
  //aniso_style = SPHEROID_AXES;
  //aniso_style = SPHEROID_SOLIDAXES;

  std::cout<<"BL DEBUG:: solid_quality for aniso is " << ccp4mg_global_params.GetInt("solid_quality") <<std::endl;

  DrawAnisoU(AnisoSpheroids,
	     selHnd, selAtoms, nSelAtoms,
	     cv, atom_radii,
	     aniso_style, spheroid_scale,
	     ccp4mg_global_params);


}

void
molecule_class_info_t::draw_aniso_spheroids() {

 if (cootanisospheroids) {
   // Maybe should be somewhere else?!
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
   glEnable(GL_NORMALIZE);
   glEnable(GL_LIGHTING);
   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_COLOR_MATERIAL);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   glPolygonMode(GL_FRONT, GL_FILL);
   glPolygonMode(GL_BACK,GL_FILL);
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glHint(GL_FOG_HINT, GL_NICEST);
   glEnable(GL_LIGHT0);
   AnisoSpheroids.draw_solids();
   glDisable(GL_LIGHTING);
 }

}
