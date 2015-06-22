# this is the nrrd file from which you wish to generate data
#model_input_file="/home/sci/brig/Documents/Data/NRRDs/Mickey/mickey4tDCS.nrrd"

# this directory must exist.  
#It is where your data will be stored.  If you don't have one, make it
#model_output_path="/home/sci/brig/Documents/Data/NRRDs/Mickey/Output"

#binary dir
#binary_dir="../bin"

# all of the materials in your nrrd file.  
#If you only have one material then you just put mats = (0)
# as is, Biomesh does not allow for more than 10 materials (0 thru 9) 
# it is hard coded into the system that way so you'll just have to deal
# -1 means all the rest of the materials combined
#mats = (0, 1, 2, 3, 4, 5)

# labels that will be used to name your data.  
#The tight surface files for material 0 above will be named air.ts.fld.
#mat_names = ('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j')

# radius of tightening during the tightening process
mat_radii = 0

# number of times the medial axis will be refined
refinement_levels = 2

# sets a cap on the sizing field.  the smaller the number the finer the mesh
MAX_SIZING_FIELD = 10.0

# initial sizing field variable
SIZING_SCALE_VAR = 1.0

# sets tet_gen flags for mesh development
tetgen_joined_vol_flags = "pYzqAa50"

#number of iterations through the particle system.  
#The higher the number, the longer it takes and the more accurate it will be
num_particle_iters = 100
