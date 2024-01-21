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

#include "gany_to_lua.h"

#include "lua_table.h"

#include <gx/debug.h>


GX_NS_BEGIN

void GAnyToLua::toLua(lua_State *L)
{
    const luaL_Reg staticMethods[] = {
            {"_create",    regGAnyCreate},
            {"_object",    regGAnyObject},
            {"_array",     regGAnyArray},
            {"_undefined", regGAnyUndefined},
            {"_null",      regGAnyNull},
            {"_parseJson", regGAnyParseJson},
            {"_equalTo",   regGAnyEqualTo},
            {"_import",    regGAnyImport},
            {"_export",    regGAnyExport},

            {nullptr,      nullptr}
    };

    const luaL_Reg methods[] = {
            {"__gc",           regGAnyGC},
            {"__tostring",     regGAnyToString},
            {"__index",        regGAnyLIndex},
            {"__newindex",     regGAnyLNewIndex},
            {"__call",         regGAnyLCall},
            {"__name",         regGAnyLName},
            {"__len",          regGAnyLLen},
            {"__add",          regGAnyLAdd},
            {"__sub",          regGAnyLSub},
            {"__mul",          regGAnyLMul},
            {"__div",          regGAnyLDiv},
            {"__unm",          regGAnyLUnm},
            {"__mod",          regGAnyLMod},
            {"__bnot",         regGAnyLBNot},
            {"__band",         regGAnyLBAnd},
            {"__bor",          regGAnyLBOr},
            {"__bxor",         regGAnyLBXor},
            {"__eq",           regGAnyLEq},
            {"__lt",           regGAnyLLt},
            {"__le",           regGAnyLLe},
            {"__pairs",        regGAnyPairs},
            {"new",            regGAnyNew},
            {"_call",          regGAnyCall},
            {"_dump",          regGAnyDump},
            {"_clone",         regGAnyClone},
            {"_classTypeName", regGAnyClassTypeName},
            {"_typeName",      regGAnyTypeName},
            {"_type",          regGAnyType},
            {"_classObject",   regGAnyClassObject},
            {"_length",        regGAnyLength},
            {"_size",          regGAnySize},
            {"_is",            regGAnyIs},
            {"_isUndefined",   regGAnyIsUndefined},
            {"_isNull",        regGAnyIsNull},
            {"_isFunction",    regGAnyIsFunction},
            {"_isClass",       regGAnyIsClass},
            {"_isException",   regGAnyIsException},
            {"_isProperty",    regGAnyIsProperty},
            {"_isObject",      regGAnyIsObject},
            {"_isArray",       regGAnyIsArray},
            {"_isInt8",        regGAnyIsInt8},
            {"_isInt16",       regGAnyIsInt16},
            {"_isInt32",       regGAnyIsInt32},
            {"_isInt64",       regGAnyIsInt64},
            {"_isFloat",       regGAnyIsFloat},
            {"_isDouble",      regGAnyIsDouble},
            {"_isNumber",      regGAnyIsNumber},
            {"_isString",      regGAnyIsString},
            {"_isBoolean",     regGAnyIsBoolean},
            {"_isUserObject",  regGAnyIsUserObject},
            {"_isEnum",        regGAnyIsEnum},
            {"_isCaller",      regGAnyIsCaller},
            {"_isTable",       regGAnyIsTable},
            {"_get",           regGAnyGet},
            {"_getItem",       regGAnyGetItem},
            {"_setItem",       regGAnySetItem},
            {"_delItem",       regGAnyDelItem},
            {"_contains",      regGAnyContains},
            {"_erase",         regGAnyErase},
            {"_pushBack",      regGAnyPushBack},
            {"_clear",         regGAnyClear},
            {"_iterator",      regGAnyIterator},
            {"_hasNext",       regGAnyHasNext},
            {"_next",          regGAnyNext},
            {"_toString",      regGAnyToString},
            {"_toInt8",        regGAnyToInt8},
            {"_toInt16",       regGAnyToInt16},
            {"_toInt32",       regGAnyToInt32},
            {"_toInt64",       regGAnyToInt64},
            {"_toFloat",       regGAnyToFloat},
            {"_toDouble",      regGAnyToDouble},
            {"_toBool",        regGAnyToBool},
            {"_toJsonString",  regGAnyToJsonString},
            {"_toTable",       regGAnyToTable},
            {"_toObject",      regGAnyToObject},

            {nullptr,          nullptr}
    };

    lua_newtable(L);
    int top = lua_gettop(L);

    const luaL_Reg *f;
    for (f = staticMethods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }

    lua_setglobal(L, "GAny");

    luaL_newmetatable(L, "GAny");
    top = lua_gettop(L);

    lua_pushliteral(L, "_name");
    lua_pushstring(L, "GAny");
    lua_settable(L, top);

    for (f = methods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }
    lua_pop(L, lua_gettop(L));


    registerEnumAnyType(L);
    registerEnumMetaFunction(L);
    registerEnumMetaFunctionS(L);
    registerRequireLs(L);
    registerLog(L);
}

void GAnyToLua::registerEnumAnyType(lua_State *L)
{
    const std::vector<std::pair<const char *, int>> enums = {
            {"undefined_t", (int) AnyType::undefined_t},
            {"null_t",      (int) AnyType::null_t},
            {"boolean_t",   (int) AnyType::boolean_t},
            {"int8_t",      (int) AnyType::int8_t},
            {"int16_t",     (int) AnyType::int16_t},
            {"int32_t",     (int) AnyType::int32_t},
            {"int64_t",     (int) AnyType::int64_t},
            {"float_t",     (int) AnyType::float_t},
            {"double_t",    (int) AnyType::double_t},
            {"string_t",    (int) AnyType::string_t},
            {"array_t",     (int) AnyType::array_t},
            {"object_t",    (int) AnyType::object_t},
            {"function_t",  (int) AnyType::function_t},
            {"class_t",     (int) AnyType::class_t},
            {"property_t",  (int) AnyType::property_t},
            {"enum_t",      (int) AnyType::enum_t},
            {"exception_t", (int) AnyType::exception_t},
            {"user_obj_t",  (int) AnyType::user_obj_t},
            {"caller_t",    (int) AnyType::caller_t}
    };

    // table
    lua_newtable(L);
    int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushinteger(L, i.second);
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "AnyType");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerEnumMetaFunction(lua_State *L)
{
    const std::vector<std::pair<const char *, int>> enums = {
            {"Init",           (int) MetaFunction::Init},
            {"Negate",         (int) MetaFunction::Negate},
            {"Addition",       (int) MetaFunction::Addition},
            {"Subtraction",    (int) MetaFunction::Subtraction},
            {"Multiplication", (int) MetaFunction::Multiplication},
            {"Division",       (int) MetaFunction::Division},
            {"Modulo",         (int) MetaFunction::Modulo},
            {"BitXor",         (int) MetaFunction::BitXor},
            {"BitOr",          (int) MetaFunction::BitOr},
            {"BitAnd",         (int) MetaFunction::BitAnd},
            {"EqualTo",        (int) MetaFunction::EqualTo},
            {"LessThan",       (int) MetaFunction::LessThan},
            {"Length",         (int) MetaFunction::Length},
            {"ToString",       (int) MetaFunction::ToString},
            {"ToInt32",        (int) MetaFunction::ToInt32},
            {"ToInt64",        (int) MetaFunction::ToInt64},
            {"ToDouble",       (int) MetaFunction::ToDouble},
            {"ToBoolean",      (int) MetaFunction::ToBoolean},
            {"ToObject",       (int) MetaFunction::ToObject}
    };

    // table
    lua_newtable(L);
    int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushinteger(L, i.second);
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "MetaFunction");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerEnumMetaFunctionS(lua_State *L)
{
    const std::vector<std::pair<const char *, std::string>> enums = {
            {"Init",           metaFunctionNames()[(size_t) MetaFunction::Init]},
            {"Negate",         metaFunctionNames()[(size_t) MetaFunction::Negate]},
            {"Addition",       metaFunctionNames()[(size_t) MetaFunction::Addition]},
            {"Subtraction",    metaFunctionNames()[(size_t) MetaFunction::Subtraction]},
            {"Multiplication", metaFunctionNames()[(size_t) MetaFunction::Multiplication]},
            {"Division",       metaFunctionNames()[(size_t) MetaFunction::Division]},
            {"Modulo",         metaFunctionNames()[(size_t) MetaFunction::Modulo]},
            {"BitXor",         metaFunctionNames()[(size_t) MetaFunction::BitXor]},
            {"BitOr",          metaFunctionNames()[(size_t) MetaFunction::BitOr]},
            {"BitAnd",         metaFunctionNames()[(size_t) MetaFunction::BitAnd]},
            {"EqualTo",        metaFunctionNames()[(size_t) MetaFunction::EqualTo]},
            {"LessThan",       metaFunctionNames()[(size_t) MetaFunction::LessThan]},
            {"Length",         metaFunctionNames()[(size_t) MetaFunction::Length]},
            {"ToString",       metaFunctionNames()[(size_t) MetaFunction::ToString]},
            {"ToInt32",        metaFunctionNames()[(size_t) MetaFunction::ToInt32]},
            {"ToInt64",        metaFunctionNames()[(size_t) MetaFunction::ToInt64]},
            {"ToDouble",       metaFunctionNames()[(size_t) MetaFunction::ToDouble]},
            {"ToBoolean",      metaFunctionNames()[(size_t) MetaFunction::ToBoolean]},
            {"ToObject",       metaFunctionNames()[(size_t) MetaFunction::ToObject]}
    };

    // table
    lua_newtable(L);
    int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushstring(L, i.second.c_str());
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "MetaFunctionS");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerRequireLs(lua_State *L)
{
    lua_pushcfunction(L, requireLs);
    lua_setglobal(L, "requireLs");
}

void GAnyToLua::registerLog(lua_State *L)
{
    lua_pushcfunction(L, printLog);
    lua_setglobal(L, "Log");

    lua_pushcfunction(L, printLogD);
    lua_setglobal(L, "LogD");

    lua_pushcfunction(L, printLogW);
    lua_setglobal(L, "LogW");

    lua_pushcfunction(L, printLogE);
    lua_setglobal(L, "LogE");
}


int GAnyToLua::noneNewIndex(lua_State *L)
{
    luaL_error(L, "Cannot insert content into the current table");
    return 0;
}

int GAnyToLua::requireLs(lua_State *L)
{
    int n = lua_gettop(L);
    if (n >= 1) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            luaL_error(L, "requireLs error: the arg1(name) requires a string");
            return 0;
        }
        std::string name = lua_tostring(L, 1);

        GAny env;
        if (n >= 2) {
            if (!GAnyLuaVM::isGAnyLuaObj(L, 2) && !lua_istable(L, 2)) {
                luaL_error(L, "requireLs error: the arg2(env) requires a GAny object or table");
                return 0;
            }
            env = GAnyLuaVM::makeLuaObjectToGAny(L, 2).toObject();
        } else {
            env = GAny::object();
        }

        auto tlLua = GAnyLuaVM::threadLocal();
        GAnyLuaVM::pushGAny(L, tlLua->requireLs(name, env));

        return 1;
    }
    luaL_error(L, "requireLs error: no relevant overloaded forms found");
    return 0;
}

int GAnyToLua::printLogF(int level, lua_State *L)
{
    int n = lua_gettop(L);
    std::stringstream msg;
    try {
        for (int i = 1; i <= n; i++) {
            msg << GAnyLuaVM::makeLuaObjectToGAny(L, i).toString();
        }
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_getglobal(L, "debug");
    lua_pushstring(L, "getinfo");
    lua_gettable(L, -2);

    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, 2);
        lua_pushstring(L, "nSl");
        if (lua_pcall(L, 2, 1, 0) == LUA_OK && lua_istable(L, -1)) {
            std::stringstream log;

            lua_pushstring(L, "short_src");
            lua_gettable(L, -2);
            if (lua_isstring(L, -1)) {
                log << lua_tostring(L, -1);
            } else {
                log << "??";
            }
            lua_pop(L, 1);
            log << "(";

            lua_pushstring(L, "currentline");
            lua_gettable(L, -2);
            if (lua_isinteger(L, -1)) {
                log << lua_tointeger(L, -1);
            } else {
                log << "?";
            }
            lua_pop(L, 1);
            log << ") : " << msg.str();
            debugPrintf(level, log.str().c_str());

            lua_pop(L, 1);
        }
    }

    return 0;
}

int GAnyToLua::printLog(lua_State *L)
{
    return printLogF(0, L);
}

int GAnyToLua::printLogD(lua_State *L)
{
    return printLogF(1, L);
}

int GAnyToLua::printLogW(lua_State *L)
{
    return printLogF(2, L);
}

int GAnyToLua::printLogE(lua_State *L)
{
    return printLogF(3, L);
}


int GAnyToLua::regGAnyCreate(lua_State *L)
{
    GAny argv;
    try {
        if (lua_gettop(L) >= 1) {
            argv = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        }
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAny *obj = GX_NEW(GAny, argv);

    void **p = (void **) lua_newuserdata(L, sizeof(void *));
    *p = obj;
    luaL_getmetatable(L, "GAny");
    lua_setmetatable(L, -2);

    return 1;
}

int GAnyToLua::regGAnyGC(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __gc error: null object");
        return 0;
    }
    GX_DELETE(self);

    return 0;
}

int GAnyToLua::regGAnyLIndex(lua_State *L)
{
    if (lua_type(L, -1) == LUA_TSTRING) {
        const char *name = lua_tostring(L, -1);
        luaL_getmetatable(L, "GAny");
        lua_getfield(L, -1, name);
        if (lua_iscfunction(L, -1)) {
            return 1;
        }
        lua_pop(L, 2);
    }

    if (lua_gettop(L) != 2) {
        luaL_error(L, "Call GAny __index error: Number of abnormal parameters");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __index error: null object");
        return 0;
    }

    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        return GAnyLuaVM::makeGAnyToLuaObject(L, self->getItem(key), true);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyLNewIndex(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        luaL_error(L, "Call GAny __newindex error: Number of abnormal parameters");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __newindex error: null object");
        return 0;
    }

    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        GAny val = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        self->setItem(key, val);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    return 0;
}

int GAnyToLua::regGAnyNew(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny new error: null object");
        return 0;
    }

    int nargs = lua_gettop(L) - 1;

    std::vector<GAny> args;
    args.reserve(nargs);
    GAny ret;
    try {
        for (int i = 0; i < nargs; i++) {
            args.push_back(GAnyLuaVM::makeLuaObjectToGAny(L, i + 2));
        }
        ret = self->_call(args);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, ret);
    return 1;
}

int GAnyToLua::regGAnyToString(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __tostring error: null object");
        return 0;
    }
    std::string str;
    try {
        str = self->toString();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    lua_pushstring(L, str.c_str());
    return 1;
}

int GAnyToLua::regGAnyLName(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __name error: null object");
        return 0;
    }
    std::string name;
    try {
        name = self->classTypeName();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyLCall(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __call error: null object");
        return 0;
    }

    int nargs = lua_gettop(L) - 1;

    std::vector<GAny> args;
    args.reserve(nargs);

    try {
        int begin = 0;
        if (self->isCaller()) {
            begin = 1;
        }
        for (int i = begin; i < nargs; i++) {
            args.push_back(GAnyLuaVM::makeLuaObjectToGAny(L, i + 2));
        }

        return GAnyLuaVM::makeGAnyToLuaObject(L, self->_call(args), true);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyLLen(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __len error: null object");
        return 0;
    }

    size_t len;
    try {
        len = self->length();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, (double) len);
    return 1;
}

int GAnyToLua::regGAnyLAdd(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs + rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLSub(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs - rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLMul(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs * rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLDiv(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs / rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLUnm(lua_State *L)
{
    GAny *lhs = glua_getcppobject(L, GAny, 1);
    if (!lhs) {
        luaL_error(L, "Call GAny __unm error: null object");
        return 0;
    }

    GAny s;
    try {
        s = -*lhs;;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLMod(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs % rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBNot(lua_State *L)
{
    GAny *lhs = glua_getcppobject(L, GAny, 1);
    if (!lhs) {
        luaL_error(L, "Call GAny __bnot error: null object");
        return 0;
    }

    GAny s;
    try {
        s = ~*lhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBAnd(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs & rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBOr(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs | rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBXor(lua_State *L)
{
    GAny s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs ^ rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLEq(lua_State *L)
{
    bool s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs == rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyLLt(lua_State *L)
{
    bool s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs < rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyLLe(lua_State *L)
{
    bool s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs <= rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyPairs(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __pairs error: null object");
        return 0;
    }

    try {
        auto iterator = self->iterator();
        lua_pushcfunction(L, GAnyToLua::regGAnyPairsClosure);
        GAnyLuaVM::pushGAny(L, iterator);
        lua_pushnil(L);
        return 3;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyPairsClosure(lua_State *L)
{
    GAny iterator = GAnyLuaVM::makeLuaObjectToGAny(L, 1);

    auto itClass = iterator.classObject();
    try {
        if (iterator.hasNext()) {
            auto item = iterator.next();
            GAnyLuaVM::makeGAnyToLuaObject(L, item.first);
            GAnyLuaVM::makeGAnyToLuaObject(L, item.second);
            return 2;
        }
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 0;
}

int GAnyToLua::regGAnyCall(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _call error: missing method name");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _call error: null object");
        return 0;
    }

    if (lua_type(L, 2) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _call error: missing method name");
        return 0;
    }

    std::string method = lua_tostring(L, 2);

    int nargs = lua_gettop(L) - 2;

    std::vector<GAny> args;
    args.reserve(nargs);

    try {
        for (int i = 0; i < nargs; i++) {
            args.push_back(GAnyLuaVM::makeLuaObjectToGAny(L, i + 3));
        }
        return GAnyLuaVM::makeGAnyToLuaObject(L, self->_call(method, args), true);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyEqualTo(lua_State *L)
{
    bool s;
    try {
        GAny lhs = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
        GAny rhs = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        s = lhs == rhs;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, (int) s);
    return 1;
}

int GAnyToLua::regGAnyDump(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _dump error: null object");
        return 0;
    }

    std::stringstream ss;
    try {
        if (self->is<GAnyClass>()) {
            ss << self->as<GAnyClass>();
        } else if (self->is<GAnyFunction>()) {
            ss << self->as<GAnyFunction>();
        } else {
            ss << *self;
        }
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

int GAnyToLua::regGAnyObject(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        GAnyLuaVM::pushGAny(L, GAny::object());
        return 1;
    }

    if (lua_istable(L, 1)) {
        try {
            GAny lTable = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
            if (lTable.is<LuaTable>()) {
                GAnyLuaVM::pushGAny(L, lTable.toObject());
                return 1;
            }
        } catch (GAnyException &e) {
            luaL_error(L, e.what());
            return 0;
        }
    }

    GAnyLuaVM::pushGAny(L, GAny::object());
    return 1;
}

int GAnyToLua::regGAnyArray(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        GAnyLuaVM::pushGAny(L, GAny::array());
        return 1;
    }

    if (lua_istable(L, 1)) {
        try {
            GAny lTable = GAnyLuaVM::makeLuaObjectToGAny(L, 1);
            if (lTable.is<LuaTable>()) {
                GAny obj = lTable.toObject();
                if (obj.isArray()) {
                    GAnyLuaVM::pushGAny(L, obj);
                    return 1;
                }
            }
        } catch (GAnyException &e) {
            luaL_error(L, e.what());
            return 0;
        }
    }

    GAnyLuaVM::pushGAny(L, GAny::array());
    return 1;
}

int GAnyToLua::regGAnyUndefined(lua_State *L)
{
    GAnyLuaVM::pushGAny(L, GAny::undefined());
    return 1;
}

int GAnyToLua::regGAnyNull(lua_State *L)
{
    GAnyLuaVM::pushGAny(L, GAny::null());
    return 1;
}

int GAnyToLua::regGAnyClone(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _clone error: null object");
        return 0;
    }

    GAny ret;
    try {
        ret = self->clone();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, ret);
    return 1;
}

int GAnyToLua::regGAnyClassTypeName(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _classTypeName error: null object");
        return 0;
    }

    std::string name;
    try {
        name = self->classTypeName();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyTypeName(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _typeName error: null object");
        return 0;
    }

    std::string name;
    try {
        name = self->typeName();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyType(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _type error: null object");
        return 0;
    }

    int type;
    try {
        type = (int) self->type();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushinteger(L, type);
    return 1;
}

int GAnyToLua::regGAnyClassObject(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _classObject error: null object");
        return 0;
    }

    GAny classObj;
    try {
        classObj = self->classObject();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, classObj);
    return 1;
}

int GAnyToLua::regGAnyLength(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _length error: null object");
        return 0;
    }

    size_t len;
    try {
        len = self->length();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, (double) len);
    return 1;
}

int GAnyToLua::regGAnySize(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _size error: null object");
        return 0;
    }

    size_t size;
    try {
        size = self->size();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, (double) size);
    return 1;
}

int GAnyToLua::regGAnyIs(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _is error: null object");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _is error: null object");
        return 0;
    }

    if (lua_type(L, 2) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _is error: arg1 not a string");
        return 0;
    }
    std::string arg1 = lua_tostring(L, 2);

    lua_pushboolean(L, (int) self->is(arg1));
    return 1;
}

int GAnyToLua::regGAnyIsUndefined(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isUndefined error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isUndefined());
    return 1;
}

int GAnyToLua::regGAnyIsNull(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isNull error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isNull());
    return 1;
}

int GAnyToLua::regGAnyIsFunction(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isFunction error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isFunction());
    return 1;
}

int GAnyToLua::regGAnyIsClass(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isClass error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isClass());
    return 1;
}

int GAnyToLua::regGAnyIsException(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isException error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isException());
    return 1;
}

int GAnyToLua::regGAnyIsProperty(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isProperty error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isProperty());
    return 1;
}

int GAnyToLua::regGAnyIsObject(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isObject error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isObject());
    return 1;
}

int GAnyToLua::regGAnyIsArray(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isArray error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isArray());
    return 1;
}

int GAnyToLua::regGAnyIsInt8(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isInt8 error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isInt8());
    return 1;
}

int GAnyToLua::regGAnyIsInt16(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isInt16 error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isInt16());
    return 1;
}

int GAnyToLua::regGAnyIsInt32(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isInt32 error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isInt32());
    return 1;
}

int GAnyToLua::regGAnyIsInt64(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isInt64 error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isInt64());
    return 1;
}

int GAnyToLua::regGAnyIsFloat(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isFloat error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isFloat());
    return 1;
}

int GAnyToLua::regGAnyIsDouble(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isDouble error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isDouble());
    return 1;
}

int GAnyToLua::regGAnyIsNumber(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isNumber error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isNumber());
    return 1;
}

int GAnyToLua::regGAnyIsString(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isString error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isString());
    return 1;
}

int GAnyToLua::regGAnyIsBoolean(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isBoolean error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isBoolean());
    return 1;
}

int GAnyToLua::regGAnyIsUserObject(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isUserObject error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isUserObject());
    return 1;
}

int GAnyToLua::regGAnyIsEnum(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isEnum error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isEnum());
    return 1;
}

int GAnyToLua::regGAnyIsCaller(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isCaller error: null object");
        return 0;
    }

    lua_pushboolean(L, (int) self->isCaller());
    return 1;
}

int GAnyToLua::regGAnyIsTable(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isTable error: null object");
        return 0;
    }

    int is = self->isUserObject() && self->is<LuaTable>();
    lua_pushboolean(L, is);
    return 1;
}

int GAnyToLua::regGAnyGet(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _get error: null object");
        return 0;
    }

    try {
        GAnyLuaVM::makeGAnyToLuaObject(L, *self);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 1;
}

int GAnyToLua::regGAnyGetItem(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _getItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _getItem error: need a parameter");
        return 0;
    }

    GAny val;
    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        val = self->getItem(key);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, val);
    return 1;
}

int GAnyToLua::regGAnySetItem(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _setItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 3) {
        luaL_error(L, "Call GAny _setItem error: two parameters are required");
        return 0;
    }

    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        GAny val = GAnyLuaVM::makeLuaObjectToGAny(L, 3);
        self->setItem(key, val);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 0;
}

int GAnyToLua::regGAnyDelItem(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _delItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _delItem error: need a parameter");
        return 0;
    }

    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        self->delItem(key);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 0;
}

int GAnyToLua::regGAnyContains(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _contains error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _contains error: need a parameter");
        return 0;
    }

    bool v;
    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        v = self->contains(key);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, (int) v);
    return 1;
}

int GAnyToLua::regGAnyErase(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _erase error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _erase error: need a parameter");
        return 0;
    }

    try {
        GAny key = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        self->erase(key);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 0;
}

int GAnyToLua::regGAnyPushBack(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _pushBack error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _pushBack error: need a parameter");
        return 0;
    }

    try {
        GAny v = GAnyLuaVM::makeLuaObjectToGAny(L, 2);
        self->pushBack(v);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
    return 0;
}

int GAnyToLua::regGAnyClear(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _clear error: null object");
        return 0;
    }

    self->clear();
    return 0;
}

int GAnyToLua::regGAnyIterator(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _iterator error: null object");
        return 0;
    }

    GAny iterator;
    try {
        iterator = self->iterator();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, iterator);
    return 1;
}

int GAnyToLua::regGAnyHasNext(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _hasNext error: null object");
        return 0;
    }

    try {
        lua_pushboolean(L, self->hasNext());
        return 1;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyNext(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _next error: null object");
        return 0;
    }

    try {
        GAnyLuaVM::pushGAny(L, self->next());
        return 1;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyToInt8(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt8 error: null object");
        return 0;
    }

    int v;
    try {
        v = (int) self->toInt8();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushinteger(L, v);
    return 1;
}

int GAnyToLua::regGAnyToInt16(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt16 error: null object");
        return 0;
    }

    int v;
    try {
        v = (int) self->toInt16();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushinteger(L, v);
    return 1;
}

int GAnyToLua::regGAnyToInt32(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt32 error: null object");
        return 0;
    }

    int v;
    try {
        v = (int) self->toInt32();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushinteger(L, v);
    return 1;
}

int GAnyToLua::regGAnyToInt64(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt64 error: null object");
        return 0;
    }

    double v;
    try {
        v = (double) self->toInt64();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToFloat(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toFloat error: null object");
        return 0;
    }

    double v;
    try {
        v = (double) self->toFloat();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToDouble(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toDouble error: null object");
        return 0;
    }

    double v;
    try {
        v = self->toDouble();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToBool(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toBool error: null object");
        return 0;
    }

    bool v;
    try {
        v = self->toBool();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushboolean(L, (int) v);
    return 1;
}

int GAnyToLua::regGAnyToJsonString(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toJsonString error: null object");
        return 0;
    }

    int indent = -1;
    if (lua_gettop(L) >= 2 && lua_isinteger(L, 2)) {
        indent = lua_tointeger(L, 2);
    }

    std::string v;
    try {
        v = self->toJsonString(indent);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    lua_pushstring(L, v.c_str());
    return 1;
}

int GAnyToLua::regGAnyToTable(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toTable error: null object");
        return 0;
    }

    try {
        LuaTable lTable = LuaTable::fromGAnyObject(*self);
        lTable.push(L);
        return 1;
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }
}

int GAnyToLua::regGAnyToObject(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toObject error: null object");
        return 0;
    }

    GAny obj;
    try {
        obj = self->toObject();
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, obj);
    return 1;
}

int GAnyToLua::regGAnyParseJson(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Call GAny _pushBack error: need a parameter");
        return 0;
    }
    if (lua_type(L, 1) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _pushBack error: the arg1 requires a string");
        return 0;
    }

    std::string json = lua_tostring(L, 1);

    GAny obj;
    try {
        obj = GAny::parseJson(json);
    } catch (GAnyException &e) {
        luaL_error(L, e.what());
        return 0;
    }

    GAnyLuaVM::pushGAny(L, obj);
    return 1;
}

int GAnyToLua::regGAnyImport(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        luaL_error(L, "Call GAny Import error: Missing parameters");
        return 0;
    }

    if (lua_isstring(L, 1)) {
        std::string path = lua_tostring(L, 1);
        GAnyLuaVM::pushGAny(L, GAny::Import(path));
    } else {
        GAnyLuaVM::pushGAny(L, GAny::undefined());
    }
    return 1;
}

int GAnyToLua::regGAnyExport(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny Export error: null object");
        return 0;
    }

    GAny::Export(self->as<std::shared_ptr<GAnyClass>>());
    return 0;
}

GX_NS_END