/*
     mmut/mmut_connectivity.h: CCP4MG Molecular Graphics Program
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


#ifndef __MMUT_Connectivity__
#define __MMUT_Connectivity__


#include <vector>
#include <string>

#include "mgtree.h"
#include "cartesian.h"
#include "connect.h"

#include <mmdb_manager.h>
#include <mman_manager.h>

//enum types { SINGLE, DOUBLE, TRIPLE, QUADRUPLE, HBOND };

// Modes of connectivity2 data a - vector between
// atoms and/or points or displacements from the atom or point
 enum connectivity2_types { CONN_POINT_POINT, CONN_ATOM_POINT,
			    CONN_POINT_ATOM,CONN_ATOM_ATOM, 
                            CONN_ATOM_DISP, CONN_POINT_DISP };

class Connection {
 public:
  bool isInTotalSelection;
  std::vector<int> external_connected_atoms;
  std::vector<int> connected_atoms;
  std::vector<unsigned int> orders;
  Connection();
  ~Connection();
  void AddConnection(int serNum);
  void AddExternalConnection(int serNum);
};

class Connectivity {
  std::vector<Connection> TotalSelection_new;
  int IsInTotalSelection(int idx) const;
  int nSelAtomsTot;
  mmdb::PPAtom SelAtomsTot;
  
 public:
  Connectivity();
  ~Connectivity();
  void AddBonds(CMMANManager *molhnd, int selhnd, const mmdb::PPAtom SelAtoms_in, const int nSelAtoms_in, int all_selHnd);
  void AddContacts(CMMANManager *molhnd, int selhnd, const mmdb::PPAtom SelAtoms_in, const int nSelAtoms_in, const PSContact contacts_in, const int ncontacts_in);
  void AddTrace(PCMMANManager molhnd, const mmdb::PPAtom selAtoms, const int nSelAtoms,
		realtype cutoff );
  std::vector<std::vector<int> > GetConnectivityLists(void) const;
  std::vector<std::vector<int> > GetExternalConnectivityLists(void) const;
  int GetNumberOfAtoms(void) const;
  mmdb::PPAtom GetAtoms(void) const;
  std::vector<int> Getmmdb::AtomIndex(void);
  void Clear(void);
  //Tree GetTree(PCMMANManager molhnd) const;
};

// Connectivy2 class to handle 'connectivity' such as Hbonds which is 
// optionally between two separate molecules (ie. MMDBManagers)


class Connectivity2 {
  
  int data_mode;
  int crystal_axes;
  int tagged;
  bool selection_on;
  int nAtomSets;
  int nxyzSets;


 public:
  std::vector<Pmmdb::Atom> pAtom1;
  std::vector<Pmmdb::Atom> pAtom2;
  std::vector<Cartesian> XYZ1;
  std::vector<SimpleConnection> connected;
  std::vector<int> tags;
  std::vector<bool> selected;

  void DeleteConnections(std::vector<unsigned int> indices, bool all=false);

  Connectivity2(int data_mode=CONN_ATOM_ATOM,int crystal_axes=0,int tagged=0);
  ~Connectivity2();
  void AddConnection(Pmmdb::Atom p_atom1,Pmmdb::Atom p_atom2, const std::string &label_in="", int tag=-1);
  void AddConnection( double xyz1[3] ,  double xyz2[3] , const std::string &label_in,int tag=-1);
  void AddConnection(  Pmmdb::Atom p_atom1, double xyz2[3] ,
    const std::string &label_in, int tag=-1);
  void InsertConnection(bool replace, unsigned int position,Pmmdb::Atom p_atom1,Pmmdb::Atom p_atom2, const std::string &label_in="", int tag=-1);
  void InsertConnection( bool replace, unsigned int position, double xyz1[3] ,  double xyz2[3] , const std::string &label_in,int tag=-1);
  void InsertConnection(  bool replace, unsigned int position, Pmmdb::Atom p_atom1, double xyz2[3] ,
    const std::string &label_in, int tag=-1);
  void AddUniqueConnection(Pmmdb::Atom p_atom1,Pmmdb::Atom p_atom2, const std::string &label_in="",int tag = -1);
  void UpdateCoordinates(bool label_dist=false);
  void RemoveConnection(Pmmdb::Atom p_atom1,Pmmdb::Atom p_atom);
  void RemoveConnection(Pmmdb::Atom p_atom1,int position);
  void DeleteConnection(int index);
  void DeleteConnections();
  int GetNofConnections() { return  connected.size(); }
  std::vector<unsigned int> FindConnections(Pmmdb::Atom p_atom1,Pmmdb::Atom p_atom,bool switchpos=1);
  std::vector<unsigned int> FindConnections(Pmmdb::Atom p_atom1, int position=0);
  int FindNofConnections(Pmmdb::Atom p_atom1,int position=0);
  void Clear(void);	
  int SelectVectors (int nSelTags , int *selTags );
  realtype* Extent();
  std::string Print( const std::string &tag1="", const std::string &tag2="" );
  int DataMode(void) { return data_mode; }
  std::string GetLabel(int i){ return connected[i].label ; }
  Cartesian GetCoordinate(int i, int j=1);
  Pmmdb::Atom GetAtom(int i, int j=1);
  std::string GetAtomID ( int i, int j);
  int GetTag(int iV);
  int SetTag(int iV,int i);
  int AddContacts(PCMMANManager molHnd1,int selHnd1, PCMMANManager molHnd2,int selHnd2_in,realtype  dist1, realtype  dist2, int  seqDist, int inter_model=0, int closest_bonding=5, int handle_hbond=0 );
  int AddRangeConnections(int set, Pmmdb::Residue res1,Pmmdb::Residue res2, 
		 Pmmdb::Residue mres1, 
		 const std::vector<std::string>& mainchain_name,
			        int tag );
  int AddCloseRangeConnections(int set,Pmmdb::Residue res1,
                    Pmmdb::Residue res2,PCMMANManager M2,
		      double central_cutoff, double cutoff,
		    const std::vector<std::string>& mainchain_name,
                    const std::string &centralAtom, int tag );

  int GetSelection (int mode=1 );
  int Superpose(int fixed);
  int MatchGraphs(Pmmdb::Residue pRes1,const pstr altLoc1,
                  Pmmdb::Residue pRes2,const pstr altLoc2,
		  int Hflag,int tag, float fracMinMatch=-1.0,
                  bool keep_matches=true);
};

inline int Connectivity::IsInTotalSelection(int idx) const {
  if(!TotalSelection_new[idx].isInTotalSelection)
    return 0;
  else
    return 1;
}



#endif
