#include "clua.h"
#include <unordered_map>
#include <set>
#include <deque>
#include <functional>
#include <ctime>

#define L S().get()

void* clua::engine::getModule(lua_State* l, const std::string_view& modName, const std::string_view& modNamespace) {
    if (lua_getglobal(l, modNamespace.data()); lua_istable(l, -1)) {
        if (lua_getfield(l, -1, modName.data()); lua_istable(l, -1)) {
            lua_getfield(l, -1, "__self");
            if (lua_islightuserdata(l, -1)) {
                return (void*)lua_touserdata(l, -1);
            }
        }
    }
    return nullptr;
}

ssize_t clua::engine::luaRegisterModule(std::initializer_list<std::pair<std::string_view, lua_CFunction>>& methods, void* modClassSelf, const std::string_view& modName, const std::string_view& modNamespace) {

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

    if (!luaScript.empty()) {
        if (luaL_dofile(L, luaScript.c_str()) != 0) {
            fprintf(stderr, "clua::execute(`%s`): %s\n", luaScript.c_str(), lua_tostring(L, -1));
            lua_pop(L, 1);
            return {};
        }
    }
    else if (L == nullptr) {
        fprintf(stderr, "clua::execute(error script): %s\n", "invalid lua context (script is empty)");
        return {};
    }

    if (!luaFunction.empty()) {
        auto multi_return_top = lua_gettop(L);
        try {
            if (lua_getglobal(L, luaFunction.c_str()); lua_isfunction(L, -1)) {
                for (auto&& v : luaArgs) { pushValue(L, v); }
                if (lua_pcall(L, (int)luaArgs.size(), LUA_MULTRET, 0) == 0) {
                    if (int multi_return_count = lua_gettop(L) - multi_return_top; multi_return_count) {
                        multiReturn.reserve(multi_return_count);
                        for (; multi_return_count; --multi_return_count) {
                            if (lua_isinteger(L, -multi_return_count)) {
                                multiReturn.emplace_back(std::any{ ssize_t(lua_tointeger(L, -multi_return_count)) });
                            }
                            else if (lua_isboolean(L, -multi_return_count)) {
                                multiReturn.emplace_back(std::any{ bool(lua_toboolean(L, -multi_return_count)) });
                            }
                            else if (lua_isnil(L, -multi_return_count)) {
                                multiReturn.emplace_back(std::any{ nullptr });
                            }
                            else if (lua_isstring(L, -multi_return_count)) {
                                size_t ptr_len{ 0 };
                                if (auto ptr_str = lua_tolstring(L, -multi_return_count, &ptr_len); ptr_str && ptr_len) {
                                    multiReturn.emplace_back(std::string{ ptr_str,ptr_str + ptr_len });
                                }
                                else {
                                    multiReturn.emplace_back(std::string{ });
                                }
                            }
                            else if (lua_istable(L, -multi_return_count)) {
                                std::unordered_map<std::string, std::string> map;
                                auto ntop = lua_gettop(L);
                                lua_pushnil(L);
                                while (lua_next(L, -multi_return_count - 1)) {
                                    if (lua_isnumber(L, -2)) {
                                        map.emplace(std::to_string(lua_tointeger(L, -2)), lua_tostring(L, -1));
                                    }
                                    else if (lua_isstring(L, -2)) {
                                        map.emplace(lua_tostring(L, -2), lua_tostring(L, -1));
                                    }
                                    lua_pop(L, 1);
                                }
                                multiReturn.emplace_back(map);
                                lua_settop(L, ntop);
                            }
                        }
                    }
                }
                else {
                    fprintf(stderr, "clua::execute(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(L, -1));
                    return {};
                }
            }
            else {
                fprintf(stderr, "clua::execute(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(L, -1));
                return {};
            }

            lua_settop(L, multi_return_top);
        }
        catch (std::exception& ex) {
            fprintf(stderr, "clua::exception(error running function `%s`): %s\n", luaFunction.c_str(), lua_tostring(L, -1));
        }
    }
    return multiReturn;
}

void clua::engine::luaAddRelativePackagePath(const std::string& path) {
    if (!path.empty()) {
        std::string pPath;
        if (path.front() != '/') {
            pPath = packageBasePath + path;
        }
        else {
            pPath = path;
        }
        packagePaths.emplace(pPath);

        if (jitLua) {
            lua_getglobal(jitLua.get(), "package");
            lua_getfield(jitLua.get(), -1, "path");
            std::string cur_path{ lua_tostring(jitLua.get(), -1) };
            cur_path.append(";" + pPath);
            lua_pop(jitLua.get(), 1);
            lua_pushstring(jitLua.get(), cur_path.c_str());
            lua_setfield(jitLua.get(), -2, "path");
            lua_pop(jitLua.get(), 1);
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

void clua::engine::luaLoadModules(std::shared_ptr<lua_State>) {

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

std::shared_ptr<lua_State> clua::engine::S() {
    if (jitLua) return jitLua;

    jitLua = std::shared_ptr<lua_State>{ luaL_newstate(), [](lua_State* l) {
        lua_close(l);
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

    luaLoadModules(jitLua);
    return jitLua;
    //*reinterpret_cast<clua**>(lua_newuserdata(L, sizeof(clua*))) = this;
    //lua_setglobal(L, "$CLUA53THIS");
}

namespace typecast {

    static void push_value(lua_State* l, const std::any& val);

    template<typename T>
    void push_integer(lua_State* l, const std::any& val) { lua_pushinteger(l, std::any_cast<T>(val)); }
    template<typename T>
    void push_number(lua_State* l, const std::any& val) { lua_pushnumber(l, std::any_cast<T>(val)); }
    void push_bool(lua_State* l, const std::any& val) { lua_pushboolean(l, std::any_cast<bool>(val)); }
    void push_string(lua_State* l, const std::any& val) { lua_pushstring(l, std::any_cast<const char*>(val)); }
    void push_cstring(lua_State* l, const std::any& val) { auto&& _v = std::any_cast<std::string>(val); lua_pushlstring(l, _v.data(), _v.length()); }
    void push_vstring(lua_State* l, const std::any& val) { auto&& _v = std::any_cast<std::string_view>(val); lua_pushlstring(l, _v.data(), _v.length()); }
    void push_rawdata(lua_State* l, const std::any& val) { auto&& _v = std::any_cast<std::vector<uint8_t>>(val); lua_pushlstring(l, (const char*)_v.data(), _v.size()); }
    void push_nil(lua_State* l, const std::any& val) { lua_pushnil(l); }
    void push_mapsa(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::unordered_map<std::string, std::any>>(val).empty()) {
            for (auto&& [k, v] : std::any_cast<std::unordered_map<std::string, std::any>>(val)) {
                lua_pushlstring(l, k.c_str(), k.length());
                push_value(l, v);
                lua_settable(l, -3);
            }
        }
    }
    void push_mapsva(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::unordered_map<std::string_view, std::any>>(val).empty()) {
            for (auto&& [k, v] : std::any_cast<std::unordered_map<std::string_view, std::any>>(val)) {
                lua_pushlstring(l, k.data(), k.size());
                push_value(l, v);
                lua_settable(l, -3);
            }
        }
    }
    void push_seta(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::set<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::set<std::any>>(val)) {
                lua_pushinteger(l, n++);
                push_value(l, (std::any&)v);
                lua_settable(l, -3);
            }
        }
    }
    void push_deqa(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::deque<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::deque<std::any>>(val)) {
                lua_pushinteger(l, n++);
                push_value(l, (std::any&)v);
                lua_settable(l, -3);
            }
        }
    }

    void push_deqsw(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::deque<std::string_view>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::deque<std::string_view>>(val)) {
                lua_pushinteger(l, n++);
                lua_pushlstring(l, v.data(), v.length());
                lua_settable(l, -3);
            }
        }
    }

    void push_class_t(lua_State* l, const std::any& val) {
        auto&& v{ std::any_cast<clua::class_t::self_t>(val) };
        void** place = (void**)lua_newuserdata(l, sizeof(void*));
        *place = v.selfObject;
        lua_newtable(l);
        lua_pushcfunction(l, clua::class_t::__index);
        lua_setfield(l, -2, "__index");
        lua_pushcfunction(l, clua::class_t::__tostring);
        lua_setfield(l, -2, "__tostring");
        lua_pushcfunction(l, clua::class_t::__gc);
        lua_setfield(l, -2, "__gc");
        lua_setmetatable(l, -2);
        v.selfObject = nullptr; // now  __gc can free resource
    }

    void push_veca(lua_State* l, const std::any& val) {
        lua_newtable(l);
        if (!std::any_cast<std::vector<std::any>>(val).empty()) {
            int n = 1;
            for (auto&& v : std::any_cast<std::vector<std::any>>(val)) {
                lua_pushinteger(l, n++);
                push_value(l, (std::any&)v);
                lua_settable(l, -3);
            }
        }
    }

    static const std::unordered_map<size_t, std::function<void(lua_State* l, const std::any& val)>> PushTypecast{
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

    void push_value(lua_State* l, const std::any& val) {
        if (val.has_value()) {
            if (auto&& it = typecast::PushTypecast.find(val.type().hash_code()); it != typecast::PushTypecast.end()) {
                it->second(l, val);
            }
            else {
                fprintf(stderr, "clua::typecast(%s) no found\n", val.type().name());
            }
        }
        else {
            push_nil(l, { nullptr });
        }
    }
}

void clua::engine::pushValue(lua_State* l, const std::any& val) {
    typecast::push_value(l, val);
}
