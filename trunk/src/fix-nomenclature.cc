
#include <iostream> // fixes undefined strchr, strchrr problems
#include "coot-nomenclature.hh"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "protein-geometry.hh"

int
main(int argc, char **argv) {

//    int a = 5;
//    std::cout << a << "\n";
//    a = a | 4; 
//    std::cout << a << "\n";
//    a = a | 8; 
//    std::cout << a << "\n";
//    a = a | 16; 
//    std::cout << a << "\n";
//    a = a | 16; 
//    std::cout << a << "\n";

//    a = 1;
//    a = a << 1;
//    std::cout << a << "\n";
//    a = a << 1;
//    std::cout << a << "\n";
//    a = a <<= 1;
//    std::cout << a << "\n";


   if (argc < 3) {
      std::cout << "Usage: " << argv[0] << " in-filename out-filename\n";
      exit(1);
   } else {
      coot::protein_geometry geom;
      std::string filename = argv[1];
      atom_selection_container_t asc = get_atom_selection(filename);
      coot::nomenclature::nomenclature n(asc.mol);
      asc.mol->WritePDBASCII(argv[2]);
      n.fix(&geom);
   }
   return 0;
}
