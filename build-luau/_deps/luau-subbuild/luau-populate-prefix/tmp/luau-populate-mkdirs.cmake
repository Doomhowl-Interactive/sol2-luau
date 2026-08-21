# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/bram/dev/sol2/build-luau/_deps/luau-src")
  file(MAKE_DIRECTORY "/home/bram/dev/sol2/build-luau/_deps/luau-src")
endif()
file(MAKE_DIRECTORY
  "/home/bram/dev/sol2/build-luau/_deps/luau-build"
  "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix"
  "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/tmp"
  "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/src/luau-populate-stamp"
  "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/src"
  "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/src/luau-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/src/luau-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/bram/dev/sol2/build-luau/_deps/luau-subbuild/luau-populate-prefix/src/luau-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
