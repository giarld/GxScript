/*
 * Copyright (c) 2023 Gxin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gany_class_to_lua.h"

#include "lua_table.h"
#include "gany_lua_vm.h"


GX_NS_BEGIN

void GAnyClassToLua::toLua(lua_State *L)
{
    const luaL_Reg staticMethods[] = {
            {"Class", regClass},
            {nullptr, nullptr}
    };

    const luaL_Reg methods[] = {
            {"__gc",       regGC},
            {"__newindex", noneNewIndex},
            {"inherit",    regInherit},
            {"func",       regFunc},
            {"staticFunc", regStaticFunc},
            {"defEnum",    regDefEnum},
            {"property",    regProperty},
            {"new",        regNew},

            {nullptr,      nullptr}
    };

    lua_newtable(L);
    int top = lua_gettop(L);

    const luaL_Reg *f;
    for (f = staticMethods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }

    lua_setglobal(L, "GAnyClass");

    luaL_newmetatable(L, "GAnyClass");
    top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "_name");
    lua_pushstring(L, "GAnyClass");
    lua_settable(L, top);

    lua_pushliteral(L, "__name");
    lua_pushstring(L, "GAnyClass");
    lua_settable(L, top);

    for (f = methods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }
    lua_pop(L, lua_gettop(L));
}

void GAnyClassToLua::pushGAnyClass(lua_State *L, const GAny &v)
{
    GAny *obj = GX_NEW(GAny, v);

    void **p = (void **) lua_newuserdata(L, sizeof(void *));
    *p = obj;
    luaL_getmetatable(L, "GAnyClass");
    lua_setmetatable(L, -2);
}

int GAnyClassToLua::noneNewIndex(lua_State *L)
{
    luaL_error(L, "Cannot insert content into the current table");
    return 0;
}

int GAnyClassToLua::regClass(lua_State *L)
{
    int n = lua_gettop(L);
    if (n == 1) {
        if (!lua_istable(L, 1)) {
            luaL_error(L, "Call GAnyClass Create error: unsupported overloaded usage.");
            return 0;
        }

        auto defObj = LuaTable(L, 1).toObject();
        if (!defObj.isObject()) {
            GAny *obj = GX_NEW(GAny, GAnyClass::Class("", "", ""));

            void **p = (void **) lua_newuserdata(L, sizeof(void *));
            *p = obj;
            luaL_getmetatable(L, "GAnyClass");
            lua_setmetatable(L, -2);
            return 1;
        }
        std::string nameSpace;
        std::string className;
        std::string doc;
        if (defObj.contains("NameSpace")) {
            nameSpace = defObj["NameSpace"].toString();
        }
        if (defObj.contains("Name")) {
            className = defObj["Name"].toString();
        }
        if (defObj.contains("Doc")) {
            doc = defObj["Doc"].toString();
        }
        auto clazz = GAnyClass::Class(nameSpace, className, doc);
        if (defObj.contains("Inherit") && defObj["Inherit"].isArray()) {
            auto parents = defObj["Inherit"];
            for (int32_t i = 0; i < parents.size(); i++) {
                clazz->inherit(parents[i]);
            }
        }
        defObj.call("forEach", [&](const std::string &k, const GAny &v) {
            if (v.isFunction()) {
                clazz->func(k, v, "");
            }
            if (!v.isObject()) {
                return;
            }
            std::string type;
            if (v.contains("type")) {
                type = v["type"].toString();
            }
            if (type == "method") {
                if (v["method"].isFunction()) {
                    std::string doc;
                    bool isStatic = false;
                    if (v["doc"].isString()) {
                        doc = v["doc"].toString();
                    }
                    if (v["isStatic"].isBoolean()) {
                        isStatic = v["isStatic"].toBool();
                    }
                    clazz->func(k, v["method"], doc, !isStatic);
                }
            } else if (type == "enum") {
                std::string doc;
                if (v["doc"].isString()) {
                    doc = v["doc"].toString();
                }
                if (v["enum"].isObject()) {
                    clazz->defEnum(k, v["enum"], doc);
                }
            } else if (type == "property") {
                if (v["get"].isFunction() || v["set"].isFunction()) {
                    std::string doc;
                    if (v["doc"].isString()) {
                        doc = v["doc"].toString();
                    }
                    GAny getter;
                    GAny setter;
                    if (v["get"].isFunction()) {
                        getter = v["get"];
                    }
                    if (v["set"].isFunction()) {
                        setter = v["set"];
                    }
                    clazz->property(k, getter, setter, doc);
                }
            }
        });

        GAny *obj = GX_NEW(GAny, clazz);

        void **p = (void **) lua_newuserdata(L, sizeof(void *));
        *p = obj;
        luaL_getmetatable(L, "GAnyClass");
        lua_setmetatable(L, -2);

        return 1;
    } else if (n == 3) {
        if (!(lua_type(L, 1) == LUA_TSTRING && lua_type(L, 2) == LUA_TSTRING && lua_type(L, 3) == LUA_TSTRING)) {
            luaL_error(L, "Call GAnyClass Create error: unsupported overloaded usage.");
            return 0;
        }

        std::string nameSpace = lua_tostring(L, 1);
        std::string name = lua_tostring(L, 2);
        std::string doc = lua_tostring(L, 3);

        GAny *obj = GX_NEW(GAny, GAnyClass::Class(nameSpace, name, doc));

        void **p = (void **) lua_newuserdata(L, sizeof(void *));
        *p = obj;
        luaL_getmetatable(L, "GAnyClass");
        lua_setmetatable(L, -2);

        return 1;
    }

    luaL_error(L, "Call GAnyClass Create error: unsupported overloaded usage.");
    return 0;
}

int GAnyClassToLua::regGC(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass __gc error: null object");
        return 0;
    }
    GX_DELETE(self);

    return 0;
}

int GAnyClassToLua::regInherit(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass inherit error: null object");
        return 0;
    }

    GAny *parent = glua_getcppobject(L, GAny, 2);
    if (!parent) {
        luaL_error(L, "Call GAnyClass inherit error: arg1 is null object");
        return 0;
    }

    self->as<GAnyClass>().inherit(*parent);

    pushGAnyClass(L, *self);

    return 1;
}

int GAnyClassToLua::regFunc(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 3 || n > 4) {
        luaL_error(L, "Call GAnyClass func error: unsupported overloaded usage.");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass func error: null object");
        return 0;
    }

    std::string doc;
    if (n == 4 && lua_type(L, 4) == LUA_TSTRING) {
        doc = lua_tostring(L, 4);
    }

    if (lua_type(L, 2) == LUA_TSTRING && (lua_isfunction(L, 3) || GAnyLuaVM::isGAnyLuaObj(L, 3))) {
        std::string name = lua_tostring(L, 2);
        GAny function = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        if (!function.isFunction()) {
            goto fail;
        }
        self->as<GAnyClass>().func(name, function, doc, true);

        pushGAnyClass(L, *self);
        return 1;
    }
    if (lua_isinteger(L, 2) && (lua_isfunction(L, 3) || GAnyLuaVM::isGAnyLuaObj(L, 3))) {
        auto type = (MetaFunction) lua_tointeger(L, 2);
        GAny function = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        if (!function.isFunction()) {
            goto fail;
        }
        self->as<GAnyClass>().func(type, function, doc, true);

        pushGAnyClass(L, *self);
        return 1;
    }

fail:
    luaL_error(L, "Call GAnyClass func error: unsupported overloaded usage.");
    return 0;
}

int GAnyClassToLua::regStaticFunc(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 3 || n > 4) {
        luaL_error(L, "Call GAnyClass staticFunc error: unsupported overloaded usage.");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass staticFunc error: null object");
        return 0;
    }

    std::string doc;
    if (n == 4 && lua_type(L, 4) == LUA_TSTRING) {
        doc = lua_tostring(L, 4);
    }

    if (lua_type(L, 2) == LUA_TSTRING && (lua_isfunction(L, 3) || GAnyLuaVM::isGAnyLuaObj(L, 3))) {
        std::string name = lua_tostring(L, 2);
        GAny function = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        if (!function.isFunction()) {
            goto fail;
        }
        self->as<GAnyClass>().func(name, function, doc, false);

        pushGAnyClass(L, *self);
        return 1;
    }
    if (lua_isinteger(L, 2) && (lua_isfunction(L, 3) || GAnyLuaVM::isGAnyLuaObj(L, 3))) {
        auto type = (MetaFunction) lua_tointeger(L, 2);
        GAny function = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        if (!function.isFunction()) {
            goto fail;
        }
        self->as<GAnyClass>().func(type, function, doc, false);

        pushGAnyClass(L, *self);
        return 1;
    }

fail:
    luaL_error(L, "Call GAnyClass staticFunc error: unsupported overloaded usage.");
    return 0;
}

int GAnyClassToLua::regDefEnum(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 3 || n > 4) {
        luaL_error(L, "Call GAnyClass defEnum error: unsupported overloaded usage.");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass defEnum error: null object");
        return 0;
    }

    std::string doc;
    if (n == 4 && lua_type(L, 4) == LUA_TSTRING) {
        doc = lua_tostring(L, 4);
    }

    if (lua_type(L, 2) == LUA_TSTRING && lua_istable(L, 3)) {
        std::string name = lua_tostring(L, 2);
        LuaTable lTable(L, 3);

        std::map<std::string, GAny> enumMap;
        lTable.toObject().call("forEach",
                               [&enumMap](const std::string &k, const int32_t &v) {
                                   enumMap[k] = v;
                               });
        self->as<GAnyClass>().defEnum(name, GAny::object(enumMap));

        pushGAnyClass(L, *self);
        return 1;
    }

    luaL_error(L, "Call GAnyClass defEnum error: unsupported overloaded usage.");
    return 0;
}

int GAnyClassToLua::regProperty(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 4 || n > 5) {
        luaL_error(L, "Call GAnyClass property error: unsupported overloaded usage.");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass property error: null object");
        return 0;
    }

    std::string doc;
    if (n == 5 && lua_type(L, 5) == LUA_TSTRING) {
        doc = lua_tostring(L, 5);
    }

    if (lua_type(L, 2) == LUA_TSTRING
        && (lua_isfunction(L, 3) || GAnyLuaVM::isGAnyLuaObj(L, 3))
        && (lua_isfunction(L, 4) || GAnyLuaVM::isGAnyLuaObj(L, 4))) {
        std::string name = lua_tostring(L, 2);
        GAny getFunc = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        if (!getFunc.isFunction()) {
            goto fail;
        }
        GAny setFunc = GAnyLuaVM::makeLuaObjectToGAny(L, 4);
        if (!setFunc.isFunction()) {
            goto fail;
        }
        self->as<GAnyClass>().property(name, getFunc, setFunc, doc);

        pushGAnyClass(L, *self);
        return 1;
    }

fail:
    luaL_error(L, "Call GAnyClass property error: unsupported overloaded usage.");
    return 0;
}

int GAnyClassToLua::regNew(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAnyClass regNew error: null object");
        return 0;
    }

    int nargs = lua_gettop(L) - 1;

    std::vector<GAny> args;
    args.reserve(nargs);
    for (int i = 0; i < nargs; i++) {
        args.push_back(GAnyLuaVM::makeLuaObjectToGAny(L, i + 2));
    }

    GAny obj = self->_call(args);
    GAnyLuaVM::pushGAny(L, obj);
    return 1;
}

GX_NS_END