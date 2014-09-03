#include <iostream>
#include <vector>
#include <algorithm>
#include "mmut_lipids.h"
#include "mmut_connectivity.h"
#include "mmut_util.h"
#include <mgtree.h>
#include <catmull.h>
#include <cartesian.h>
#include <geomutil.h>
#include <string.h>

int  MMUTLipidAnalyse(CMMANManager *molHnd, int selHnd_in, int minimum_chain_length){

  /* This probably needs to be even more exhaustive */
  char excluded_residues[] = "ALA,ARG,ASN,ASP,CYS,GLN,GLU,GLY,HIS,ILE,LEU,LYS,MET,PHE,PRO,SER,THR,TRP,TYR,VAL,HEM,WAT,SUL,ASX,ADE,CYT,GUA,INO,THY,URA,WAT,HOH,TIP,H2O,DOD,MOH,AD,GD,CD,TD,AR,GR,CR,UR,YG,PSU,IR,GCH";

  std::vector<std::vector<int> > branches;
  std::vector<std::vector<TreeVertex*> > head_groups;
  std::vector<int> head_group_chain_ids;

  int nAtoms;
  mmdb::PPAtom atomTable=0;
  int nAtoms_all;
  mmdb::PPAtom atomTable_all=0;
  
  int selHnd = molHnd->NewSelection();
  molHnd->Select  (selHnd,STYPE_ATOM,selHnd_in,SKEY_NEW);

  int selHnd_all = molHnd->NewSelection();
  molHnd->Select  (selHnd_all,STYPE_ATOM,selHnd,SKEY_NEW);
  molHnd->GetSelIndex ( selHnd_all, atomTable_all, nAtoms_all );
  if(nAtoms_all==0) return 0;

  molHnd->GetSelIndex ( selHnd, atomTable, nAtoms );
  if(nAtoms==0) return 0;
  char* chainID = atomTable[0]->GetChainID();
  molHnd->Select  (selHnd,STYPE_ATOM,1,chainID,ANY_RES,"*",ANY_RES,"*","*","*","C","*",SKEY_AND);
  molHnd->Select  (selHnd,STYPE_ATOM,1,chainID,ANY_RES,"*",ANY_RES,"*",excluded_residues,"*","*","*",SKEY_XOR);
  atomTable=0;
  molHnd->GetSelIndex ( selHnd, atomTable, nAtoms );
  
  //std::cout << "Selected " << nAtoms << " atoms\n";
  if(nAtoms<minimum_chain_length) return 0; // At this point ModelAnalysis.analyse will be happy. Need go no further.

  Connectivity conn;
  conn.AddBonds(molHnd,selHnd,atomTable,nAtoms,0);
  std::vector<std::vector<int> > conn_lists = conn.GetConnectivityLists();
  Tree tree;
  std::vector<Cartesian> carts = mmdb::PPAtomsToCartesians(nAtoms,atomTable);
  tree.SetCoords(carts, 0,conn_lists);
 
  branches = tree.FindLongBranches(minimum_chain_length);
  if(branches.size()<1) return 0; // At this point ModelAnalysis.analyse will be happy. Need go no further.

  return 1; // We seem to have some lipids.
}

std::vector<MMUTLipid> MMUTLipidCalculate(CMMANManager *molHnd, int selHnd_in, int minimum_chain_length){
  //clock_t t1 = clock();

  /* This probably needs to be even more exhaustive */
  char excluded_residues[] = "ALA,ARG,ASN,ASP,CYS,GLN,GLU,GLY,HIS,ILE,LEU,LYS,MET,PHE,PRO,SER,THR,TRP,TYR,VAL,HEM,WAT,SUL,ASX,ADE,CYT,GUA,INO,THY,URA,WAT,HOH,TIP,H2O,DOD,MOH,AD,GD,CD,TD,AR,GR,CR,UR,YG,PSU,IR,GCH";

  std::vector<MMUTLipid> lipids;

  std::vector<std::vector<int> > branches;
  std::vector<std::vector<TreeVertex*> > head_groups;
  std::vector<int> head_group_chain_ids;

  int nAtoms;
  mmdb::PPAtom atomTable=0;
  int nAtoms_all;
  mmdb::PPAtom atomTable_all=0;
  
  int selHnd = molHnd->NewSelection();
  molHnd->Select  (selHnd,STYPE_ATOM,selHnd_in,SKEY_NEW);

  int selHnd_all = molHnd->NewSelection();
  molHnd->Select  (selHnd_all,STYPE_ATOM,1,"*",ANY_RES,"*",ANY_RES,"*","*","*","*","*",SKEY_NEW);
  //molHnd->Select  (selHnd_all,STYPE_ATOM,selHnd,SKEY_NEW);
  molHnd->GetSelIndex ( selHnd_all, atomTable_all, nAtoms_all );
  if(nAtoms_all==0) return lipids;

  molHnd->GetSelIndex ( selHnd, atomTable, nAtoms );
  if(nAtoms==0) return lipids;
  molHnd->Select  (selHnd,STYPE_ATOM,1,"*",ANY_RES,"*",ANY_RES,"*","*","*","C","*",SKEY_AND);
  molHnd->Select  (selHnd,STYPE_ATOM,1,"*",ANY_RES,"*",ANY_RES,"*",excluded_residues,"*","*","*",SKEY_CLR);
  atomTable=0;
  molHnd->GetSelIndex ( selHnd, atomTable, nAtoms );
  
  //std::cout << "Total " << nAtoms_all << " atoms\n";
  //std::cout << "Selected " << nAtoms << " atoms\n";
  if(nAtoms<minimum_chain_length) return lipids;

  Connectivity conn;
  conn.AddBonds(molHnd,selHnd,atomTable,nAtoms,0);
  std::vector<std::vector<int> > conn_lists = conn.GetConnectivityLists();
  Tree tree;
  std::vector<Cartesian> carts = mmdb::PPAtomsToCartesians(nAtoms,atomTable);
  tree.SetCoords(carts, 0,conn_lists);
 
  branches = tree.FindLongBranches(minimum_chain_length);
  if(branches.size()<1) return lipids;

  //std::cout << "Selected " << nAtoms_all << " atoms in total\n";

  Connectivity conn_all;
  conn_all.AddBonds(molHnd,selHnd_all,atomTable_all,nAtoms_all,0);
  std::vector<std::vector<int> > conn_lists_all = conn_all.GetConnectivityLists();
  Tree tree_all;
  std::vector<Cartesian> carts_all = mmdb::PPAtomsToCartesians(nAtoms_all,atomTable_all);
  tree_all.SetCoords(carts_all, 0,conn_lists_all);
 

  //clock_t t1a = clock(); std::cout << "Time for preamble " << ((t1a-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";
  std::vector<std::vector<TreeVertex*> > branch_nodes(branches.size());
  std::vector<std::vector<int> >::iterator branches_iter = branches.begin();
  int ii=0;
  //std::cout << "There are " << branches.size() << " branches\n";
  std::vector<int> serNums;
  for(int jj=0;jj<nAtoms_all;jj++){
    serNums.push_back(atomTable_all[jj]->serNum);
  }
  while(branches_iter!=branches.end()){
    std::vector<int>::iterator branch_iter = branches_iter->begin();
    int kk=0;
    while(branch_iter!=branches_iter->end()){
       int branch_kk_atom = atomTable[*branch_iter]->serNum;
       int kk_index = -1;
       for(int jj=0;jj<nAtoms_all;jj++){
         if(serNums[jj]==branch_kk_atom){
            kk_index = jj; break;
         }
       }
       if(kk_index>-1) branch_nodes[ii].push_back(tree_all.GetCoord(kk_index));
       branch_iter++; kk++;
    }
    branches_iter++; ii++;
  }
  //clock_t t1b = clock(); std::cout << "Time to end of branch thingy " << ((t1b-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  /* Check to see if the chains are "branched", ie. if there are side chains. We will
   * need to deal with them at some time probably, but for now, we just take the longest chain.
   */

  /*
  // Seems we may not need this, if we just avoid the extra_branch_nodes confusion below.
  std::vector<int> rejected_side_branches(branch_nodes.size());
  //std::cout << "Branch nodes size: " << branch_nodes.size() << "\n"; std::cout.flush();
  for(unsigned ii=0;ii<branch_nodes.size();ii++){
    //std::cout << "Branch nodes[" << ii << "] size: " << branch_nodes[ii].size() << "\n"; std::cout.flush();
    for(unsigned jj=ii+1;jj<branch_nodes.size();jj++){
       //std::cout << "Branch nodes[" << jj << "] size: " << branch_nodes[jj].size() << "\n"; std::cout.flush();
       if(branch_nodes[ii].size()>1&&branch_nodes[jj].size()>1
	&&(
	(branch_nodes[ii][0]->GetID()==branch_nodes[jj][0]->GetID()
	&&branch_nodes[ii][1]->GetID()==branch_nodes[jj][1]->GetID())
	||
	(branch_nodes[ii][branch_nodes[ii].size()-1]->GetID()==branch_nodes[jj][branch_nodes[jj].size()-1]->GetID()
	&&branch_nodes[ii][branch_nodes[ii].size()-2]->GetID()==branch_nodes[jj][branch_nodes[jj].size()-2]->GetID())
	)
	){
	  //std::cout << "Chains " << ii << "(" << branch_nodes[ii].size() << ") and " << jj << "(" << branch_nodes[jj].size() << ") appear to share vertices\n";
	  if(branch_nodes[ii].size()<branch_nodes[jj].size()) rejected_side_branches[ii] = 1; 
	  else rejected_side_branches[jj] = 1;
       }
    }
  }

  std::vector<std::vector<int> > new_branches;
  std::vector<std::vector<TreeVertex*> > new_branch_nodes;
  for(unsigned ii=0;ii<rejected_side_branches.size();ii++){
    if(1||rejected_side_branches[ii]<1){
       new_branches.push_back(branches[ii]);
       new_branch_nodes.push_back(branch_nodes[ii]);
       //std::cout << "Reject " << ii << "\n";
    }
  }

  branches = new_branches;
  branch_nodes = new_branch_nodes;
  */

  /* Check to see if end Cs are in fact part of head group */
  for(unsigned ii=0;ii<branches.size();ii++){
    //std::cout << "Branch is of size " << branch_nodes[ii].size() << " (1)\n";
    std::vector<TreeVertex *> desc_dum;
    //std::cout << "Get descendants of " << atomTable_all[branch_nodes[ii][0]->GetID()]->name << "(" << ii <<")\n";
    branch_nodes[ii][0]->GetDescendants(desc_dum,branch_nodes[ii][1]);
    bool all_CH=true;
    for(unsigned kk=0;kk<desc_dum.size();kk++){
	    //std::cout << atomTable_all[desc_dum[kk]->GetID()]->name << "\n";
      if(!(!strncmp(atomTable_all[desc_dum[kk]->GetID()]->element," H",2)||!strncmp(atomTable_all[desc_dum[kk]->GetID()]->element," C",2))) all_CH = false;
    }
    if(!all_CH&&branches[ii].size()>1){
      //std::cout << "This is a carboxyl (at begin) and should be put in head! (" << ii <<")\n";
      branches[ii] = std::vector<int> (branches[ii].begin()+1,branches[ii].end());
      branch_nodes[ii] = std::vector<TreeVertex*> (branch_nodes[ii].begin()+1,branch_nodes[ii].end());
    }
    desc_dum.clear();
    //std::cout << "Get descendants of " << atomTable_all[branch_nodes[ii].back()->GetID()]->name << "(" << ii <<")\n";
    branch_nodes[ii].back()->GetDescendants(desc_dum);
    all_CH=true;
    for(unsigned kk=0;kk<desc_dum.size();kk++){
	    //std::cout << atomTable_all[desc_dum[kk]->GetID()]->name << "\n";
      if(!(!strncmp(atomTable_all[desc_dum[kk]->GetID()]->element," H",2)||!strncmp(atomTable_all[desc_dum[kk]->GetID()]->element," C",2))) all_CH = false;
    }
    if(!all_CH&&branches[ii].size()>1){
      //std::cout << "This is a carboxyl (at end) and should be put in head! (" << ii <<")\n";
      branches[ii] = std::vector<int> (branches[ii].begin(),branches[ii].end()-1);
      branch_nodes[ii] = std::vector<TreeVertex*> (branch_nodes[ii].begin(),branch_nodes[ii].end()-1);
    }
    desc_dum.clear();
    //std::cout << "\n";
  }
  //clock_t t1c = clock(); std::cout << "Time to end of head checking " << ((t1c-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  head_group_chain_ids = std::vector<int>(branches.size());
  std::vector<std::vector<TreeVertex*> >stop_nodes;
  for(unsigned ii=0;ii<branches.size();ii++){
    //std::cout << "Branch is of size " << branch_nodes[ii].size() << " (2)\n";
    stop_nodes.push_back(std::vector<TreeVertex*>(0));
    int branch_start = branches[ii][0];
    int branch_end = branches[ii].back();
    mmdb::PAtom branch_start_atom = atomTable[branch_start];
    mmdb::PAtom branch_end_atom = atomTable[branch_end];
    //std::cout << "branch start/end: " << branch_start << " " << branch_end << "\n";
    //std::cout << "atom serial numbers: " << branch_start_atom->serNum << " " << branch_end_atom->serNum << "\n";
    int end_index = -1;
    int start_index = -1;
    for(int jj=0;jj<nAtoms_all;jj++){
      if(serNums[jj]==branch_end_atom->serNum){
         end_index = jj; break;
      }
    }
    for(int jj=0;jj<nAtoms_all;jj++){
      if(serNums[jj]==branch_start_atom->serNum){
         start_index = jj; break;
      }
    }
    stop_nodes.back().push_back(tree_all.GetCoord(start_index));
    stop_nodes.back().push_back(tree_all.GetCoord(end_index));
    //std::cout << "Adding stop nodes " << tree_all.GetCoord(start_index)->GetID() << " " << tree_all.GetCoord(end_index)->GetID() << "\n";
  }

  std::vector<std::vector<TreeVertex *> > extra_branch_nodes(branches.size()); // Hydrogens.
  //clock_t t1d = clock(); std::cout << "Time to end of adding stop nodes " << ((t1d-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  for(unsigned ii=0;ii<branches.size();ii++){
    //std::cout << "Branch is of size " << branch_nodes[ii].size() << " (3)\n";
    head_group_chain_ids[ii] = -1;
    int branch_start = branches[ii][0];
    int branch_end = branches[ii].back();
    mmdb::PAtom branch_start_atom = atomTable[branch_start];
    mmdb::PAtom branch_end_atom = atomTable[branch_end];
    //std::cout << "branch start/end: " << branch_start << " " << branch_end << "\n";
    //std::cout << "atom serial numbers: " << branch_start_atom->serNum << " " << branch_end_atom->serNum << "\n";
    //std::cout << "atoms: " << branch_start_atom->name << " " << branch_end_atom->name << "\n";
    int end_index = -1;
    int start_index = -1;
    for(int jj=0;jj<nAtoms_all;jj++){
      if(serNums[jj]==branch_end_atom->serNum){
         end_index = jj; break;
      }
    }
    for(int jj=0;jj<nAtoms_all;jj++){
      if(serNums[jj]==branch_start_atom->serNum){
         start_index = jj; break;
      }
    }
    // Get nodes above and below branch.
    //std::cout << "start_index: " << start_index << ", end_index: " << end_index << "\n"; std::cout.flush();
    std::vector<TreeVertex *> desc_vertices;
    std::vector<TreeVertex *> non_desc_vertices;
    TreeVertex *end_vertex = tree_all.GetCoord(end_index);
    TreeVertex *start_vertex = tree_all.GetCoord(start_index);
    //std::cout << "Getting descendents\n"; std::cout.flush();
    std::vector<TreeVertex*> other_stop_nodes;
    for(unsigned iother=0;iother<branches.size();iother++)
	    if(iother!=ii) other_stop_nodes.insert(other_stop_nodes.begin(),branch_nodes[iother].begin(),branch_nodes[iother].end());
    end_vertex->GetDescendants(desc_vertices,other_stop_nodes);
    //std::cout << "There are " << desc_vertices.size() << " after branch\n";
    //std::cout << "Getting non-descendents\n"; std::cout.flush();
    //std::cout << "of " << atomTable_all[start_vertex->GetID()]->name << " " << start_vertex->GetID() << "\n";
    start_vertex->GetNonDescendants(non_desc_vertices,other_stop_nodes);
    //std::cout << "There are " << non_desc_vertices.size() << " before branch\n";
    if(desc_vertices.size()>0){
      if(desc_vertices.size()>3){
        std::vector<TreeVertex *> desc_vertices_new;
	for(unsigned kk=0;kk<desc_vertices.size();kk++){
          char elname = atomTable_all[desc_vertices[kk]->GetID()]->element[1];
	  //std::cout << atomTable_all[desc_vertices[kk]->GetID()]->name << "\n";
	  if(desc_vertices[kk]->GetParent()==end_vertex&&elname=='H'){
            extra_branch_nodes[ii].push_back(desc_vertices[kk]);   
	  }else{
	    desc_vertices_new.push_back(desc_vertices[kk]);
	  }
	}
	desc_vertices = desc_vertices_new;
        head_groups.push_back(desc_vertices);
        //std::cout << "Adding head group\n";
	head_group_chain_ids[ii] = head_groups.size()-1;
      }else{ /* If group is all Hs and Cs then add to branch. */
        bool all_CH=true;
	for(unsigned kk=0;kk<desc_vertices.size();kk++){
          char elname = atomTable_all[desc_vertices[kk]->GetID()]->element[1];
          if(elname!='H'&&elname!='C') { all_CH = false; break; }
	}
	if(!all_CH){
          head_groups.push_back(desc_vertices);
          //std::cout << "Adding head group\n";
	  head_group_chain_ids[ii] = head_groups.size()-1;
	}else{
        // Add to branch ....
	  for(unsigned kk=0;kk<desc_vertices.size();kk++){
            extra_branch_nodes[ii].push_back(desc_vertices[kk]);
	    //std::cout << "Adding an extra node (1)\n";
          }
	}
      }
    }
    if(non_desc_vertices.size()>0){
      if(non_desc_vertices.size()>3){
        head_groups.push_back(non_desc_vertices);
        //std::cout << "Adding head group\n";
	head_group_chain_ids[ii] = head_groups.size()-1;
      }else{ /* If group is all Hs and Cs then add to branch. */
	      //std::cout << " So non desc vertices size is " << non_desc_vertices.size() << "\n";
        bool all_CH=true;
	for(unsigned kk=0;kk<non_desc_vertices.size();kk++){
		std::cout << atomTable_all[non_desc_vertices[kk]->GetID()]->name << " " << non_desc_vertices[kk]->GetID() << "\n";
          char elname = atomTable_all[non_desc_vertices[kk]->GetID()]->element[1];
          if(elname!='H'&&elname!='C') { all_CH = false; break; }
	}
	if(!all_CH){
          head_groups.push_back(non_desc_vertices);
          //std::cout << "Adding head group\n";
	  head_group_chain_ids[ii] = head_groups.size()-1;
	}else{
        // Add to branch ....
	  for(unsigned kk=0;kk<non_desc_vertices.size();kk++){
            extra_branch_nodes[ii].push_back(non_desc_vertices[kk]);
	    //std::cout << "Adding an extra node (2)\n";
          }
	}
      }
    }
    //std::cout << "extra_branch_nodes[ii].size() before getting descendants: " << extra_branch_nodes[ii].size() << "\n";
    // This is where branched tails go wrong. I have commented this out for now
    // as I can't rememer what circumstances it may be necessary.
    /*
    for(unsigned kk=0;kk<branches[ii].size()-1;kk++){
      branch_nodes[ii][kk]->GetDescendants(extra_branch_nodes[ii],branch_nodes[ii][kk+1]);
    }
    */
    //std::cout << "extra_branch_nodes[ii].size() after getting descendants: " << extra_branch_nodes[ii].size() << "\n";
  }
  //clock_t t1e = clock(); std::cout << "Time to end of descendents stuff " << ((t1e-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  // We now have all branches and head groups! But head groups may have redundancy.
  if(head_groups.size()>0){
    std::vector<int> head_group_start_indices;
    std::vector<std::vector<TreeVertex*> > head_groups_new;
    head_group_start_indices.push_back(head_groups[0][0]->GetID());
    head_groups_new.push_back(head_groups[0]);
    for(unsigned jj=1;jj<head_groups.size();jj++){
      if(std::find(head_group_start_indices.begin(),head_group_start_indices.end(),head_groups[jj][0]->GetID())==head_group_start_indices.end()){
         head_group_start_indices.push_back(head_groups[jj][0]->GetID());
         head_groups_new.push_back(head_groups[jj]);
      } else {
	 for(unsigned ii=0;ii<head_group_chain_ids.size();ii++){
	   if(head_group_chain_ids[ii]>=(int)jj) head_group_chain_ids[ii]--;
	 }
      }
    }
    head_groups = head_groups_new;
    head_groups_new.clear();
  }
  //clock_t t1f = clock(); std::cout << "Time to end of pruning head groups " << ((t1f-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  /* Now we have to construct the lipids ... */
  std::vector<int> tails_selhnds;
  if(head_groups.size()>0){
    for(unsigned jj=0;jj<head_groups.size();jj++){
      // We don't consider more than one head per lipid at the moment??
      std::vector<std::vector<int> > head_serNums(1);
      std::vector<std::vector<Cartesian> > head_carts(1);
      MMUTLipid lipid;
      lipid.SetMainSelectionHandle(selHnd_in);
      int head_selhnd = molHnd->NewSelection(); 
      //ivector ivec = new int[head_groups[jj].size()];
      for(unsigned ii=0;ii<head_groups[jj].size();ii++){
        //ivec[ii] = serNums[head_groups[jj][ii]->GetID()];
        mmdb::PAtom at = atomTable_all[head_groups[jj][ii]->GetID()];
        head_carts.back().push_back(Cartesian(at->x,at->y,at->z));
	head_serNums.back().push_back(serNums[head_groups[jj][ii]->GetID()]);
      }
      //molHnd->SelectAtoms(head_selhnd,ivec,head_groups[jj].size(),SKEY_OR);
      //delete [] ivec;
      // We don't consider more than one head per lipid at the moment??
      std::vector<int> head_selHnds;
      head_selHnds.push_back(head_selhnd);
      lipid.SetHeadCartesians(head_carts);
      lipid.SetHeadSerNums(head_serNums);
      lipids.push_back(lipid);
    }
  }

  //clock_t t1g = clock(); std::cout << "Time to end of initial lipid construction " << ((t1g-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  std::vector<std::vector<Cartesian> > tail_carts;
  std::vector<std::vector<int> > tail_serNums;
  std::vector<std::vector<int> > tail_other_serNums;
  for(unsigned jj=0;jj<branches.size();jj++){
    //std::cout << "Branch is of size " << branch_nodes[jj].size() << " (4)\n";
    tail_carts.push_back(std::vector<Cartesian>(0));
    tail_serNums.push_back(std::vector<int>(0));
    tail_other_serNums.push_back(std::vector<int>(0));
    //int tail_selhnd = molHnd->NewSelection(); 
    //tails_selhnds.push_back(tail_selhnd);
    //ivector ivec = new int[branches[jj].size()+extra_branch_nodes[jj].size()];
    for(unsigned ii=0;ii<branches[jj].size();ii++){
      mmdb::PAtom at = atomTable[branches[jj][ii]];
      //ivec[ii] = at->serNum;
      tail_carts.back().push_back(Cartesian(at->x,at->y,at->z));
      //std::cout << "Branch: " << at->name << " " << tail_carts.back().back() << "\n";
      tail_serNums.back().push_back(at->serNum);
    }
    if(tail_carts.back().size()>0) std::reverse(tail_carts.back().begin(),tail_carts.back().end());
    if(tail_serNums.back().size()>0) std::reverse(tail_serNums.back().begin(),tail_serNums.back().end());
    for(unsigned ii=0;ii<extra_branch_nodes[jj].size();ii++){
      //ivec[ii+branches[jj].size()] = atomTable_all[extra_branch_nodes[jj][ii]->GetID()]->serNum;
      mmdb::PAtom at = atomTable_all[extra_branch_nodes[jj][ii]->GetID()];
      char elname = at->element[1];
      if(elname=='C'){ // But what if this atom is a side branch off the main branch?
        tail_carts.back().push_back(Cartesian(at->x,at->y,at->z));
        tail_serNums.back().push_back(at->serNum);
        //std::cout << "Extra node: " << at->name << " " << tail_carts.back().back() << "\n";
      } else {
        tail_other_serNums.back().push_back(at->serNum);
      }
    }
    /*
    for(unsigned ii=0;ii<branches[jj].size()+extra_branch_nodes[jj].size();ii++)
      //std::cout << ivec[ii] << " ";
    //std::cout << "\n";
    */
    //molHnd->SelectAtoms(tail_selhnd,ivec,branches[jj].size()+extra_branch_nodes[jj].size(),SKEY_OR);
    //delete [] ivec;
  }
  //clock_t t1h = clock(); std::cout << "Time to end of tail_selhnd business " << ((t1h-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";
  
  std::vector<int> start_of_tail(head_group_chain_ids.size());
  // Now we have to check the if the tails without heads really should have heads...
  for(unsigned ii=0;ii<head_group_chain_ids.size();ii++){
    //std::cout << "Branch " << ii << " has head group " << head_group_chain_ids[ii] << "\n";
    // We'll consider all tails, because we still have to find the vertex next to the head group.
    if(1||head_group_chain_ids[ii]<0){
      //std::cout << "Chain " << ii << " is without a head\n";
      int branch_start = branches[ii][0];
      int branch_end = branches[ii].back();
      mmdb::PAtom branch_start_atom = atomTable[branch_start];
      mmdb::PAtom branch_end_atom = atomTable[branch_end];
      int end_index = -1;
      int start_index = -1;
      for(int jj=0;jj<nAtoms_all;jj++){
        if(serNums[jj]==branch_end_atom->serNum){
           end_index = jj; break;
        }
      }
      for(int jj=0;jj<nAtoms_all;jj++){
        if(serNums[jj]==branch_start_atom->serNum){
           start_index = jj; break;
        }
      }
      // Get nodes above and below branch.
      //std::cout << "start_index: " << start_index << ", end_index: " << end_index << "\n"; std::cout.flush();
      std::vector<TreeVertex *> desc_vertices;
      std::vector<TreeVertex *> non_desc_vertices;
      TreeVertex *end_vertex = tree_all.GetCoord(end_index);
      TreeVertex *start_vertex = tree_all.GetCoord(start_index);

      bool done=false;
      start_of_tail[ii] = end_vertex->GetID(); // In case it really is orphoned.
      for(unsigned ihead=0;ihead<head_groups.size()&&!done;ihead++){
        for(unsigned ihead_atom=0;ihead_atom<head_groups[ihead].size();ihead_atom++){
          TreeVertex* head_atom = head_groups[ihead][ihead_atom];
          if(start_vertex->GetParent()&&start_vertex->GetParentID()==head_atom->GetID()){
            //std::cout << "Seems branch " << ii << " belongs to head " << ihead << "\n";
            head_group_chain_ids[ii] = ihead;
            //std::cout << ii << ": " << start_vertex->GetParent()->GetID() << "\n";
            //std::cout << "   " << ihead << ": " << head_atom->GetID()  << "\n";
            done = true;
            start_of_tail[ii] = start_vertex->GetID();
          }
          if(head_atom->GetParent()&&head_atom->GetParentID()==end_vertex->GetID()){
            //std::cout << "Seems branch " << ii << " belongs to head " << ihead << "\n";
            //std::cout << ii << ": " << end_vertex->GetID() << "\n";
            //std::cout << "   " << ihead << ": " << head_atom->GetParent()->GetID()  << "\n";
            head_group_chain_ids[ii] = ihead;
            done = true;
            start_of_tail[ii] = end_vertex->GetID();
          }
          if(!done&&extra_branch_nodes[ii].size()>0){
            for(unsigned iextra=0;iextra<extra_branch_nodes[ii].size()&&!done;iextra++){
              if(extra_branch_nodes[ii][iextra]->GetParent()&&extra_branch_nodes[ii][iextra]->GetParentID()==head_atom->GetID()){
                //std::cout << "Seems branch " << ii << " belongs to head " << ihead << "\n";
                head_group_chain_ids[ii] = ihead;
                done = true;
               start_of_tail[ii] = extra_branch_nodes[ii][iextra]->GetID();
              }
              if(head_atom->GetParent()&&head_atom->GetParentID()==extra_branch_nodes[ii][iextra]->GetID()){
                //std::cout << "Seems branch " << ii << " belongs to head " << ihead << "\n";
                head_group_chain_ids[ii] = ihead;
                done = true;
               start_of_tail[ii] = extra_branch_nodes[ii][iextra]->GetID();
              }
            }
          }
        }
      }
    }
  }
  //clock_t t1i = clock(); std::cout << "Time to end of dealing with orphoned tails " << ((t1i-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  for(unsigned jj=0;jj<branches.size();jj++){
    //std::cout << "Branch is of size " << branch_nodes[jj].size() << " (5)\n";
    int lipid_id;
    if(head_group_chain_ids[jj]>-1){
      lipid_id = head_group_chain_ids[jj];
    } else {
      MMUTLipid lipid;
      lipid.SetMainSelectionHandle(selHnd_in);
      lipids.push_back(lipid);
      lipid_id = lipids.size()-1;
    }
    lipids[lipid_id].AddTailCartesians(tail_carts[jj]);
    lipids[lipid_id].AddTailSerNums(tail_serNums[jj]);
    lipids[lipid_id].AddTailHSerNums(tail_other_serNums[jj]);
  }
  //clock_t t1j = clock(); std::cout << "Time to end of final lipid setting " << ((t1j-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  //std::cout << "There are " << lipids.size() << " lipids\n";
  //for(unsigned ii=0;ii<lipids.size();ii++)
    //std::cout << "Lipid " << ii << " has " << lipids[ii].GetNumberOfHeads() << " heads and " << lipids[ii].GetNumberOfTails() << " tails\n";

  //clock_t t2 = clock();
  //std::cout << "Time for lipid calculate " << ((t2-t1)*1000.0/CLOCKS_PER_SEC)/1000.0<< "\n";

  for(unsigned jj=0;jj<branch_nodes.size();jj++){
    //std::cout << "Branch is of size " << branch_nodes[jj].size() << " (5)\n";
    for(unsigned ii=0;ii<branch_nodes[jj].size();ii++){
      //std::cout << atomTable_all[branch_nodes[jj][ii]->GetID()]->name << "\n";
    }
  }
  return lipids;

}
