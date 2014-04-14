# get-ebi.py
# Copyright 2005, 2006 by Bernhard Lohkamp
# Copyright 2005, 2006 by Paul Emsley, The University of York
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.


pdbe_server = "http://www.ebi.ac.uk"
pdbe_pdb_file_dir = "pdbe-srv/view/files"

pdbe_file_name_tail = "ent"

# sf example http://www.ebi.ac.uk/pdbe-srv/view/files/r4hrhsf.ent

global coot_tmp_dir
coot_tmp_dir = "coot-download"

# e.g. (ebi-get-pdb "1crn")
# 
# no useful return value
#
# Note that that for sf data, we need to construct something like the
# string: http://oca.ebi.ac.uk/oca-bin/send-sf?r2acesf.ent.Z and we
# don't need to strip any html (thank goodness). Also not that the
# accession code now is lower case.
#
# data-type(here string) can be 'pdb' or 'sfs' (structure factors). 
# We might like to use
# 'coordinates rather than 'pdb in the future.
# 
# The optional argument imol-coords-arg-list is necessary for
# ouptutting sfs, because we need coordinates from which we can
# calculate phases.
#

# helper function to avoid downloading empty files
# returns download filename upon success or False when fail
#
def coot_urlretrieve(url, file_name):

    """Helper function to avoid downloading empty files
    returns download filename upon success or False when fail."""

    import urllib
    local_filename = False
    class CootURLopener(urllib.FancyURLopener):
        def http_error_default(self, url, fp, errcode, errmsg, headers):
            # handle errors the way you'd like to
            # we just pass
            pass

    opener = CootURLopener()
    try:
        local_filename, header = opener.retrieve(url, file_name)
    except:
        # we could catch more here, but dont bother for now
        print "BL WARNING:: retrieve of url %s failed" %url

    return local_filename

# we dont need something like net-get-url in python 
# since we have build in functions like urlretrieve (in module urllib)

# check the directory and get url url_string.
#
def check_dir_and_get_url(dir,file_name,url_string):
    import os,urllib

    # FIXME logic, can be done better
    if (os.path.isfile(dir) or os.path.isdir(dir)):
       if (os.path.isfile(dir)):
          print dir, " is atually a file and not a dir, so we can't write to it"
       else:
          if (os.path.isdir(dir)):
              coot_urlretrieve(url_string, file_name)
          else:
              print "ERROR:: Oops - Can't write to ", dir, " directory!"
    else:
       os.mkdir(dir)
       if (os.path.isdir(dir)):
           coot_urlretrieve(url_string, file_name)
       else:
         print "ERROR:: Oops - create-directory ",dir," failed!"

# get url_string for data type (string actually) 'pdb' or 'sfs'
#
def get_url_str(id, url_string, data_type, imol_coords_arg_list):
    import operator

    #print "DEBUG:: in get_url_string:", id, url_string, data_type

    if (data_type == "pdb"):
       pdb_file_name = coot_tmp_dir + "/" + id + ".pdb." + pdbe_file_name_tail
       check_dir_and_get_url(coot_tmp_dir,pdb_file_name,url_string)
       imol_coords = handle_read_draw_molecule(pdb_file_name)
       return imol_coords

    if (data_type == "sfs"):
       sfs_file_name = coot_tmp_dir + "/" + id + ".cif"
#       print "BL DEBUG:: cif output file is: ",sfs_file_name
       imol_coords = imol_coords_arg_list
       if (operator.isNumberType(imol_coords) and imol_coords>=-1):
         check_dir_and_get_url(coot_tmp_dir, sfs_file_name, url_string)
         read_cif_data(sfs_file_name, imol_coords_arg_list) 

# Get the pdb and sfs. @var{id} is the accession code
#
def get_ebi_pdb_and_sfs(id):

    import operator,string

    imol_coords = get_ebi_pdb(id)
    if (not operator.isNumberType(imol_coords)):
       print "Failed at reading coordinates. imol-coords was ",imol_coords

    if (imol_coords < 0):	# -1 is coot code for failed read.
       print "failed to read coordinates."
    else:
       down_id = string.lower(id)
       url_str = pdbe_server + "/" + pdbe_pdb_file_dir + "/" + \
                 "r" + down_id + "sf." + \
                 pdbe_file_name_tail
       get_url_str(id, url_str, "sfs", imol_coords)

# Return a molecule number on success
# or not a number (False) or -1 on error.
#
def get_ebi_pdb(id):
    import urllib, string

    print "======= id:", id
    down_id = string.lower(id)
    url_str = pdbe_server + "/" + pdbe_pdb_file_dir + "/" + down_id + \
              "." + pdbe_file_name_tail
    imol_coords = get_url_str(id, url_str, "pdb", None)
    # e.g. http://ftp.ebi.ac.uk/pub/databases/pdb + 
    #      /validation_reports/cb/1cbs/1cbs_validation.xml.gz
    if valid_model_molecule_qm(imol_coords):
        pdb_validate(down_id, imol_coords)
    return imol_coords


# Get data and pdb for accession code id from the Electron Density
# Server.
#
# @var{id} is the accession code.
#
# returns imol of read pdb or False on error.
#
# 20050725 EDS code
#
# return a list of 3 molecule numbers [imol, map, diff_map] or False
#
#
def get_eds_pdb_and_mtz(id):
    import string
    import urllib

    # Gerard DVD Kleywegt says we can find the coords/mtz thusly:
    #
    # - model = http://eds.bmc.uu.se/eds/sfd/1cbs/pdb1cbs.ent
    # - mtz   = http://eds.bmc.uu.se/eds/sfd/1cbs/1cbs_sigmaa.mtz
    #
    # 20091222
    # - newprefix: http://eds.bmc.uu.se/eds/dfs/cb/1cbs/
    # 
    # URL:: "http://eds.bmc.uu.se/eds/sfd/sa/2sar/pdb2sar.ent"
    # URL:: "http://eds.bmc.uu.se/eds/sfd/sa/2sar/2sar_sigmaa.mtz"

    def get_cached_eds_files(accession_code):
        down_code = string.lower(accession_code)
        pdb_file_name = os.path.join("coot-download",
                                     "pdb" + down_code + ".ent")
        mtz_file_name = os.path.join("coot-download",
                                     down_code + "_sigmaa.mtz")

        if not os.path.isfile(pdb_file_name):
            return False
        else:
            if not os.path.isfile(mtz_file_name):
                return False
            else:
                imol = read_pdb(pdb_file_name)
                imol_map = make_and_draw_map(mtz_file_name, "2FOFCWT", "PH2FOFCWT", "", 0, 0)
                imol_map_d = make_and_draw_map(mtz_file_name, "FOFCWT", "PHFOFCWT", "", 0, 1)
                if not (valid_model_molecule_qm(imol) and
                        valid_map_molecule_qm(imol_map) and
                        valid_map_molecule_qm(imol_map_d)):
                    close_molecule(imol)
                    close_molecule(imol_map)
                    close_molecule(imol_map_d)
                    return False
                else:
                    return [imol, imol_map, imol_map_d]
    
    eds_site = "http://eds.bmc.uu.se/eds"
    eds_core = "http://eds.bmc.uu.se"

    # "1cbds" -> "cb/"
    #
    def mid_chars(id_code):
        if not id_code:  # check for string?
            return "//fail//"
        if not (len(id_code) == 4):
            return "/FAIL/"
        else:
            return id_code[1:3] + "/"

    # main line
    #
    cached_status = get_cached_eds_files(id)
    if not cached_status:
        
        r = coot_mkdir(coot_tmp_dir)

        if (r):
            down_id = string.lower(id)
            eds_url = eds_site + "/dfs/"
            target_pdb_file = "pdb" + down_id + ".ent"
            dir_target_pdb_file = coot_tmp_dir + "/" + target_pdb_file
            mc = mid_chars(down_id)
            model_url = eds_url + mc + down_id + "/" + target_pdb_file
            target_mtz_file = down_id + "_sigmaa.mtz"
            dir_target_mtz_file = coot_tmp_dir + "/" + target_mtz_file
            mtz_url = eds_url + mc + down_id + "/" + target_mtz_file
            eds_info_page = eds_core + "/cgi-bin/eds/uusfs?pdbCode=" + down_id

            try:
                pre_download_info = coot_get_url_as_string(eds_info_page)
                # print "INFO:: --------------- pre-download-info:", pre_download_info
                bad_map_status = "No reliable map available" in pre_download_info
            except:
                print "BL ERROR:: could not get pre_download_info from", eds_core
                bad_map_status = True
            s1 = coot_urlretrieve(model_url, dir_target_pdb_file)
            print "INFO:: read model status: ",s1
            s2 = coot_urlretrieve(mtz_url, dir_target_mtz_file)
            print "INFO:: read mtz   status: ",s2

            if bad_map_status:
                s = "This map (" + down_id + \
                    ") is marked by the EDS as \"not a reliable map\""
                info_dialog(s)

            # maybe should then not load the map!?

            r_imol = handle_read_draw_molecule(dir_target_pdb_file)
            sc_map = make_and_draw_map(dir_target_mtz_file, "2FOFCWT", "PH2FOFCWT","",0,0)
            make_and_draw_map(dir_target_mtz_file, "FOFCWT", "PHFOFCWT",
                              "", 0, 1)
            set_scrollable_map(sc_map)
            if (valid_model_molecule_qm(r_imol)):
                return r_imol
            else:
                return False

        else:
            print "Can't make directory ",coot_tmp_dir

# not sure if coot functio better or python script function coot_urlretrieve
def net_get_url(my_url, file_name):
    coot_get_url(my_url, file_name)

def get_pdb_redo(text):

    if not isinstance(text, str):
        print "BL WARNING:: No string. No accession code."
    else:
        if not (len(text) == 4):
            print "BL WARNING:: Accession code not 4 chars."
        else:
            text = string.lower(text)
            stub = "http://www.cmbi.ru.nl/pdb_redo/" + \
                   text[1:3] + \
                   "/" + text + "/" + text + "_final"
            pdb_file_name = text + "_final.pdb"
            mtz_file_name = text + "_final.mtz"
            url_pdb = stub + ".pdb"
            url_mtz = stub + ".mtz"

            print "getting", url_pdb
            net_get_url(url_pdb, pdb_file_name)
            print "getting", url_mtz
            net_get_url(url_mtz, mtz_file_name)

            read_pdb(pdb_file_name)
            print "make-and-draw-map with", mtz_file_name
            make_and_draw_map(mtz_file_name, "FWT", "PHWT", "", 0, 0)


# BL says: to test, some examples

#id = "2BSX"
#get_ebi_pdb_and_sfs(id)
#get_eds_pdb_and_mtz(id)
