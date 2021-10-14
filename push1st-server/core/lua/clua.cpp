#include "clua.h"
#include <unordered_map>
#include <set>
#include <deque>
#include <functional>
#include <ctime>

#define l L.get()

void* clua::engine::getModule(clua::lua_t L, const std::string_view& modName, const std::string_view& modNamespace) {
    if (lua_getglobal(L, modNamespace.data()); lua_istable(L, -1)) {
        if (lua_getfield(L, -1, modName.data()); lua_istable(L, -1)) {
            lua_getfield(L, -1, "__self");
            if (lua_islightuserdata(L, -1)) {
                return (void*)lua_touserdata(L, -1);
            }
        }
    }
    return nullptr;
}

ssize_t clua::engine::luaRegisterModule(lua_t L, std::initializer_list<std::pair<std::string_view, lua_CFunction>>& methods, void* modClassSelf, const std::string_view& modName, const std::string_view& modNamespace) {

    if (!modNamespace.empty()) {
        lua_getglobal(L, modNamespace.data());
        if (/*auto nfn = lua_gettop(L); */lua_isnil(L, -1)) {
            lua_newtable(L);
            //lua_pushstring(L, modNamespace.c_str());
            //lua_setfield(L, -2, "__namespace");
            lua_setglobal(L, modNamespace.data());
        }
        if (lua_getglobal(L, modNamespace.data()); lua_istable(L, -1)) {
            lua_pushstring(L, modName.data());
            {
                lua_newtable(L);
                lua_pushlightuserdata(L, modClassSelf);
                lua_setfield(L, -2, "__self");
                for (auto&& [mn, mfn] : methods) {
                    lua_pushcfunction(L, mfn);
                    lua_setfield(L, -2, mn.data());
                }
                lua_settable(L, -3);
            }
        }
    }
    else {
        lua_newtable(L);
        lua_pushlightuserdata(L, modClassSelf);
        lua_setfield(L, -2, "__self");
        for (auto&& [mn, mfn] : methods) {
            lua_pushcfunction(L, mfn);
            lua_setfield(L, -2, mn.data());
        }
        lua_setglobal(L, modName.data());
    }

    return 0;
}

std::vector<std::any> clua::engine::luaExecute(const std::string& luaScript, const std::string& luaFunction, const std::vector<std::any>& luaArgs) {
    std::vector<std::any> multiReturn;
    if (auto&& L{ S() }; L) {
        if (!luaScript.empty()) {
            if (luaL_dofile(l, luaScript.c_str()) != 0) {
                fprintf(stderr, "clua::execute(`%s`): %s\n", luaScript.c_str(), lua_tostring(l, -1));
                lua_pop(l, 1);
                return {};
            }
        }
        if (!luaFunction.empty()) {
            auto multi_return_top = lua_gettop(l);
            try {
                if (lua_getglobal(l, luaFunction.c_str()); lua_isfunction(l, -1)) {
                    for (auto&& v : luaArgs) { pushValue(l, v); }
                    if (lua_pcall(l, (int)luaArgs.size(), LUA_MULTRET, 0) == 0) {
                        if (int multi_return_count = lua_gettop(l) - multi_return_top; multi_return_count) {
                            multiReturn.reserve(multi_return_count);
                            for (; multi_return_count; --multi_return_count) {
                                if (lua_isinteger(l, -multi_return_count)) {
                                    multiReturn.emplace_back(std::any{ ssize_t(lua_tointeger(l, -multi_return_count)) });
                                }
                                else if (lua_isboolean(l, -multi_return_count)) {
                                    multiReturn.emplace_back(std::any{ bool(lua_toboolean(l, -multi_return_count)) });
                                }
                                else if (lua_isnil(l, -multi_return_count)) {
                                    multiReturn.emplace_back(std::any{ nullptr });
                                }
                                else if (lua_isstring(l, -multi_return_count)) {
                                    size_t ptr_len{ 0 };
                                    if (auto ptr_str = lua_tolstring(l, -multi_return_count, &ptr_len); ptr_str && ptr_len) {
                                        multiReturn.emplace_back(std::string{ ptr_str,ptr_str + ptr_len });
                                    }
                                    else {
                                        multiReturn.emplace_back(std::string{ });
                                    }
                                }
                                else if (lua_istable(l, -multi_return_count)) {
                                    std::unordered_map<std::string, std::string> map;
                                    auto ntop = lua_gettop(l);
                                    lua_pushnil(l);
                                    while (lua_next(l, -multi_return_count - 1)) {
                                        if (lua_isnumber(l, -2)) {
                                            map.emplace(std::to_string(lua_tointeger(l, -2)), lua_tostring(l, -1));
                                        }
                                        else if (lua_isstring(l, -2)) {
                                            map.emplace(lua_tostring(l, -2), lua_tostring(l, -1));
                                        }
                                        lua_pop(l, 1);
                                    }
                                    multiReturn.emplace_back(map);
                                    lua_settop(l, ntop);
                                }
                            }
                        }
                    }
                    else {
                        fprintf(stderr, "clua::execute(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(l, -1));
                        return {};
                    }
                }
                else {
                    fprintf(stderr, "clua::execute(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(l, -1));
                    return {};
                }

                lua_settop(l, multi_return_top);
            }
            catch (std::exception& ex) {
                fprintf(stderr, "clua::exception(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(l, -1));
            }
        }
    }
    else {
        fprintf(stderr, "clua::execute(error script): %s\n", "invalid lua context (script is empty)");
    }
    return multiReturn;
}

void clua::engine::luaAddRelativePackagePath(lua_t L, const std::string& path) {
    if (!path.empty()) {
        std::string pPath;
        if (path.front() != '/') {
            pPath = packageBasePath + path;
        }
        else {
            pPath = path;
        }
        packagePaths.emplace(pPath);

        if (L) {
            lua_getglobal(L, "package");
            lua_getfield(L, -1, "path");
            std::string cur_path{ lua_tostring(L, -1) };
            cur_path.append(";" + pPath);
            lua_pop(L, 1);
            lua_pushstring(L, cur_path.c_str());
            lua_setfield(L, -2, "path");
            lua_pop(L, 1);
        }
    }
}

void clua::engine::luaAddPackagePath(const std::string& path) {
    if (!path.empty()) {
        if (path.back() == '/')
            packagePaths.emplace(path + "?.lua");
        else
            packagePaths.emplace(path + "/?.lua");
    }
}

void clua::engine::luaPackagePath(std::string& pathPackages) {
    for (auto&& path : packagePaths) {
        pathPackages.append(";" + path);
    }
}

clua::engine::engine() {

}
clua::engine::~engine() {

}

clua::lua_ptr_t clua::engine::S() {
    lua_ptr_t jitLua{ luaL_newstate(), [](lua_t L) {
        lua_close(L);
    } };

    luaL_openlibs(jitLua.get());

    /* adjust lua package.path */
    {
        lua_getglobal(jitLua.get(), "package");
        lua_getfield(jitLua.get(), -1, "path");
        std::string cur_path{ lua_tostring(jitLua.get(), -1) };

        luaPackagePath(cur_path);

        lua_pop(jitLua.get(), 1);
        lua_pushstring(jitLua.get(), cur_path.c_str());
        lua_setfield(jitLua.get(), -2, "path");
        lua_pop(jitLua.get(), 1);
    }

    luaLoadModules(jitLua.get());
    return jitLua;
}

namespace typecast {

    static void push_value(clua::lua_t L, const std::any& val);

    template<typename T>
    void push_integer(clua::lua_t L, const std::any& val) { lua_pushinteger(L, std::any_cast<T>(val)); }
    template<typename T>
    void push_number(clua::lua_t L, const std::any& val) { lua_pushnumber(L, std::any_cast<T>(val)); }
    void push_bool(clua::lua_t L, const std::any& val) { lua_pushboolean(L, std::any_cast<bool>(val)); }
    void push_string(clua::lua_t L, const std::any& val) { lua_pushstring(L, std::any_cast<const char*>(val)); }
    void push_cstring(clua::lua_t L, const std::any& val) { auto&& _v = std::any_cast<std::string>(val); lua_pushlstring(L, _v.data(), _v.length()); }
    void push_vstring(clua::lua_t L, const std::any& val) { auto&& _v = std::any_cast<std::string_view>(val); lua_pushlstring(L, _v.data(), _v.length()); }
    void push_rawdata(clua::lua_t L, const std::any& val) { auto&& _v = std::any_cast<std::vector<uint8_t>>(val); lua_pushlstring(L, (const char*)_v.data(), _v.size()); }
    void push_nil(clua::lua_t L, const std::any& val) { lua_pushnil(L); }
    void push_mapsa(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::unordered_map<std::string, std::any>>(val).empty()) {
            for (auto&& [k, v] : std::any_cast<std::unordered_map<std::string, std::any>>(val)) {
                lua_pushlstring(L, k.c_str(), k.length());
                push_value(L, v);
                lua_settable(L, -3);
            }
        }
    }
    void push_mapsva(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::unordered_map<std::string_view, std::any>>(val).empty()) {
            for (auto&& [k, v] : std::any_cast<std::unordered_map<std::string_view, std::any>>(val)) {
                lua_pushlstring(L, k.data(), k.size());
                push_value(L, v);
                lua_settable(L, -3);
            }
        }
    }
    void push_seta(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::set<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::set<std::any>>(val)) {
                lua_pushinteger(L, n++);
                push_value(L, (std::any&)v);
                lua_settable(L, -3);
            }
        }
    }
    void push_deqa(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::deque<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::deque<std::any>>(val)) {
                lua_pushinteger(L, n++);
                push_value(L, (std::any&)v);
                lua_settable(L, -3);
            }
        }
    }

    void push_deqsw(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::deque<std::string_view>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::deque<std::string_view>>(val)) {
                lua_pushinteger(L, n++);
                lua_pushlstring(L, v.data(), v.length());
                lua_settable(L, -3);
            }
        }
    }

    void push_class_t(clua::lua_t L, const std::any& val) {
        auto&& v{ std::any_cast<clua::class_t::self_t>(val) };
        void** place = (void**)lua_newuserdata(L, sizeof(void*));
        *place = v.selfObject;
        lua_newtable(L);
        lua_pushcfunction(L, clua::class_t::__index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, clua::class_t::__tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, clua::class_t::__gc);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);
        v.selfObject = nullptr; // now  __gc can free resource
    }

    void push_veca(clua::lua_t L, const std::any& val) {
        lua_newtable(L);
        if (!std::any_cast<std::vector<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::vector<std::any>>(val)) {
                lua_pushinteger(L, n++);
                push_value(L, (std::any&)v);
                lua_settable(L, -3);
            }
        }
    }

    static const std::unordered_map<size_t, std::function<void(clua::lua_t L, const std::any& val)>> PushTypecast{
        {typeid(float).hash_code(),push_number<float>},
        {typeid(double).hash_code(),push_number<double>},
        {typeid(int).hash_code(),push_integer<int>},
        {typeid(int16_t).hash_code(),push_integer<int16_t>},
        {typeid(uint16_t).hash_code(),push_integer<uint16_t>},
        {typeid(int32_t).hash_code(),push_integer<int32_t>},
        {typeid(uint32_t).hash_code(),push_integer<uint32_t>},
        {typeid(ssize_t).hash_code(),push_integer<ssize_t>},
        {typeid(std::time_t).hash_code(),push_integer<std::time_t>},
        {typeid(size_t).hash_code(),push_integer<size_t>},
        {typeid(int64_t).hash_code(),push_integer<int64_t>},
        {typeid(uint64_t).hash_code(),push_integer<uint64_t>},
        {typeid(bool).hash_code(),push_bool},
        {typeid(const char*).hash_code(),push_string},
        {typeid(std::string).hash_code(),push_cstring},
        {typeid(std::string_view).hash_code(),push_vstring},
        {typeid(std::deque<std::any>).hash_code(),push_deqa},
        {typeid(std::deque<std::string_view>).hash_code(),push_deqsw},
        {typeid(std::vector<std::any>).hash_code(),push_veca},
        {typeid(std::set<std::any>).hash_code(),push_seta},
        {typeid(std::unordered_map<std::string,std::any>).hash_code(),push_mapsa},
        {typeid(std::unordered_map<std::string_view,std::any>).hash_code(), push_mapsva},
        {typeid(clua::class_t::self_t).hash_code(),push_class_t},
        {typeid(nullptr).hash_code(),push_nil},
    };

    void push_value(clua::lua_t L, const std::any& val) {
        if (val.has_value()) {
            if (auto&& it = typecast::PushTypecast.find(val.type().hash_code()); it != typecast::PushTypecast.end()) {
                it->second(L, val);
            }
            else {
                fprintf(stderr, "clua::typecast(%s) no found\n", val.type().name());
            }
        }
        else {
            push_nil(L, { nullptr });
        }
    }
}

void clua::engine::pushValue(clua::lua_t L, const std::any& val) {
    typecast::push_value(L, val);
}
