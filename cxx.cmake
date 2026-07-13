# common cmake helpers for C++. 
# Functions prefixed with openmobilemaps__ "namespace" to make stand as "this
# is our code" out at call site.

####
# openmobilemaps__target_default_compile_options(name)
#
# Set C++ standard version and basic Warning options
####
function(openmobilemaps__target_default_compile_options name)
  target_compile_features(${name} PRIVATE cxx_std_17)
  target_compile_options(${name} PRIVATE -Werror -Wunused -Wundef -Wno-reorder)
endfunction()
