# InstallMakefileConfig.cmake

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig)


if (WIN32)

  add_custom_target(
  install_makefile_config
  COMMAND ${CMAKE_COMMAND} -E remove -f
          ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig/CMakeCache.txt
  COMMAND ${CMAKE_COMMAND} -E remove_directory
          ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig/CMakeFiles
  COMMAND
    ${CMAKE_COMMAND} -Dsgrid_DIR=${CMAKE_INSTALL_PREFIX}/lib/cmake/
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DSGRID_ENABLE_KOKKOS_KERNELS=OFF
    ${CMAKE_SOURCE_DIR}/cmake/utils
  COMMAND ${CMAKE_COMMAND} --build . --target all
  COMMAND ${CMAKE_COMMAND} -P cmake_install.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig
  COMMENT "Installing configuration for makefile users."
  VERBATIM)

else()

add_custom_target(
  install_makefile_config
  COMMAND ${CMAKE_COMMAND} -E remove -f
          ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig/CMakeCache.txt
  COMMAND ${CMAKE_COMMAND} -E remove_directory
          ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig/CMakeFiles
  COMMAND
    ${CMAKE_COMMAND} -Dsgrid_DIR=${CMAKE_INSTALL_PREFIX}/lib/cmake/
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    ${CMAKE_SOURCE_DIR}/cmake/utils
  COMMAND ${CMAKE_COMMAND} --build . --target all
  COMMAND ${CMAKE_COMMAND} -P cmake_install.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/InstallMakefileConfig
  COMMENT "Installing configuration for makefile users."
  VERBATIM)

endif()