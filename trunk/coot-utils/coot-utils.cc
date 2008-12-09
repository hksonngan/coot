/* coot-utils/coot-utils.cc
 * 
 * Copyright 2004, 2005, 2006 by The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

// Portability (to Windows, particularly) functions go here.
// 

#include "coot-sysdep.h"
#if defined _MSC_VER
#include <direct.h>
#include <windows.h>
#include <lm.h>
#else
#if !defined(WINDOWS_MINGW)
#include <pwd.h>
#endif // MINGW
#endif

// These 2 are for getpwnam
#include <sys/types.h>

#include <ctype.h>  // for toupper

#include <sys/stat.h>   // for mkdir
#include <sys/types.h>
#if defined WINDOWS_MINGW
#include <unistd.h>   // BL says: in windows needed for mkdir
#endif // MINGW

#include "clipper/core/hkl_compute.h"
#include "coot-utils.hh"



std::string
coot::util::append_dir_dir (const std::string &s1, const std::string &dir) { 

   std::string s;
   
   s = s1;
   s += "/";
   s += dir;

   return s;

} 

std::string
coot::util::append_dir_file(const std::string &s1, const std::string &file) {

   std::string s;
   
   s = s1;
   s += "/";
   s += file;

   return s;
}

// Return the userid and name (e.g ("paule", "Paul Emsley")) for use
// as a label in the database.  Not important to get right.
// 
std::pair<std::string, std::string> coot::get_userid_name_pair() {

   std::pair<std::string, std::string> p("unknown","unknown");
// BL says:: in windows we dont have USER and no getpwnam, so do somethign else
// and we avoid the ugly code below!!!
#if defined WINDOWS_MINGW
   const char *u = getenv("USERNAME");
   if (u) {
      p.first  = u;
      p.second = u;
   }
#else
   const char *u = getenv("USER");
#ifdef _MSC_VER
   // Man this is ugly windows code...
   LPUSER_INFO_10 pBuf = NULL;
   NET_API_STATUS nStatus;

   // Call the NetUserGetInfo function with level 10
   nStatus = NetUserGetInfo(NULL, (LPCWSTR) u, 10, (LPBYTE *)&pBuf);
   if (nStatus == NERR_Success) {
      if (pBuf) {
	 p.first  = (char *) pBuf->usri10_name;
	 p.second = (char *) pBuf->usri10_full_name;
      }
   }

#else    
   if (u) {
      struct passwd *pwbits = getpwnam(u);
      std::string uid;
      std::string nam;
      p.first  = pwbits->pw_name;
      p.second = pwbits->pw_gecos;
   }
#endif // MSC
#endif // MINGW
   return p;
}


// Return like mkdir: mkdir returns zero on success, or -1 if an error
// occurred.
// 
int
coot::util::create_directory(const std::string &dir_name) {

   int istat = -1;
   struct stat s;
   // on Windows stat works only properly if we remove the last / (if it exists)
   // everything else following seems to be fine with the /
#ifdef WINDOWS_MINGW
   std::string tmp_name = dir_name;
   if (dir_name.find_last_of("/") == dir_name.size() - 1) {
     tmp_name = dir_name.substr(0, dir_name.size() - 1);
   }
   int fstat = stat(tmp_name.c_str(), &s);
#else
   int fstat = stat(dir_name.c_str(), &s);
#endif // MINGW

   // 20060411 Totally bizarre pathology!  (FC4) If I comment out the
   // following line, we fail to create the directory, presumably
   // because the S_ISDIR returns true (so we don't make the
   // directory).
   
   if ( fstat == -1 ) { // file not exist
      // not exist
      std::cout << "INFO:: Creating directory " << dir_name << std::endl;

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
      istat = mkdir(dir_name.c_str()); 
#else
      mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR; // over-ridden
      mode = 511; // octal 777
      mode_t mode_o = umask(0);
      mode_t mkdir_mode = mode - mode_o;
      istat = mkdir(dir_name.c_str(), mkdir_mode);
      umask(mode_o); // oh yes we do, crazy.
#endif
      
   } else {
      if ( ! S_ISDIR(s.st_mode) ) {
	 // exists but is not a directory
	 istat = -1;
      } else {
	 // was a directory already
	 istat = 0; // return as if we made it
      }
   }
   return istat; 
}



std::string
coot::util::intelligent_debackslash(const std::string &s) {

   std::string filename_str = s;

#ifdef WINDOWS_MINGW
   int slen = s.length();
   for (int i=0; i<slen; i++) {
       if (filename_str[i] == '\\') {
          filename_str.replace (i,1,"/");
       }
   }
#endif
  return filename_str;
}

bool
coot::util::is_number(char c) {

   return ((c >= 48) && (c<=57));
}

bool
coot::util::is_letter(char c) {

   return (((c >= 65) && (c<=90)) || ((c >= 97) && (c<=122))) ;
}


short int
coot::is_member_p(const std::vector<std::string> &v, const std::string &a) { 

   short int ir = 0;
   int vsize = v.size();

   for (int i=0; i<vsize; i++) { 
      if (v[i] == a) { 
	 ir = 1;
	 break;
      } 
   }
   return ir;
}

// Some sort of polymorphism would be appropriate here, perhaps?
short int
coot::is_member_p(const std::vector<int> &v, const int &a) { 

   short int ir = 0;
   int vsize = v.size();

   for (int i=0; i<vsize; i++) { 
      if (v[i] == a) { 
	 ir = 1;
	 break;
      } 
   }
   return ir;
}

void
coot::remove_member(std::vector<int> *v_p, const int &a) {

   int vsize = v_p->size();

   for (int i=0; i<vsize; i++) {
      if ((*v_p)[i] == a) {
	 // shift down the others and reduct the vector length by one:
	 for (int j=(i); j<(vsize-1); j++) {
	    (*v_p)[j] =  (*v_p)[j+1];
	 }
	 v_p->resize(vsize-1);
      }
   }
} 


std::string
coot::util::Upper(const std::string &s) {

   std::string r = s;
   int nchars = s.length();
   for (int i=0; i<nchars; i++) {
      r[i] = toupper(s[i]);
   }
   return r;
}

std::string
coot::util::remove_leading_spaces(const std::string &s) {

   int sl = s.length();
   int cutpoint = 0;
   if (sl > 0) { 
      for (int i=0; i<sl; i++) {
	 if (!(s[i] == ' ')) {
	    cutpoint = i;
	    break;
	 }
      }
   }
   return s.substr(cutpoint, sl);
}

// Remove the first bit from long
// e.g. remove_string("Cottage", "tag") -> "Cote";
// 
std::string
coot::util::remove_string(const std::string &long_string, const std::string &bit) {

   std::string r = long_string;

   std::string::size_type ipos = long_string.find(bit);
   if (ipos != std::string::npos) {
      // OK, we found a match
      if ((ipos + bit.length()) < long_string.length())
	 r = r.substr(0,ipos) + r.substr((ipos+bit.length()));
      else
	 r = r.substr(0,ipos);
   } 
   return r;

} 



 

std::string
coot::util::int_to_string(int i) {
   char s[100];
   snprintf(s,99,"%d",i);
   return std::string(s);
}

std::string
coot::util::long_int_to_string(long int i) {
   char s[100];
   snprintf(s,99,"%ld",i);
   return std::string(s);
}

std::string
coot::util::float_to_string(float f) {
   char s[100];
   snprintf(s,99,"%5.2f",f);
   return std::string(s);
}

std::string
coot::util::float_to_string_using_dec_pl(float f, unsigned short int n_dec_pl) {
   char s[100];
   std::string prec="%7.";
   prec += coot::util::int_to_string(n_dec_pl);
   prec += "f";
   // snprintf(s,99,"%7.4f",f); // haha, FIXME. (use n_dec_pl, not 4)
   snprintf(s, 99, prec.c_str() ,f); // haha, FIXME. (use n_dec_pl, not 4)
   return std::string(s);
}



std::string
coot::util::plain_text_to_sequence(const std::string &s) {

   std::string r;

   for (unsigned int i=0; i<s.length(); i++) {
      // std::cout << "testing :" << s.substr(i, 1) << ":" << std::endl;
      if (coot::util::is_fasta_aa((s.substr(i, 1))))
	 r += toupper(s[i]);
   }

   return r;
}



short int
coot::util::is_fasta_aa(const std::string &a_in) {

   short int r = 0;

   std::string a(upcase(a_in));
   
   if (a == "A" || a == "G" ) { 
      r = 1;
   } else { 
      if (a == "B" 
	  || a == "C" || a == "D" || a == "E" || a == "F" || a == "H" || a == "I"
	  || a == "K" || a == "L" || a == "M" || a == "N" || a == "P" || a == "Q" 
	  || a == "R" || a == "S" || a == "T" || a == "U" || a == "V" || a == "W" 
	  || a == "Y" || a == "Z" || a == "X" || a == "*" || a == "-") {
	 r = 1;
      }
   }
   return r;
}

std::string
coot::util::single_quote(const std::string &s) {

   std::string r("\"");
   r += s;
   r += "\"";
   return r;
}


coot::sequence::fasta::fasta(const std::string &seq_in) {

   std::string seq;

   int nchars = seq_in.length();
   short int found_greater = 0;
   short int found_newline = 0;
   std::string t;

   for (int i=0; i<nchars; i++) {

      // std::cout << "checking character: " << seq_in[i] << std::endl;

      if (found_newline && found_greater) {
	 t = toupper(seq_in[i]);
	 if (is_fasta_aa(t)) {
	    std::cout << "adding character: " << seq_in[i] << std::endl;
	    seq += t;
	 }
      }
      if (seq_in[i] == '>') {
	 std::cout << "DEBUG:: " << seq_in[i] << " is > (greater than)\n";
	 found_greater = 1;
      }
      if (seq_in[i] == '\n') { 
	 if (found_greater) {
	    std::cout << "DEBUG:: " << seq_in[i] << " is carriage return\n";
	    found_newline = 1;
	 }
      }
   }
   
   if (seq.length() > 0) { 
      std::cout << "storing sequence: " << seq << std::endl;
   } else { 
      std::cout << "WARNING:: no sequence found or improper fasta sequence format\n";
   }
} 

bool
coot::sequence::fasta::is_fasta_aa(const std::string &a) const { 

   bool r = 0;
   
   if (a == "A" || a == "G" ) { 
      r = 1;
   } else { 
      if (a == "B" 
	  || a == "C" || a == "D" || a == "E" || a == "F" || a == "H" || a == "I"
	  || a == "K" || a == "L" || a == "M" || a == "N" || a == "P" || a == "Q" 
	  || a == "R" || a == "S" || a == "T" || a == "U" || a == "V" || a == "W" 
	  || a == "Y" || a == "Z" || a == "X" || a == "*" || a == "-") {
	 r = 1;
      }
   }
   return r;
}

// This should be a util function return the directory of this
// filename: /d/a -> /d/      "a" -> ""
std::string coot::util::file_name_directory(const std::string &file_name) {

   int end_char = -1;
   std::string rstring = "";
   
   for (int i=file_name.length()-1; i>=0; i--) {
      // std::cout << file_name.substr(0, i) << std::endl;
// BL says:: in windows we should check for \ too. Too much pain to get
// everything converted to / for file_chooser, e.g. with debackslash!
      
// Windows specific #ifdef removed 20081010, makes indenting lower done
// this file work again.
      if (file_name[i] == '/' || file_name[i] == '\\') {
	 if (i < int(file_name.length())) { 
	    end_char = i;
	 } else {
	    // never get here?
	    std::cout << "cannont happen. end_char = " << end_char
		      << " file_name.length(): " << file_name.length() << std::endl;
	    end_char = i-1;
	 }
	 break;
      }
   }
   if (end_char != -1) 
      rstring = file_name.substr(0, end_char+1);

   return rstring; 
}

std::string
coot::util::file_name_non_directory(const std::string &file_name) {

   int slash_char = -1;
   std::string rstring = "";
   
   for (int i=file_name.length()-1; i>=0; i--) {
#ifdef WINDOWS_MINGW
      if (file_name[i] == '/' || file_name[i] == '\\') {
#else
      if (file_name[i] == '/') {
#endif // MINGW
	 slash_char = i;
	 break;
      }
   }

   if (slash_char != -1) 
      rstring = file_name.substr(slash_char+1);

   // std::cout << "DEBUG:: non-directory of " << file_name << " is " << rstring << std::endl;
   return rstring;
}

// return "" on no extension /usr/blogs/thign/other
std::string
coot::util::file_name_extension(const std::string &file_name) {

   int dot_char = -1;
   std::string rstring = "";
   
   for (int i=file_name.length()-1; i>=0; i--) {
      if (file_name[i] == '.') {
	 dot_char = i;
	 break;
      }
   }

   if (dot_char != -1) 
      rstring = file_name.substr(dot_char);

   //    std::cout << "DEBUG:: extension of " << file_name << " is " << rstring << std::endl;
   return rstring;
}


short int
coot::util::extension_is_for_shelx_coords(const std::string &ext) {

   short int r = 0;
   if ((ext == ".INS") ||
       (ext == ".ins") ||
       (ext == ".RES") ||
       (ext == ".res") ||
       (ext == ".hat") ||
       (ext == ".HAT"))
      r = 1;

   return r; 
}


short int
coot::is_mmcif_filename(const std::string &filename) {

   short int i=0;

   std::string::size_type idot = filename.find_last_of(".");
   if (idot != std::string::npos) { 
      std::string t = filename.substr(idot);
   
      std::string::size_type icif   = t.rfind(".cif");
      std::string::size_type immcif = t.rfind(".mmcif");
      std::string::size_type immCIF = t.rfind(".mmCIF");

      if ( (icif   != std::string::npos) ||
	   (immcif != std::string::npos) ||
	   (immCIF != std::string::npos) ) {
	 i = 1;
      }
   }
   return i;
} 


// The user can set COOT_DATA_DIR (in fact this is the usual case
// when using binaries) and that should over-ride the built-in
// PKGDATADIR.
//
// Use this to find things in $prefix/share/coot
std::string
coot::package_data_dir() {

   std::string pkgdatadir = PKGDATADIR;
   // For binary installers, they use the environment variable:
   char *env = getenv("COOT_DATA_DIR");
   if (env)
      pkgdatadir = std::string(env);
   return pkgdatadir;
} 


std::pair<std::string, std::string>
coot::util::split_string_on_last_slash(const std::string &string_in) {

   std::string::size_type islash = string_in.find_last_of("/");
   std::string first;
   std::string second("");
   
   if (islash != std::string::npos) { 
      first  = string_in.substr(0, islash);
      second = string_in.substr(islash);
      if (second.length() > 0)
	 second = second.substr(1);
   } else {
      first = string_in;
   }
   return std::pair<std::string, std::string> (first, second);
} 

std::vector<std::string>
coot::util::split_string(const std::string &string_in,
			 const std::string &splitter) {

  
   std::vector<std::string> v;
   std::string s=string_in;

   while (1) {
      std::string::size_type isplit=s.find_first_of(splitter);
      if (isplit != std::string::npos) {
	 std::string f = s.substr(0, isplit);
	 v.push_back(f);
	 if (s.length() >= (isplit+splitter.length())) { 
	    s = s.substr(isplit+splitter.length());
	 } else {
	    break;
	 }
      } else {
	 v.push_back(s);
	 break;
      } 
   }
   return v;
}


std::string
coot::util::downcase(const std::string &s) {

   std::string r = s;
   std::string::iterator it=r.begin();

   while ( (it!=r.end()) ) { 
      *it = ::tolower(*it);
      it++;
   }
   return r;
}

std::string
coot::util::upcase(const std::string &s) {

   std::string r = s;
   std::string::iterator it=r.begin();

   while (it!=r.end()) { 
      *it = ::toupper(*it);
      it++;
   }
   return r;
}


long int
coot::util::random() { 

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
          long int r = rand();
          return r;
#else
          long int r = ::random();
          return r;
#endif
}

bool
coot::file_exists(const std::string &filename) {

   struct stat s;
   int fstat = stat(filename.c_str(), &s);
   if ( fstat == -1 ) { // file not exist
      return 0;
   } else {
      return 1;
   }
}

 bool coot::is_directory_p(const std::string &filename) {

    bool st = 0;
    struct stat s; 
   int fstat = stat(filename.c_str(), &s);
   if ( fstat == -1 ) { // file not exist
      return 0;
   } else {
      if (S_ISDIR(s.st_mode)) {
	 return 1;
      } else {
	 return 0;
      }
   }
    return st;
 }




// is ALA, GLY, TRP, MET, MSE...?
short int
coot::util::is_standard_residue_name(const std::string &residue_name) {

   if (residue_name == "ALA")
      return 1;
   if (residue_name == "ARG")
      return 1;
   if (residue_name == "ASN")
      return 1;
   if (residue_name == "ASP")
      return 1;
   if (residue_name == "CYS")
      return 1;
   if (residue_name == "GLN")
      return 1;
   if (residue_name == "GLU")
      return 1;
   if (residue_name == "GLY")
      return 1;
   if (residue_name == "HIS")
      return 1;
   if (residue_name == "ILE")
      return 1;
   if (residue_name == "LEU")
      return 1;
   if (residue_name == "LYS")
      return 1;
   if (residue_name == "MET")
      return 1;
   if (residue_name == "MSE")
      return 1;
   if (residue_name == "PHE")
      return 1;
   if (residue_name == "PRO")
      return 1;
   if (residue_name == "SER")
      return 1;
   if (residue_name == "THR")
      return 1;
   if (residue_name == "TRP")
      return 1;
   if (residue_name == "TYR")
      return 1;
   if (residue_name == "VAL")
      return 1;

   return 0;

} 
