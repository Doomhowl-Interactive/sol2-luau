// sol2

// The MIT License (MIT)

// Copyright (c) 2013-2022 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_COMPATIBILITY_LUA_VERSION_HPP
#define SOL_COMPATIBILITY_LUA_VERSION_HPP

#include <sol/version.hpp>

// clang-format off

#if defined(SOL_LUAU) && (SOL_LUAU != 0)
	// Luau is always compiled as C++: its headers must NOT be wrapped
	// in extern "C" or the functions will fail to link.
	// There is no lauxlib.h in Luau: all of the auxiliary library
	// calls live inside of lualib.h instead.
	#if __has_include(<lua/lua.h>)
		#include <lua/lua.h>
		#include <lua/lualib.h>
	#else
		#include <lua.h>
		#include <lualib.h>
	#endif
#elif SOL_IS_ON(SOL_USING_CXX_LUA)
	#if __has_include(<lua/lua.h>)
		#include <lua/lua.h>
		#include <lua/lauxlib.h>
		#include <lua/lualib.h>
	#else
		#include <lua.h>
		#include <lauxlib.h>
		#include <lualib.h>
	#endif
#elif SOL_IS_ON(SOL_USE_LUA_HPP)
	#if __has_include(<lua/lua.hpp>)
		#include <lua/lua.hpp>
	#else
		#include <lua.hpp>
	#endif
#else
	extern "C" {
		#if __has_include(<lua/lua.h>)
			#include <lua/lua.h>
			#include <lua/lauxlib.h>
			#include <lua/lualib.h>
		#else
			#include <lua.h>
			#include <lauxlib.h>
			#include <lualib.h>
		#endif
	}
#endif // C++ Mangling for Lua vs. Not

// Detect Luau when the user did not tell us about it beforehand.
// LUA_VECTOR_SIZE is defined unconditionally by Luau's luaconf.h and by
// no other Lua implementation. If we get here without SOL_LUAU being set,
// the headers were included with the wrong linkage and things would
// fail in confusing ways at link-time: bail out early with instructions.
#if !defined(SOL_LUAU) && defined(LUA_VECTOR_SIZE)
	#error "sol2: a Luau implementation was detected (LUA_VECTOR_SIZE is defined), but SOL_LUAU was not defined. Please define SOL_LUAU=1 before including sol2 when compiling against Luau, so that the headers are included with the correct (C++) linkage."
#endif // Luau detection

#if defined(SOL_LUAU) && (SOL_LUAU != 0)
	#define SOL_USE_LUAU_I_ SOL_ON
#else
	#define SOL_USE_LUAU_I_ SOL_OFF
#endif // luau

#if defined(SOL_LUAJIT)
	#if (SOL_LUAJIT != 0)
		#define SOL_USE_LUAJIT_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_I_ SOL_OFF
	#endif
#elif defined(LUAJIT_VERSION)
	#define SOL_USE_LUAJIT_I_ SOL_ON
#elif SOL_IS_ON(SOL_USING_CXX_LUAJIT)
	#define SOL_USE_LUAJIT_I_ SOL_ON
#else
	#define SOL_USE_LUAJIT_I_ SOL_DEFAULT_OFF
#endif // luajit

#if SOL_IS_ON(SOL_USING_CXX_LUAJIT)
	#include <luajit.h>
#elif SOL_IS_ON(SOL_USE_LUAJIT)
	extern "C" {
		#include <luajit.h>
	}
#endif // C++ LuaJIT ... whatever that means

#if defined(SOL_LUAJIT_VERSION)
	#define SOL_LUAJIT_VERSION_I_ SOL_LUAJIT_VERSION
#elif SOL_IS_ON(SOL_USE_LUAJIT)
	#define SOL_LUAJIT_VERSION_I_ LUAJIT_VERSION_NUM
#else
	#define SOL_LUAJIT_VERSION_I_ 0
#endif

#if defined(SOL_LUAJIT_FFI_DISABLED)
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_ON
#elif defined(LUAJIT_DISABLE_FFI)
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_ON
#else
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_DEFAULT_OFF
#endif

#if defined(MOONJIT_VERSION)
	#define SOL_USE_MOONJIT_I_ SOL_ON
#else
	#define SOL_USE_MOONJIT_I_ SOL_OFF
#endif

#if SOL_IS_ON(SOL_USE_LUAU)
	// Luau is derived from Lua 5.1: route sol2 through the 5.1 code paths
	// and use SOL_USE_LUAU to handle everything that deviates from it.
	#if defined(SOL_LUA_VERSION) && (SOL_LUA_VERSION != 501)
		#error "sol2: SOL_LUA_VERSION must be left alone (or set to 501) when compiling against Luau"
	#endif
	#define SOL_LUA_VERSION 501
#endif // Luau acts like Lua 5.1

#if !defined(SOL_LUA_VERSION)
	#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502
		#define SOL_LUA_VERSION LUA_VERSION_NUM
	#elif defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501
		#define SOL_LUA_VERSION LUA_VERSION_NUM
	#elif !defined(LUA_VERSION_NUM) || !(LUA_VERSION_NUM)
		// Definitely 5.0
		#define SOL_LUA_VERSION 500
	#else
		// ??? Not sure, assume latest?
		#define SOL_LUA_VERSION 504
	#endif // Lua Version 503, 502, 501 || luajit, 500
#endif // SOL_LUA_VERSION

#if defined(SOL_LUA_VERSION)
	#define SOL_LUA_VERSION_I_ SOL_LUA_VERSION
#else
	#define SOL_LUA_VERSION_I_ 504
#endif

// Exception safety / propagation, according to Lua information
// and user defines. Note this can sometimes change based on version information...
#if defined(SOL_EXCEPTIONS_ALWAYS_UNSAFE)
	#if (SOL_EXCEPTIONS_ALWAYS_UNSAFE != 0)
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_OFF
	#else
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_ON
	#endif
#elif defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
	#if (SOL_EXCEPTIONS_SAFE_PROPAGATION != 0)
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_ON
	#else
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_USE_LUAU)
		#if defined(LUA_USE_LONGJMP) && (LUA_USE_LONGJMP != 0)
			// longjmp-based error handling means exceptions cannot
			// safely propagate through the VM
			#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_OFF
		#else
			// Luau is built with C++ exception handling by default and
			// converts std::exception thrown from embedder code into
			// proper, catchable Lua errors, so propagation is safe.
			#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_ON
		#endif
	#elif SOL_IS_ON(SOL_USE_LUAJIT)
		#if SOL_USE(SOL_LUAJIT_VERSION) >= 20100
			// LuaJIT 2.1.0-beta3 and better have exception support locked in for all platforms (mostly)
			#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_ON
		#elif SOL_USE(SOL_LUAJIT_VERSION) >= 20000
			// LuaJIT 2.0.x have exception support only on x64 builds
			#if SOL_IS_ON(SOL_PLATFORM_X64)
				#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_ON
			#else
				#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_OFF
			#endif
		#endif
	#else
		// otherwise, there is no exception safety for
		// shoving exceptions through Lua and errors should
		// always be serialized
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_OFF
	#endif
#endif

// Some configurations work with exceptions,
// but cannot catch-all everything...
#if defined(SOL_EXCEPTIONS_CATCH_ALL)
	#if (SOL_EXCEPTIONS_CATCH_ALL != 0)
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_ON
	#else
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_USE_LUAU)
		// Luau is C++ and uses C++ exceptions itself: it is irresponsible
		// to "catch all" in this setting, just like C++ builds of Lua.
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_OFF
	#elif SOL_IS_ON(SOL_USE_LUAJIT) || SOL_IS_ON(SOL_USING_CXX_LUAJIT)
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_OFF
	#elif SOL_IS_ON(SOL_USING_CXX_LUA)
		// C++ builds of Lua will throw an exception to implement its `yield` behavior;
		// it is irresponsible to "catch all" on this setting.
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_OFF
	#else
		// Otherwise, by default, everyhting should be caught.
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_ON
	#endif
#endif

#if defined(SOL_LUAJIT_USE_EXCEPTION_TRAMPOLINE)
	#if (SOL_LUAJIT_USE_EXCEPTION_TRAMPOLINE != 0)
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_OFF(SOL_PROPAGATE_EXCEPTIONS) && SOL_IS_ON(SOL_USE_LUAJIT)
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_LUAL_STREAM_HAS_CLOSE_FUNCTION)
	#if (SOL_LUAL_STREAM_HAS_CLOSE_FUNCTION != 0)
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_ON
	#else
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_OFF(SOL_USE_LUAJIT) && (SOL_LUA_VERSION > 501)
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_ON
	#else
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined (SOL_LUA_BIT32_LIB)
	#if SOL_LUA_BIT32_LIB != 0
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#else
		#define SOL_LUA_BIT32_LIB_I_ SOL_OFF
	#endif
#else
	// Lua 5.2 only (deprecated in 5.3 (503)) (Can be turned on with Compat flags)
	// Lua 5.2, or other versions of Lua with the compat flag, or Lua that is not 5.2 with the specific define (5.4.1 either removed it entirely or broke it)
	#if SOL_IS_ON(SOL_USE_LUAU)
		// Luau ships bit32 as a standard library
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#elif (SOL_LUA_VERSION_I_ == 502)
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#elif defined(LUA_COMPAT_BITLIB)
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#elif (SOL_LUA_VERSION_I_ < 504 && defined(LUA_COMPAT_5_2))
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#else
		#define SOL_LUA_BIT32_LIB_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined (SOL_LUA_NIL_IN_TABLES)
	#if SOL_LUA_NIL_IN_TABLES != 0
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_ON
	#else
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_OFF
	#endif
#else
	#if defined(LUA_NILINTABLE) && (LUA_NILINTABLE != 0)
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_DEFAULT_ON
	#else
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_DEFAULT_OFF
	#endif
#endif

// clang-format on

#endif // SOL_COMPATIBILITY_LUA_VERSION_HPP
