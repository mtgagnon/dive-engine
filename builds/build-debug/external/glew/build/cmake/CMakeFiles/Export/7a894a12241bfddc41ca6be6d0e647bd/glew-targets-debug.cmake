#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "GLEW::glew" for configuration "Debug"
set_property(TARGET GLEW::glew APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GLEW::glew PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libGLEWd.so.2.1.0"
  IMPORTED_SONAME_DEBUG "libGLEWd.so.2.1"
  )

list(APPEND _cmake_import_check_targets GLEW::glew )
list(APPEND _cmake_import_check_files_for_GLEW::glew "${_IMPORT_PREFIX}/lib/libGLEWd.so.2.1.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
