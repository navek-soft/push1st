#pragma once
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <any>

extern "C" {
#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
}

#define cluamethod(cls,fn,...) __method(#fn, [](clua::ctx_t l) -> int {\
	auto&& self = clua::class_t::self<cls>(l);\
	clua::argslist args(l,2);\
	clua::result_t&& result{ self->fn(__VA_ARGS__) };\
	for (auto&& v : result) { clua::engine::pushValue(l, v); }\
	return (int)result.size();\
})

#define cluafunction(ns,mod,cls,fn,...) \
	clua::fn_t{ #fn, [](clua::ctx_t l) -> int {\
		clua::argslist args(l);\
		return clua::call<cls>(&cls::fn, l, mod, ns __VA_OPT__(,) __VA_ARGS__);\
	} }

class clua {
public:
	using fn_t = std::pair<std::string_view, lua_CFunction>;
	using method_t = lua_CFunction;
	using ctx_t = lua_State*;
	using result_t = std::vector<std::any>;
	using table_t = std::unordered_map<std::string, std::string>;
	using vector_t = std::vector<std::string>;
	using map_t = std::unordered_map<std::string, std::any>;
	using lua_t = lua_State*;
	using lua_ptr_t = std::unique_ptr<lua_State, void(*)(lua_t)>;

	class class_t {
	public:
		struct self_t {
			mutable class_t* selfObject{ nullptr };
			self_t(class_t* c) : selfObject{ c } { ; }
			self_t(const self_t& f) { selfObject = f.selfObject; f.selfObject = nullptr; }
			~self_t() { if (selfObject) { delete selfObject; } }
			operator std::any() { return std::any{ *this }; }
		};
	private:
		std::unordered_map<std::string_view, clua::method_t> clsMethods;
	protected:
		inline void __method(const std::string_view& method, clua::method_t fn) {
			clsMethods.emplace(method, fn);
		}
	public:
		virtual ~class_t() { ; }
		static inline int __index(lua_t l) {
			class_t* object = *((class_t**)lua_touserdata(l, 1));
			if (auto&& method{ object->clsMethods.find(lua_tostring(l, 2)) }; method != object->clsMethods.end()) {
				lua_pushcfunction(l, method->second);
				return 1;
			}
			return 0;
		}
		static inline int __tostring(lua_t l) {
			//class_t* object = *((class_t**)lua_touserdata(l, 1));
			lua_pushstring(l, "class_t");
			return 1;
		}
		static inline int __gc(lua_t l) {
			if (class_t* object = *((class_t**)lua_touserdata(l, 1)); object) {
				delete object;
			}
			return 0;
		}

		template<typename SELF_T>
		static inline SELF_T* self(lua_t l) {
			void* object_p = lua_touserdata(l, 1);
			if (object_p == NULL) {
				luaL_error(l, "invalid type");
			}
			return dynamic_cast<SELF_T*>(*static_cast<class_t**>(object_p));
		}
	};

	template<typename CLS_T, typename ... CTOR_ARGS>
	static inline clua::class_t::self_t cls(CTOR_ARGS&& ... args) {
		return { dynamic_cast<clua::class_t*>(new CLS_T(args...)) };
	}

	class engine : public std::enable_shared_from_this<engine> {
	public:
		engine();
		virtual ~engine();
		void luaAddPackagePath(const std::string& path);
		inline void luaAddBasePackagePath(const std::string& path) { if (!path.empty()) { packageBasePath = path.back() == '/' ? path : (path + '/'); } }
		void luaAddRelativePackagePath(lua_t L, const std::string& path);

		std::vector<std::any> luaExecute(const std::string& luaScript, const std::string& luaFunction, const std::vector<std::any>& luaArgs);
		inline std::vector<std::any> luaExecute(const std::string& luaScript) {
			return luaExecute(luaScript, {}, {});
		}
		inline std::vector<std::any> luaExecute(const std::string& luaFunction, const std::vector<std::any>& luaArgs) {
			return luaExecute({}, luaFunction, luaArgs);
		}

		template<typename SELF_T, typename ... METHOD_T>
		ssize_t luaRegisterModule(lua_t L, SELF_T* modClassSelf, const std::string_view& modName, const std::string_view& modNamespace, METHOD_T&& ... methods) {
			std::initializer_list<fn_t> methods_list{ methods ... };
			return luaRegisterModule(L, methods_list, modClassSelf, modName, modNamespace);
		}

	protected:
		virtual inline void luaLoadModules(lua_t) { ; }
		virtual void luaPackagePath(std::string& pathPackages);
	private:
		inline lua_ptr_t S();
//		std::shared_ptr<lua_State> jitLua;
		std::unordered_set<std::string> packagePaths;
		std::string packageBasePath;
		ssize_t luaRegisterModule(lua_t L, std::initializer_list<std::pair<std::string_view, lua_CFunction>>& methods, void* modClassSelf, const std::string_view& modName, const std::string_view& modNamespace);
	public:
		static void* getModule(lua_State* l, const std::string_view& modName, const std::string_view& modNamespace);
		static void pushValue(lua_State* l, const std::any& val);
	};

	template<typename SELF_T, typename FN_T, typename ... ARGS_T>
	static inline int call(FN_T fn, lua_State* l, const std::string_view& modName, const std::string_view& modNamespace, ARGS_T&& ... args) {
		if (auto self{ (SELF_T*)(engine::getModule(l, modName, modNamespace)) }; self) {
			std::vector<std::any>&& result{ (self->*fn)(args...) };
			for (auto&& v : result) { engine::pushValue(l, v); }
			return (int)result.size();
		}
		else {
			throw std::runtime_error(std::string{ modNamespace } + "::" + std::string{ modName } + " lua module not exists");
		}
		return 0;
	}

	class argslist {
	private:
		template<typename CONT_T = map_t>
		inline std::any pop_table_any(int n) {
			CONT_T result;
			lua_pushnil(jitLua);
			std::string key;
			while (lua_next(jitLua, n)) {
				if (lua_isnumber(jitLua, -2)) {
					key.assign(std::to_string(lua_tointeger(jitLua, -2)));
				}
				else if (lua_isstring(jitLua, -2)) {
					key.assign(lua_tostring(jitLua, -2));
				}
				switch (lua_type(jitLua, -1))
				{
				case LUA_TSTRING:
					result.emplace(key, std::any(std::string{ lua_tostring(jitLua, -1) }));
					break;
				case LUA_TNUMBER:
					if (lua_isinteger(jitLua, -1)) {
						result.emplace(key, std::any((ssize_t)lua_tointeger(jitLua, -1)));
					}
					else {
						result.emplace(key, std::any((float)lua_tonumber(jitLua, -1)));
					}
					break;
				case LUA_TBOOLEAN:
					result.emplace(key, std::any((bool)lua_toboolean(jitLua, -1)));
					break;
				case LUA_TTABLE:
					result.emplace(key, pop_table_any<CONT_T>(lua_gettop(jitLua)));
					break;
				default:
					result.emplace(key, std::any(nullptr));
					break;
				}
				lua_pop(jitLua, 1);
			}
			return std::any{ result };
		}
	public:
		argslist(lua_State* l, int offset = 1) : jitLua{ l }, argOffset{ offset }{; }
		inline std::any string(int n) { if (lua_type(jitLua, n + argOffset) == LUA_TSTRING) { return std::string{ luaL_checkstring(jitLua, n + argOffset) }; } return std::string{}; }
		inline std::any array(int n) {
			std::vector<uint8_t> result;
			if (lua_type(jitLua, n + argOffset) == LUA_TSTRING) {
				size_t len;
				if (auto str{ (const uint8_t*)lua_tolstring(jitLua, n + argOffset, &len) }; len) {
					result.assign(str, str + len);
				}
			}
			return std::any(result);
		}
		inline std::any boolean(int n) { if (lua_type(jitLua, n + argOffset) == LUA_TBOOLEAN) { return (bool)lua_toboolean(jitLua, n + argOffset); } return false; }
		inline std::any number(int n) { if (lua_type(jitLua, n + argOffset) == LUA_TNUMBER) { return ssize_t{ lua_tointeger(jitLua, n + argOffset) }; } return 0; }
		inline std::any table(int n) {
			table_t result;
			if (lua_type(jitLua, n + argOffset) == LUA_TTABLE) {
				lua_pushnil(jitLua);
				while (lua_next(jitLua, n + argOffset)) {
					if (lua_isnumber(jitLua, -2)) {
						result.emplace(std::to_string(lua_tointeger(jitLua, -2)), lua_tostring(jitLua, -1));
					}
					else if (lua_isstring(jitLua, -2)) {
						result.emplace(lua_tostring(jitLua, -2), lua_tostring(jitLua, -1));
					}
					lua_pop(jitLua, 1);
				}
			}
			return std::any(result);
		}
		template<typename CONT_T = vector_t>
		inline std::any vector(int n) {
			CONT_T result;
			if (lua_type(jitLua, n + argOffset) == LUA_TTABLE) {
				lua_pushnil(jitLua);
				while (lua_next(jitLua, n + argOffset)) {
					result.emplace_back(lua_tostring(jitLua, -1));
					lua_pop(jitLua, 1);
				}
			}
			return std::any(result);
		}
		template<typename CONT_T = map_t>
		inline std::any map(int n) {
			CONT_T result;
			if (lua_type(jitLua, n + argOffset) == LUA_TTABLE) {
				lua_pushnil(jitLua);
				std::string key;
				while (lua_next(jitLua, n + argOffset)) {
					if (lua_isnumber(jitLua, -2)) {
						key.assign(std::to_string(lua_tointeger(jitLua, -2)));
					}
					else if (lua_isstring(jitLua, -2)) {
						key.assign(lua_tostring(jitLua, -2));
					}
					switch (lua_type(jitLua, -1))
					{
					case LUA_TSTRING:
						result.emplace(key, std::any(std::string{ lua_tostring(jitLua, -1) }));
						break;
					case LUA_TNUMBER:
						if (lua_isinteger(jitLua, -1)) {
							result.emplace(key, std::any((ssize_t)lua_tointeger(jitLua, -1)));
						}
						else {
							result.emplace(key, std::any((float)lua_tonumber(jitLua, -1)));
						}
						break;
					case LUA_TBOOLEAN:
						result.emplace(key, std::any((bool)lua_toboolean(jitLua, -1)));
						break;
					case LUA_TTABLE:
						result.emplace(key, pop_table_any<CONT_T>(lua_gettop(jitLua)));
						break;
					default:
						result.emplace(key, std::any(nullptr));
						break;
					}
					lua_pop(jitLua, 1);
				}
			}
			return std::any(result);
		}
	private:
		lua_State* jitLua;
		int argOffset{ 1 };
	};

};

