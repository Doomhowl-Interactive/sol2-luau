# # # # sol2
# The MIT License (MIT)
#
# Copyright (c) 2013-2022 Rapptz, ThePhD, and contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Include guard
if(_lua_luau_build_included)
	return()
endif()
set(_lua_luau_build_included true)

# Latest known Luau release, used when the requested version
# does not map to an explicit tag
set(LUAU_LATEST_VERSION 0.734)

# Clean up the version variable: accept "luau", "luau-0.734",
# or a bare "0.734"-style release tag
if (LUA_VERSION MATCHES "([0-9]+\\.[0-9]+)")
	set(LUAU_VERSION ${CMAKE_MATCH_1})
else()
	set(LUAU_VERSION ${LUAU_LATEST_VERSION})
endif()

include(FetchContent)

FIND_PACKAGE_MESSAGE(LUAUBUILD
	"Selecting Luau ${LUAU_VERSION} from '${LUA_VERSION}' and building the Luau.VM + Luau.Compiler libraries..."
	"[${LUAU_VERSION}][${LUA_VERSION}]")

set(LUAU_BUILD_CLI OFF CACHE BOOL "Build Luau CLI" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "Build Luau tests" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "Build Luau Web module" FORCE)

FetchContent_Declare(
	luau
	GIT_REPOSITORY https://github.com/luau-lang/luau.git
	GIT_TAG ${LUAU_VERSION}
	GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(luau)

# sol2 needs both the VM (runtime) and the Compiler (source -> bytecode,
# since luau_load only accepts bytecode). Everything else comes along as
# transitive dependencies of Luau.Compiler.
add_library(lua_luau_build INTERFACE)
target_link_libraries(lua_luau_build
	INTERFACE
	Luau.VM
	Luau.Compiler)
# tell sol2 to include the headers with C++ linkage and enable its
# Luau compatibility layer
target_compile_definitions(lua_luau_build
	INTERFACE
	SOL_LUAU=1)

add_library(Lua::Lua ALIAS lua_luau_build)

set(LUA_LIBRARIES Lua::Lua)
set(LUA_INTERPRETER "")
set(LUA_INCLUDE_DIRS "")
foreach(_luau_target Luau.VM Luau.Compiler)
	get_target_property(_luau_target_includes ${_luau_target} INTERFACE_INCLUDE_DIRECTORIES)
	if (_luau_target_includes)
		list(APPEND LUA_INCLUDE_DIRS ${_luau_target_includes})
	endif()
endforeach()
set(LUA_VERSION_STRING luau-${LUAU_VERSION})
