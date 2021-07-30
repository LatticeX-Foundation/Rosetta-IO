#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "spdlog::spdlog" for configuration "Release"
set_property(TARGET spdlog::spdlog APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(spdlog::spdlog PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libspdlog.so"
  IMPORTED_SONAME_RELEASE "libspdlog.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS spdlog::spdlog )
list(APPEND _IMPORT_CHECK_FILES_FOR_spdlog::spdlog "${_IMPORT_PREFIX}/lib/libspdlog.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
