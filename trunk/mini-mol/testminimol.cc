 
#include <iostream>

#include "mini-mol.hh"

int
main(int argc, char **argv) {

//     std::vector<int> vec(5);
//     vec[3] = 12;
//     vec.resize(10);
//     std::vector<int> vec2 = vec;
    
//     for(int i=0; i<vec.size(); i++)
//        std::cout << i << " " << vec2[i] << std::endl;

   if (argc > 1) {
      std::string pdb_name(argv[1]);
      CMMDBManager mol;
      coot::minimol::molecule myminimol;
      myminimol.read_file(pdb_name);
      
      std::vector<coot::minimol::atom *> atoms =
	 myminimol.select_atoms_serial();

      atoms[2]->pos = clipper::Coord_orth(1000, 1000, 1000);

      myminimol.write_file("minimoled-mol.pdb", 20.0);

      coot::minimol::molecule m2 = myminimol.molecule_of_atom_types(std::string(" CA "));

      m2.write_file("minimoled-cas.pdb", 20.0);
      for(int ifrag=0; ifrag<m2.fragments.size(); ifrag++)
	 std::cout << "m2 fragment " << ifrag << " has "
		   << m2[ifrag].residues.size()
		   << " residues " << std::endl; 
      std::string unused_chain = m2.unused_chain_id("W");
      std::cout << "Found unused chain " << unused_chain << std::endl;

   }
   
   coot::minimol::molecule a;
   int ifrag = a.fragment_for_chain("A");
   coot::minimol::residue res(1,"ALA");
   res.addatom(" CA ", " C", clipper::Coord_orth(0,0,0), "");
   a[ifrag].addresidue(res, 0);

   a.write_file("atest.pdb", 20.0);

   // double v = 0.12;
   // good - returns negative when given negative 0.119429  -0.119429
   // std::cout << atan2(v,1) << "  " << atan2(-v,1) << std::endl;

   
   
   return 0;
}
