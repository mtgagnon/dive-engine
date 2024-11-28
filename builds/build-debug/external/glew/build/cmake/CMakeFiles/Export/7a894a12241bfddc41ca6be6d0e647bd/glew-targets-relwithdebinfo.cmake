#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "GLEW::glew" for configuration "RelWithDebInfo"
set_property(TARGET GLEW::glew APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(GLEW::glew PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libGLEW.so.2.1.0"
  IMPORTED_SONAME_RELWITHDEBINFO "libGLEW.so.2.1"
  )

list(APPEND _cmake_import_check_targets GLEW::glew )
list(APPEND _cmake_import_check_files_for_GLEW::glew "${_IMPORT_PREFIX}/lib/libGLEW.so.2.1.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
