/* src/sdf-interface.hh
 * 
 * Copyright 2012 by The University of Oxford
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


// 20140226: now we change things so that the interface functions always get generated,
//           and what the function does depends on MAKE_ENHANCED_LIGAND_TOOLS
// 
// #ifdef MAKE_ENHANCED_LIGAND_TOOLS
// 
// was it OK or not?  (i.e. did we not catch an exception)
bool residue_to_sdf_file(int imol, const char *chain_id, int resno, const char *ins_code, 
			 const char *sdf_file_name, bool kekulize = true);
bool residue_to_mdl_file_for_mogul(int imol, const char *chain_id,
				   int resno, const char *ins_code, 
				   const char *mdl_file_name);
// rdkit chemical features.
bool show_feats(int imol, const char *chain_id, int resno, const char *ins_code);

// #endif

