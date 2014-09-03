/*
     mmut/mmut_basepairs.h: CCP4MG Molecular Graphics Program
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

#ifndef _CCP4_MMUT_BASE_PAIRS_H_
#define _CCP4_MMUT_BASE_PAIRS_H_
#include <vector>
#include <utility>
#include <mman_manager.h>
#include "atom_util.h"

class CNABasePairs{
  std::vector<std::pair<Pmmdb::Residue,Pmmdb::Residue> > base_pairs;
  std::vector<std::pair<double*,double*> > colours;
 public:
  CNABasePairs(){};
  void Calculate(CMMANManager *molHnd, int selHnd, mmdb::PPAtom selAtoms, int nSelAtoms, AtomColourVector *atom_colour_vector);
  CNABasePairs(CMMANManager *molHnd, int selHnd, mmdb::PPAtom selAtoms, int nSelAtoms, AtomColourVector *atom_colour_vector);
  Pmmdb::Residue GetPairedResidue(const Pmmdb::Residue res_in) const ;
  int GetPairedResidueIndex(const int i) const ;
  std::vector<std::pair<Pmmdb::Residue,Pmmdb::Residue> > GetPairs() const {return base_pairs;};
  std::vector<std::pair<double*,double*> > GetColours() const {return colours;};
};
#endif //_CCP4_MMUT_BASE_PAIRS_H_
