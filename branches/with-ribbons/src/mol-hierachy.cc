	 clipper::HKL_info::HKL_reference_index hri;
	 for (hri=fphidata.first(); !hri.last(); hri.next()) {
	    std::cout << " MTZ fphi: " << hri.hkl().h() << " "
		      << hri.hkl().k() << " " << hri.hkl().l() << " "
		      << fphidata[hri].f() << " "
		      << clipper::Util::rad2d(fphidata[hri].phi()) << std::endl;
	 } 

// template code for history addition.
   std::string cmd = "";
   std::vector<coot::command_arg_t> args;
   args.push_back();
   add_to_history_typed(cmd, args);

   add_to_history_simple("");

//    if (atom_sel.n_selected_atoms > 0) { 

      int n_models = atom_sel.mol->GetNumberOfModels();
      for (int imod=1; imod<=n_models; imod++) { 
      
	 CModel *model_p = mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p == NULL) {  
		  // This should not be necessary. It seem to be a
		  // result of mmdb corruption elsewhere - possibly
		  // DeleteChain in update_molecule_to().
		  std::cout << "NULL chain in ... " << std::endl;
	       } else { 
		  int nres = chain_p->GetNumberOfResidues();
		  PCResidue residue_p;
		  CAtom *at;
		  for (int ires=0; ires<nres; ires++) { 
		     residue_p = chain_p->GetResidue(ires);
		     int n_atoms = residue_p->GetNumberOfAtoms();

		     for (int iat=0; iat<n_atoms; iat++) {
			at = residue_p->GetAtom(iat);

			
  // ---- simple version 


   // for(int imod = 1; imod<=asc.mol->GetNumberOfModels(); imod++) {
   int imod = 1;
   CModel *model_p = asc.mol->GetModel(imod);
   CChain *chain_p;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();
      PCResidue residue_p;
      CAtom *at;
      for (int ires=0; ires<nres; ires++) { 
	 residue_p = chain_p->GetResidue(ires);
	 int n_atoms = residue_p->GetNumberOfAtoms();
	 
	 for (int iat=0; iat<n_atoms; iat++) {
	    at = residue_p->GetAtom(iat);

	       
 // ---- 
void check_chiral_volumes(int imol) { 
   graphics_info_t g;
   if (imol < graphics_info_t::n_molecules) { 
      if (graphics_info_t::molecule[imol].has_model()) { 
	 // my function here
      } else {
	 std::cout << "WARNING:: molecule " << imol 
		   <<  " does not have coordinates\n";
      }
   } else {
      std::cout << "WARNING:: no such molecule " << imol << std::endl;
   } 
} 
