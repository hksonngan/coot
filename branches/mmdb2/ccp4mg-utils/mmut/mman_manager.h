/*
     mmut/mman_manager.h: CCP4MG Molecular Graphics Program
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


#ifndef __MMAN_Manager__
#define __MMAN_Manager__

#include <string>
#include <vector>
#include <map>
#include <cartesian.h>
#include <matrix.h>
#include <mmut_manager.h> 
#include <mmut_sasarea.h>
#include <mmut_bonds.h>

enum enum_atomTypeData { HBTYPE, VDWRADIUS, VDWHRADIUS, IONRADIUS };
enum enum_property { PROPERTY_B, PROPERTY_OCC, PROPERTY_CHARGE,
                     PROPERTY_X, PROPERTY_Y, PROPERTY_Z,
                     PROPERTY_SEC, PROPERTY_ATOM_SAS, PROPERTY_RES_SAS,
                     PROPERTY_ATOM_CONTACT, PROPERTY_RES_CONTACT,
                     PROPERTY_SERIAL }; 

enum enum_editMode { MMAN_COORDINATES, MMAN_XTLDATA };

// =======================  CMMANManager  ===========================

DefineClass(CMMANManager);
DefineStreamFunctions(CMMANManager) ;

// Function to return the parent PCMMANManager of an atom
CMMANManager* GetMMANManager(Pmmdb::Atom pAtom);

  
class CMMANManager : public CMMUTManager  {
 
  public :

    CMMANManager();
    CMMANManager(PCMGSBase p_sbase_in, PCMolBondParams p_bond_params_in);
    ~CMMANManager();

    // MG SBase class
    CMGSBase *GetMGSBase();
    // SAS
    CSASArea *GetSASArea(int selHndin=-1);

    //Bonds
    std::string GetMolBonds (std::string monlib_file="");
    int EditBonds (int mode, Pmmdb::Atom p_atom1, Pmmdb::Atom p_atom2);

    //Sbase and Atom Energy types
    pstr GetSbaseCompoundID(Pmmdb::Atom atom);
    pstr GetSbaseCompoundID(Pmmdb::Residue res);
    int GetSbaseAtomOrdinal(Pmmdb::Atom atom);

    int SetupAtomEnergyTypes();
    int GetAtomTypeData (int selHnd,int type,ivector &dataout,int &nat );
    int GetAtomTypeData (int selHnd,int type,rvector &dataout,int &nat );
    std::vector<double> GetAtomRadii ( int selHnd, int type, double scale );
    int GetAtomEnergyType(Pmmdb::Atom p_atom);

    realtype GetAtomVDWRadius(Pmmdb::Atom p_atom);
    const char* GetAtomHBondType(Pmmdb::Atom p_atom);
    int GetAtomHBondType1(Pmmdb::Atom p_atom);
    int LoadCharge(std::string loadfrom);
    std::string PrintCharges(void);

    void SetLabelMask(int i, int value);
    std::string AtomLabel(Pmmdb::Atom p_atom);
    std::string AtomLabel(Pmmdb::Atom p_atom,int mask[]);
    void ListBonds(int selHnd,int natoms,mmdb::PPAtom selAtom);
    std::string ListSecStructure (int mask_in[], Pmmdb::Atom pAtom=NULL );
    int TestBonding ( Pmmdb::Atom patom1, Pmmdb::Atom patom2, int max=5);
    int RestoreData (PCMMDBManager restore_molHnd, int mode=MMAN_COORDINATES);
    int LoadUDDData( const int property=PROPERTY_B );

    int CopyModel(int model);
    int GenerateSymmetryModel(int model,int nsym,int i,int j,int k);
    std::string GetSymOpTitle(int nsym,int i,int j,int k);
    int ApplySymmetrytoModel(int model,int nsym,int i,int j,int k,int undo_nsym=-1,int undo_i=0,int undo_j=0,int undo_k=0);
    int IfSymmetryNeighbours(int selHnd, int model, int nsym, 
			     int i, int j, int k, double dist );
    int MoveFragment(int nMove, mmdb::PPAtom moveAtoms, Cartesian dxyz); 
    int SelectChainTermini( void );
    int SelectSSETermini( int selHnd=-1 );

    bool isAminoacid (Pmmdb::Residue pres);
    int GetRestypeCode ( Pmmdb::Residue pres);
    std::map<std::string,PCSBStructure> monlib; 

    int SetCustomRestype ( const std::string &resname , const int &restype , Boolean clear=false);
    int SetCustomResSynonym ( const std::string &resname , const std::string &alias , Boolean clear=false);


    int ExcludeOverlappedAtoms ( const int selHnd ,const realtype cutoff );
    int SetTransform ( const std::vector<float>& transf , const std::vector<float>& transl , const bool reset);
    int SetTransform ( const realtype rot ,const std::vector<float>& axis, const int selHnd = -1 );
    int SetTransform ( const matrix tMat, const bool reset);
    int SetTransform ( mat44 &TMatrix , const bool reset);
    int ReApplyTransform( const bool reset=0);
    void UnSetTransform(bool apply_inverse=1);
    std::vector<float> GetTransform();
    std::string GetTransformString();
    bool GetIsTransformed() { return isTransformed; }
    int TransformToSuperposeAtoms (  mmdb::PPAtom A1, int nA, mmdb::PPAtom A2 );
    double DeltaResidueOrientation (Pmmdb::Residue pRes, Pmmdb::Residue pResFx);
    int TransformToSuperposeCloseAtoms(PCMMANManager fxMolHnd, int fxSelHnd , realtype central_cutoff, realtype cutoff, int mvSuperposeHnd,int fxSuperposeHnd);
    int CopyCoordinates(const PCMMDBManager fromMolHnd,int fromModel=1);
    int LoadSerial(const PCMMDBManager fromMolHnd );
    bool GetUnremediated() { return unremediated; };
    std::string PrintSecStructure (void);

  private: 
    
    // SAS
    PCSASArea p_sas;
    int udd_atomSAS;
    int udd_resSAS;

    // Bonds
    PCMGSBase p_sbase;
    PCMolBondParams p_bond_params;
    PCMolBonds p_bonds;
    //Atom energy types
    int udd_sbaseCompoundID;
    int udd_sbaseAtomOrdinal;
    int udd_atomEnergyType;

    std::string loaded_charge;

    int label_mask[20];

    std::map<std::string,int> customResTypes;
    std::map<std::string,std::string> customResSynonym;

    bool isTransformed,transform_com_set ;
    mat44 current_transform;
    realtype transform_com[3];

    bool unremediated;

};

#endif
