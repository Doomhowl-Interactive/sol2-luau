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

// A compatibility layer that shims the parts of the standard Lua C API that
// sol2 relies on but which are missing, renamed, or behave differently on
// top of the Luau programming language (https://luau.org).
//
// Luau is derived from Lua 5.1: most of the 5.1 API surface works as-is.
// This layer covers the differences:
//
//   * no `lauxlib.h`: everything lives in `lualib.h`
//     (handled by <sol/compatibility/lua_version.hpp>)
//   * no registry-based `luaL_ref`/`luaL_unref`: emulated with `lua_ref`/
//     `lua_unref`, or with append-into-table semantics for plain tables
//   * no `lua_compare`/`LUA_OP*`: emulated with `lua_equal`/`lua_lessthan`
//   * no `lua_copy`/`lua_rotate`/`lua_len`/`lua_geti`/`lua_seti`/
//     `lua_pushglobaltable`/`lua_newuserdatauv`: emulated with 5.1-style calls
//   * userdata have no per-object environment: `lua_getuservalue`/
//     `lua_setuservalue` fall back to a weak-keyed registry table
//   * no `lua_atpanic`: routed through `lua_callbacks(L)->panic`
//   * no source-loading API (`lua_load`, `luaL_loadbufferx`,
//     `luaL_loadfilex`): source is compiled with `Luau::compile` from
//     Luau.Compiler and loaded with `luau_load`; pre-compiled bytecode
//     passes straight through
//   * no `lua_dump`: stubbed out to report failure
//   * `lua_pushcclosure` takes a debug name: a 3-argument overload is
//     provided (the 4-argument macro is undefined)
//   * level-based debug info: `lua_getstack` + string-`what`
//     `lua_getinfo` are emulated over `lua_stackdepth`/`lua_getinfo`

#ifndef SOL_COMPATIBILITY_COMPAT_LUAU_H
#define SOL_COMPATIBILITY_COMPAT_LUAU_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <string>

#include <sol/compatibility/lua_version.hpp>

#if SOL_IS_ON(SOL_USE_LUAU)

#include <string>

#if defined(__has_include)
#if __has_include(<Luau/Compiler.h>)
#include <Luau/Compiler.h>
#define SOL_LUAU_COMPILER_AVAILABLE_I_ 1
#endif
#endif // __has_include support

// status codes and comparison operators that Luau does not define,
// kept in sync with the values used by sol2's Lua 5.1 path
#ifndef LUA_ERRFILE
#define LUA_ERRFILE (LUA_ERRERR + 1)
#endif
#ifndef LUA_ERRGCMM
#define LUA_ERRGCMM (LUA_ERRERR + 2)
#endif
#ifndef LUA_OPEQ
#define LUA_OPEQ 0
#endif
#ifndef LUA_OPLT
#define LUA_OPLT 1
#endif
#ifndef LUA_OPLE
#define LUA_OPLE 2
#endif

// typedefs for API pieces that exist in standard Lua but not in Luau's headers.
// they are only referenced through the shims below.
typedef const char* (*lua_Reader)(lua_State* L, void* data, size_t* size);
typedef int (*lua_Writer)(lua_State* L, const void* p, size_t sz, void* ud);
typedef int (*lua_Panic)(lua_State* L);

// Luau's luaL_error is noreturn (void); restore the standard signature
// that sol2's `return luaL_error(...)` call style expects
#undef luaL_error
template <typename... Args>
inline int luaL_error(lua_State* L, const char* fmt, Args&&... args) {
	luaL_errorL(L, fmt, std::forward<Args>(args)...);
	// not reached: luaL_errorL never returns
	return 0;
}

// Luau has no io library and therefore no stream userdata type,
// but sol2's FILE* support references this type unconditionally
#ifndef LUA_FILEHANDLE
#define LUA_FILEHANDLE "FILE*"
#endif
typedef struct luaL_Stream {
	FILE* f;
} luaL_Stream;

namespace sol_luau_compat {
	// unique addresses used as registry keys
	static const char ref_table_key = '0';
	static const char uservalue_table_key = '0';
	static const char panic_function_key = '0';

	// the level most recently "selected" by lua_getstack, so that the
	// string-based lua_getinfo overload can find it again
	inline int& last_debug_level() {
		static thread_local int level = 0;
		return level;
	}

	inline const char* load_mode_string(const char* mode) {
		// sol2 passes one of "", "bt" (any), "t" (text), or "b" (binary)
		if (mode == nullptr || mode[0] == '\0') {
			return "bt";
		}
		return mode;
	}

	inline bool looks_like_bytecode(const char* buff, size_t sz) {
		// luau_load reads the first byte as a bytecode version number
		// (small positive integer), or 0 followed by an encoded error
		// message. textual sources effectively never begin with two
		// consecutive control characters, but they very often begin with
		// whitespace: skip it before sniffing.
		size_t i = 0;
		while (i < sz && (buff[i] == ' ' || buff[i] == '\t' || buff[i] == '\r' || buff[i] == '\n')) {
			++i;
		}
		if (i >= sz) {
			return false;
		}
		unsigned char c0 = static_cast<unsigned char>(buff[i]);
		if (c0 == 0) {
			return true;
		}
		if (c0 > 31) {
			return false;
		}
		unsigned char c1 = (i + 1 < sz) ? static_cast<unsigned char>(buff[i + 1]) : 0;
		return c1 >= 1 && c1 <= 31;
	}
} // namespace sol_luau_compat

// // // references

// registry-based references go through Luau's native pinning system;
// table-based references emulate luaL_ref's append-an-integer-key behavior
inline int luaL_ref(lua_State* L, int t) {
	int abs_t = lua_absindex(L, t);
	if (abs_t == LUA_REGISTRYINDEX) {
		int ref = lua_ref(L, -1);
		lua_pop(L, 1);
		return ref;
	}
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return LUA_REFNIL;
	}
	int ref = static_cast<int>(lua_objlen(L, abs_t)) + 1;
	lua_pushinteger(L, ref);
	lua_pushvalue(L, -2);
	lua_rawset(L, abs_t);
	lua_pop(L, 1);
	return ref;
}

inline void luaL_unref(lua_State* L, int t, int ref) {
	if (ref <= 0) {
		return;
	}
	int abs_t = lua_absindex(L, t);
	if (abs_t == LUA_REGISTRYINDEX) {
		lua_unref(L, ref);
		return;
	}
	lua_pushnil(L);
	lua_rawseti(L, abs_t, ref);
}

// // // stack manipulation

inline void lua_pushglobaltable(lua_State* L) {
	lua_pushvalue(L, LUA_GLOBALSINDEX);
}

inline void lua_copy(lua_State* L, int from, int to) {
	int abs_to = lua_absindex(L, to);
	lua_pushvalue(L, from);
	lua_replace(L, abs_to);
}

inline void lua_rotate(lua_State* L, int idx, int n) {
	int abs_idx = lua_absindex(L, idx);
	int n_elems = lua_gettop(L) - abs_idx + 1;
	if (n_elems < 2) {
		return;
	}
	// a positive rotation by r brings the top r elements to the bottom of
	// the segment, which is exactly what repeatedly inserting the top
	// element at abs_idx accomplishes; sol2 only ever rotates by tiny amounts
	int r = ((n % n_elems) + n_elems) % n_elems;
	while (r-- > 0) {
		lua_insert(L, abs_idx);
	}
}

inline int lua_compare(lua_State* L, int idx1, int idx2, int op) {
	switch (op) {
	case LUA_OPEQ:
		return lua_equal(L, idx1, idx2);
	case LUA_OPLT:
		return lua_lessthan(L, idx1, idx2);
	case LUA_OPLE:
		return lua_lessthan(L, idx1, idx2) || lua_equal(L, idx1, idx2);
	default:
		return luaL_error(L, "invalid 'op' argument for lua_compare");
	}
}

inline void lua_len(lua_State* L, int idx) {
	// note: Luau has no __len metamethod, so this is always the raw length
	lua_pushinteger(L, lua_objlen(L, idx));
}

inline lua_Integer luaL_len(lua_State* L, int idx) {
	return static_cast<lua_Integer>(lua_objlen(L, idx));
}

inline int lua_geti(lua_State* L, int index, lua_Integer i) {
	lua_pushinteger(L, i);
	lua_gettable(L, index);
	return lua_type(L, -1);
}

inline void lua_seti(lua_State* L, int index, lua_Integer i) {
	lua_pushinteger(L, i);
	lua_insert(L, -2);
	lua_settable(L, index);
}

inline int lua_isinteger(lua_State* L, int idx) {
	// only true for values stored with the dedicated integer type
	return lua_type(L, idx) == LUA_TINTEGER;
}

inline void* lua_newuserdatauv(lua_State* L, size_t sz, int nuv) {
	(void)nuv;
	return lua_newuserdata(L, sz);
}

// // // uservalues
//
// Luau keeps environments for functions and threads only: userdata have no
// per-object environment. For those, keep values alive in a weak-keyed
// registry table keyed by the object's address, which preserves the
// "lives exactly as long as the owning object" semantics.

inline int sol_luau_uservalue_table(lua_State* L) {
	lua_pushlightuserdata(L, const_cast<void*>(static_cast<const void*>(&sol_luau_compat::uservalue_table_key)));
	lua_rawget(L, LUA_REGISTRYINDEX);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_createtable(L, 0, 0);
		lua_createtable(L, 0, 1);
		lua_pushliteral(L, "__mode");
		lua_pushliteral(L, "k");
		lua_rawset(L, -3);
		lua_setmetatable(L, -2);
		lua_pushlightuserdata(L, const_cast<void*>(static_cast<const void*>(&sol_luau_compat::uservalue_table_key)));
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}
	return lua_gettop(L);
}

inline int lua_getuservalue(lua_State* L, int idx) {
	int abs_idx = lua_absindex(L, idx);
	int t = lua_type(L, abs_idx);
	if (t == LUA_TFUNCTION || t == LUA_TTHREAD) {
		lua_getfenv(L, abs_idx);
		return lua_type(L, -1);
	}
	const void* p = lua_topointer(L, abs_idx);
	if (p == nullptr) {
		lua_pushnil(L);
		return LUA_TNIL;
	}
	int uv_table = sol_luau_uservalue_table(L);
	lua_pushlightuserdata(L, const_cast<void*>(p));
	lua_rawget(L, uv_table);
	lua_remove(L, uv_table);
	return lua_type(L, -1);
}

inline void lua_setuservalue(lua_State* L, int idx) {
	int abs_idx = lua_absindex(L, idx);
	int t = lua_type(L, abs_idx);
	if (t == LUA_TFUNCTION || t == LUA_TTHREAD) {
		lua_setfenv(L, abs_idx);
		return;
	}
	const void* p = lua_topointer(L, abs_idx);
	if (p == nullptr) {
		lua_pop(L, 1);
		return;
	}
	int uv_table = sol_luau_uservalue_table(L);
	lua_pushlightuserdata(L, const_cast<void*>(p));
	lua_pushvalue(L, -3);
	lua_rawset(L, uv_table);
	lua_pop(L, 2);
}

// // // debug information

inline int lua_getstack(lua_State* L, int level, lua_Debug* ar) {
	if (level < 0 || level >= lua_stackdepth(L)) {
		sol_luau_compat::last_debug_level() = 0;
		return 0;
	}
	memset(ar, 0, sizeof(*ar));
	sol_luau_compat::last_debug_level() = level;
	return 1;
}

// string-based overload matching the standard Lua signature; resolves the
// function through the level recorded by the preceding lua_getstack call
// (or the current frame when none was made)
inline int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar) {
	int level = sol_luau_compat::last_debug_level();
	return lua_getinfo(L, level, what, ar);
}

// // // panic handling

inline void sol_luau_panic_trampoline(lua_State* L, int) {
	lua_pushlightuserdata(L, const_cast<void*>(static_cast<const void*>(&sol_luau_compat::panic_function_key)));
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_Panic f = reinterpret_cast<lua_Panic>(lua_tolightuserdata(L, -1));
	lua_pop(L, 1);
	if (f != nullptr) {
		f(L);
	}
}

inline lua_Panic lua_atpanic(lua_State* L, lua_Panic panicf) {
	const void* key = static_cast<const void*>(&sol_luau_compat::panic_function_key);
	lua_pushlightuserdata(L, const_cast<void*>(key));
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_Panic old = reinterpret_cast<lua_Panic>(lua_tolightuserdata(L, -1));
	lua_pop(L, 1);
	lua_pushlightuserdata(L, const_cast<void*>(key));
	lua_pushlightuserdata(L, reinterpret_cast<void*>(panicf));
	lua_rawset(L, LUA_REGISTRYINDEX);
	lua_callbacks(L)->panic = &sol_luau_panic_trampoline;
	return old;
}

// // // closures

// Luau's macro takes an extra debug-name argument; restore the standard
// 3-argument form as an inline function
#undef lua_pushcclosure
inline void lua_pushcclosure(lua_State* L, lua_CFunction fn, int nup) {
	lua_pushcclosurek(L, fn, nullptr, nup, nullptr);
}

#undef lua_pushcfunction
inline void lua_pushcfunction(lua_State* L, lua_CFunction fn) {
	lua_pushcclosurek(L, fn, nullptr, 0, nullptr);
}

// // // bytecode dumping
//
// Luau has no way to serialize an existing closure back into bytecode
// (bytecode is produced by the compiler from source): report failure.

inline int lua_dump(lua_State*, lua_Writer, void*, int) {
	return 1;
}

// // // auxiliary library pieces Luau does not ship

inline void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup) {
	for (; l->name != nullptr; ++l) {
		for (int i = 0; i < nup; ++i) {
			lua_pushvalue(L, -nup);
		}
		lua_pushcclosure(L, l->func, nup);
		lua_setfield(L, -(nup + 2), l->name);
	}
	lua_pop(L, nup);
}

inline void luaL_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb) {
	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		// create the _LOADED registry table if this Luau state has never
		// seen a require-style registration before
		luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
	}
	lua_getfield(L, -1, modname);
	if (lua_toboolean(L, -1) == 0) {
		lua_pop(L, 1);
		lua_pushcfunction(L, openf);
		lua_pushstring(L, modname);
		lua_call(L, 1, 1);
		lua_pushvalue(L, -1);
		lua_setfield(L, -3, modname);
	}
	lua_remove(L, -2);
	if (glb != 0) {
		lua_pushvalue(L, -1);
		lua_setglobal(L, modname);
	}
}

// // // loading source and bytecode

inline int sol_luau_load(lua_State* L, const char* chunkname, const char* data, size_t sz, const char* mode) {
	const char* normalized_mode = sol_luau_compat::load_mode_string(mode);
	bool force_text = normalized_mode[0] == 't' && normalized_mode[1] == '\0';
	bool force_binary = normalized_mode[0] == 'b' && normalized_mode[1] == '\0';

#if SOL_LUAU_COMPILER_AVAILABLE_I_
	std::string bytecode;
	const char* load_data = data;
	size_t load_size = sz;
	if (!force_binary && (force_text || !sol_luau_compat::looks_like_bytecode(data, sz))) {
		bytecode = Luau::compile(std::string(data, sz));
		load_data = bytecode.data();
		load_size = bytecode.size();
	}
	if (load_size >= 1 && load_data[0] == 0) {
		// Luau::compile encodes syntax errors as a NUL version byte
		// followed by the error message: luau_load would push that
		// message but return a meaningless status, so do it ourselves
		// and report a proper syntax error
		lua_pushlstring(L, load_data + 1, load_size - 1);
		return LUA_ERRSYNTAX;
	}
	return luau_load(L, chunkname, load_data, load_size, 0);
#else
	if (!force_binary && !sol_luau_compat::looks_like_bytecode(data, sz)) {
		lua_pushliteral(L,
		     "sol2: cannot compile Luau source code: link against Luau.Compiler so that <Luau/Compiler.h> is available");
		return LUA_ERRSYNTAX;
	}
	return luau_load(L, chunkname, data, sz, 0);
#endif
}

inline int luaL_loadbufferx(lua_State* L, const char* buff, size_t sz, const char* name, const char* mode) {
	return sol_luau_load(L, name, buff, sz, mode);
}

#define luaL_loadbuffer(L, buff, sz, name) luaL_loadbufferx((L), (buff), (sz), (name), "bt")

inline int luaL_loadfilex(lua_State* L, const char* filename, const char* mode) {
	std::string chunkname;
	FILE* file;
	bool close_file;
	if (filename == nullptr) {
		file = stdin;
		close_file = false;
		chunkname = "=stdin";
	}
	else {
		file = fopen(filename, "rb");
		if (file == nullptr) {
			lua_pushfstring(L, "cannot open %s: %s", filename, strerror(errno));
			return LUA_ERRFILE;
		}
		close_file = true;
		chunkname = std::string("@") + filename;
	}
	std::string buffer;
	char read_buffer[1024];
	size_t read_size;
	while ((read_size = fread(read_buffer, 1, sizeof(read_buffer), file)) != 0) {
		buffer.append(read_buffer, read_size);
	}
	int read_error = ferror(file);
	if (close_file) {
		fclose(file);
		file = nullptr;
	}
	if (read_error != 0) {
		lua_pushfstring(L, "cannot read %s: %s", filename != nullptr ? filename : "stdin", strerror(errno));
		return LUA_ERRFILE;
	}
	return sol_luau_load(L, chunkname.c_str(), buffer.data(), buffer.size(), mode);
}

// reader-based loading: drain the reader callback completely, then hand the
// result to the compiler / bytecode loader
inline int lua_load(lua_State* L, lua_Reader reader, void* data, const char* chunkname, const char* mode) {
	std::string buffer;
	size_t piece_size = 0;
	while (const char* piece = reader(L, data, &piece_size)) {
		if (piece_size == 0) {
			break;
		}
		buffer.append(piece, piece_size);
		piece_size = 0;
	}
	return sol_luau_load(L, chunkname, buffer.data(), buffer.size(), mode);
}

#endif // SOL_USE_LUAU

#endif // SOL_COMPATIBILITY_COMPAT_LUAU_H
