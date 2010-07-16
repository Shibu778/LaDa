if(VASP_LIBRARY)
  set(VASP_FIND_QUIETLY True)
endif(VASP_LIBRARY)

if(USE_VASP5)
  set(which_vasp_lib "vasp5")
else(USE_VASP5)
  set(which_vasp_lib "vasp")
endif(USE_VASP5)
FIND_LIBRARY(_VASP_LIBRARY
  ${which_vasp_lib}
  PATH
  $ENV{VASP_LIBRARY_DIR}
)


IF(_VASP_LIBRARY)
  set(VASP_LIBRARY ${_VASP_LIBRARY} CACHE PATH "Path to vasp library.")
  SET(VASP_FOUND TRUE)
  unset(_VASP_LIBRARY CACHE)
ENDIF(_VASP_LIBRARY)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(vasp DEFAULT_MSG VASP_LIBRARY)
