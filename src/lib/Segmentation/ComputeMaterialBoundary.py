#!/usr/bin/env python
#  
#  For more information, please see: http://software.sci.utah.edu
#  
#  The MIT License
#  
#  Copyright (c) 2009 Scientific Computing and Imaging Institute,
#  University of Utah.
#   
#  
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#  
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#  
#    Author : Martin Cole

import string
from sys import argv
import sys
import subprocess
import re
import os
import time
import glob

if sys.version_info[0] < 3 :
    from thread import start_new_thread
else :
    from _thread import start_new_thread

from math import fabs

import Utils

maxval_re = re.compile("max: ([\d\.\d]+)")
axsizes_re = re.compile("sizes: (\d+) (\d+) (\d+)")
#space directions: (-0.671875,0,-0) (0,0.671875,0) (-0,0,5)
tri = "\(([\-\d\.]+),([\-\d\.]+),([\-\d\.]+)\)\s?"
spc_re = re.compile("space directions: %s%s%s" % (tri, tri, tri))

maxval = 0
axsizes = ()
spacing = None
print_only = False
use_shell = True
if sys.platform == "win32" :
    use_shell = False
	

def transform_with_transform(tet_file) :

	assert_exists(tet_file)
	transformed_fname = "%s_transformed.fld" % tet_file[:-4]
	transf = "%s_pad_transform.tf" % model_input_file[:-5]
	dummy,filename = os.path.split(transf)
	transf = os.path.join(model_output_path,filename)
	print("Transforming %s" % tet_file)
	trans_cmmd = r'"%s" -input %s -transform "%s" -output %s' % (os.path.join(binary_path,"TransformFieldWithTransform"), tet_file, transf, transformed_fname)
	Utils.do_system(trans_cmmd,print_only,use_shell)
	
	########## New command start
	transformed_nrrd = "%s_transformed.nrrd" % tet_file[:-4]	
        field_to_nrrd_cmd = r'"%s" %s %s' % (os.path.join(binary_path,"ConvertFieldToNrrd"), transformed_fname, transformed_nrrd)
        print("Converting %s to %s" % (tet_file,transformed_nrrd))
        Utils.do_system(field_to_nrrd_cmd,print_only,use_shell)
	########## New command end
	

	print("Done Transforming")

def assert_exists(file):
    if not(os.path.exists(file)):
        print("2 No such file %s. Perhaps you need to run a previous stage."%file)
        sys.exit(-1)

param_lines = []

if __name__ == "__main__" :

# Check that we got the right arguments......
    if len(argv) < 2 or not(os.path.exists(argv[1])):
        print('Usage: %s [model_config] [binary_path]' % argv[0])
        exit(1)

    model_config = argv[1]
    exec(open(model_config).read())
    
    model_path, dummy = os.path.split(model_config)
    
    if not(os.path.isabs(model_input_file)) :
      model_input_file = os.path.normpath(os.path.join(model_path, model_input_file))

    if not(os.path.isabs(model_output_path)) :
      model_output_path = os.path.normpath(os.path.join(model_path, model_output_path))
   
    
    if len(argv) > 2:
        binary_path = argv[2]
    else:
        binary_path = ""  

# Done checking arguments

    if not(os.path.exists(model_output_path)):
        os.makedirs(model_output_path)

    Utils.output_path = model_output_path
    Utils.current_stage = 2

    curr_work_dir = os.getcwd()
    os.chdir(model_output_path)

    Utils.delete_complete_log()
    Utils.delete_error_log()
    Utils.rec_running_log()
    
    start_time = time.time()

    isosurface_ts_file_names = []

    idx = 0    
    for i in mats :
        n = Utils.extract_root_matname(mat_names[idx])
        #convert the nrrd to a latvol.
        print("Working on material: %s" % n)
        nrrd_filename = "%s.tight.nrrd" % n
        assert_exists(nrrd_filename)
        field_filename = "%s.tight.fld" % n
        nrrd_to_field_cmd = r'"%s" %s %s' % (os.path.join(binary_path,"ConvertNrrdToField"), nrrd_filename, field_filename)
        print("Converting %s to %s" % (nrrd_filename,field_filename))
        Utils.do_system(nrrd_to_field_cmd,print_only,use_shell)
        isosurf_prefix = "%s_isosurface.ts" % n
        isosurface_ts_file_names.append(isosurf_prefix + ".fld")
        isosurf_cmd = r'"%s" %s %s.fld %s' % (os.path.join(binary_path,"ExtractIsosurface"), field_filename, isosurf_prefix, 0)
        print("Isosurfacing %s" % field_filename)
        Utils.do_system(isosurf_cmd,print_only,use_shell)
	##### New Transform
	transform_with_transform(field_filename) 
	#####
        idx = idx + 1

    join_fields_cmmd = r'"%s" -indexdata isosurface-all.ts.fld %s' % (os.path.join(binary_path,"JoinFields"), " ".join(isosurface_ts_file_names)) # isosurface-all is called when displaying (-d flag)
    #print(join_fields_cmmd)
    Utils.do_system(join_fields_cmmd,print_only,use_shell) 


    stop_time = time.time()
    f = open("compute-material-boundary-runtime.txt","w")
    f.write("%lf" % (stop_time-start_time))
    f.close()

    Utils.rec_completed_log()
    os.chdir(curr_work_dir)
