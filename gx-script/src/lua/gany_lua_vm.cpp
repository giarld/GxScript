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

#include "gany_lua_vm.h"

#include "lua_function.h"
#include "lua_table.h"

#include "gany_to_lua.h"
#include "gany_class_to_lua.h"

#include <gx/gfile.h>
#include <gx/debug.h>

#include <math.h>

#include <utility>

#ifndef LUA_BUILD_AS_CPP
extern "C" {
#endif

#include "lobject.h"
#include "lstate.h"

#ifndef LUA_BUILD_AS_CPP
}
#endif


#define HANDLE_EXCEPTION(e) \
    do {                    \
        std::string exception = std::string("GAnyLuaVM Exception: ") + e; \
        if (sExceptionHandler) {    \
            sExceptionHandler(exception);   \
            return GAny::undefined();    \
        } else {    \
            throw GAnyException(exception);  \
        }   \
    } while(false)


GX_NS_BEGIN

GAnyLuaVM::ScriptReader GAnyLuaVM::sScriptReader = nullptr;

GAnyLuaVM::ExceptionHandler GAnyLuaVM::sExceptionHandler = nullptr;

GAnyLuaVM::GAnyLuaVM()
{
    mL = luaL_newstate();
    luaL_openlibs(mL);

    GAnyToLua::toLua(mL);
    GAnyClassToLua::toLua(mL);
}

GAnyLuaVM::~GAnyLuaVM()
{
    shutdown();
}

std::shared_ptr<GAnyLuaVM> GAnyLuaVM::threadLocal()
{
    thread_local auto vm = std::make_shared<GAnyLuaVM>();
    return vm;
}

lua_State *GAnyLuaVM::getLuaState() const
{
    return mL;
}

void GAnyLuaVM::shutdown()
{
    {
        GLockerGuard locker(mFuncsLock);
        mLFuncs.clear();
    }
    if (mL) {
        lua_close(mL);
        mL = nullptr;
    }
}

GAny GAnyLuaVM::requireLs(const std::string &name, const GAny &env)
{
    std::string path = name;
    GFile file = [&]() {
        auto searchPaths = GAny::Import("getPluginSearchPaths")().castAs<std::vector<std::string>>();

        for (const auto &path: searchPaths) {
            GFile dir(path);
            if (!dir.isDirectory()) {
                continue;
            }
            GFile f(dir, name);
            if (f.exists() && f.isFile()) {
                return f;
            }
            f = GFile(dir, name + ".lsc");
            if (f.exists() && f.isFile()) {
                return f;
            }
            f = GFile(dir, name + ".lua");
            if (f.exists() && f.isFile()) {
                return f;
            }
        }

        return GFile();
    }();
    if (!file.exists()) {
        if (!sScriptReader) {
            LogE("requireLs: %s is not found", name.c_str());
            return GAny::undefined();
        } else {
            path = name;
        }
    } else {
        path = file.absoluteFilePath();
    }

    return scriptFile(path, env);
}

GAny GAnyLuaVM::script(const std::string &script, std::string sourcePath, const GAny &env)
{
    if (sourcePath.empty()) {
        GString scriptStr = script;
        // Ensure that UTF-8 characters are not truncated
        if (scriptStr.count() > 512) {
            scriptStr = scriptStr.left(512) + "...";
        }
        sourcePath = scriptStr.toStdString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    GByteArray buffer;
    buffer.write(script.data(), script.size());
    return loadScriptFromBuffer(buffer, sourcePath, env);
}

GAny GAnyLuaVM::scriptFile(const std::string &filePath, const GAny &env)
{
    GByteArray buffer;
    if (sScriptReader) {
        buffer = sScriptReader(filePath);
    } else {
        GFile file(filePath);

        if (!file.exists()) {
            HANDLE_EXCEPTION("Run lua script error: file(" + filePath + ") does not exist.");
        }

        if (file.open(GFile::ReadOnly | GFile::Binary)) {
            buffer = file.read();
            file.close();
        } else {
            HANDLE_EXCEPTION("Open file failure.");
        }
    }

    if (!buffer.isEmpty()) {
        return loadScriptFromBuffer(buffer, "@" + filePath, env);
    }

    return GAny::undefined();
}

GAny GAnyLuaVM::scriptBuffer(const GByteArray &buffer, std::string sourcePath, const GAny &env)
{
    if (sourcePath.empty()) {
        sourcePath = "@buffer://" + GByteArray::md5Sum(buffer).toHexString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    return loadScriptFromBuffer(buffer, sourcePath, env);
}

void GAnyLuaVM::gc()
{
    lua_gc(mL, LUA_GCCOLLECT, 0);
}

bool GAnyLuaVM::gcStep(int32_t kb)
{
    return lua_gc(mL, LUA_GCSTEP, kb) != 0;
}

int32_t GAnyLuaVM::gcSetStepMul(int32_t mul)
{
    return lua_gc(mL, LUA_GCSETSTEPMUL, mul);
}

int32_t GAnyLuaVM::gcSetPause(int32_t pause)
{
    return lua_gc(mL, LUA_GCSETPAUSE, pause);
}

void GAnyLuaVM::gcStop()
{
    lua_gc(mL, LUA_GCSTOP, 0);
}

void GAnyLuaVM::gcRestart()
{
    lua_gc(mL, LUA_GCRESTART, 0);
}

bool GAnyLuaVM::gcIsRunning()
{
    return lua_gc(mL, LUA_GCISRUNNING, 0) != 0;
}

int32_t GAnyLuaVM::gcGetCount()
{
    return lua_gc(mL, LUA_GCCOUNT, 0);
}

void GAnyLuaVM::gcModeGen()
{
    lua_gc(mL, LUA_GCGEN, 0);
}

void GAnyLuaVM::gcModeInc()
{
    lua_gc(mL, LUA_GCINC, 0);
}

void GAnyLuaVM::setExceptionHandler(ExceptionHandler handler)
{
    sExceptionHandler = std::move(handler);
}

void GAnyLuaVM::setScriptReader(GAnyLuaVM::ScriptReader reader)
{
    sScriptReader = std::move(reader);
}


GAny GAnyLuaVM::loadScriptFromBuffer(const GByteArray &buffer, const std::string &sourcePath, const GAny &env)
{
    lua_State *L = mL;

    if (buffer.size() - buffer.readPos() > 4) {
        char head[4];
        buffer.read(head, 4);
        if (head[0] == (char) 0xff && head[1] == 'l' && head[2] == 's' && head[3] == (char) 0xee) {
            GByteArray data;
            buffer >> data;

            if (GByteArray::isCompressed(data)) {
                data = GByteArray::uncompress(data);
            }

            if (luaL_loadbuffer(
                        L, (const char *) data.data(),
                        (size_t) data.size(),
                        (const char *) sourcePath.c_str()) != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                HANDLE_EXCEPTION(err);
            }

            GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

            if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                HANDLE_EXCEPTION(err);
            }
            GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
            lua_pop(L, 1);
            return ret;
        }
        buffer.seekReadPos(SEEK_CUR, -4);
    }

    if (luaL_loadbuffer(
                L, (const char *) buffer.data(),
                (size_t) buffer.size(),
                (const char *) sourcePath.c_str()) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        HANDLE_EXCEPTION(err);
    }

    GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        HANDLE_EXCEPTION(err);
    }
    GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
    lua_pop(L, 1);
    return ret;
}

void GAnyLuaVM::addLFunctionRef(const std::shared_ptr<LuaFunction> &ref)
{
    GLockerGuard locker(mFuncsLock);
    mLFuncs.push_back(ref);
}

void GAnyLuaVM::removeLFunctionRef(const std::shared_ptr<LuaFunction> &ref)
{
    GLockerGuard locker(mFuncsLock);
    auto fIt = std::find_if(
            mLFuncs.begin(), mLFuncs.end(),
            [&](const auto &v) {
                return v == ref;
            });
    if (fIt != mLFuncs.end()) {
        mLFuncs.erase(fIt);
    }
}

void GAnyLuaVM::pushGAny(lua_State *L, const GAny &v)
{
    GAny *obj = GX_NEW(GAny, v);

    void **p = (void **) lua_newuserdata(L, sizeof(void *));
    *p = obj;
    luaL_getmetatable(L, "GAny");
    lua_setmetatable(L, -2);
}

int GAnyLuaVM::findUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    bool hasEnv = false;
    int upIdx = 1;
    do {
        const char *envName = lua_getupvalue(L, funcIdx, upIdx);
        if (envName == nullptr) {
            break;
        }
        if (strcmp(envName, name) == 0) {
            lua_pop(L, 1);
            hasEnv = true;
            break;
        }
        lua_pop(L, 1);
        upIdx += 1;
    } while (true);

    return hasEnv ? upIdx : 0;
}

bool GAnyLuaVM::getUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    int upIdx = findUpValue(L, funcIdx, name);
    if (upIdx == 0) {
        return false;
    }
    lua_getupvalue(L, funcIdx, upIdx);
    return true;
}

bool GAnyLuaVM::setUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    int upIdx = findUpValue(L, funcIdx, name);
    if (upIdx == 0) {
        return false;
    }
    const char *upName = lua_setupvalue(L, funcIdx, upIdx);
    if (upName == nullptr) {
        lua_pop(L, 1);
        return false;
    }
    return strcmp(upName, name) == 0;
}

void GAnyLuaVM::setEnvironment(lua_State *L, const GAny &env, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    int upIdx = findUpValue(L, funcIdx, "_ENV");
    if (upIdx == 0) {
        return;
    }

    int bTop = lua_gettop(L);
    lua_newtable(L);
    // _G -> metadata.__index
    lua_newtable(L);
    int top = lua_gettop(L);
    lua_pushliteral(L, "__index");
    lua_getglobal(L, "_G");
    lua_settable(L, top);

    // _ENV
    lua_setmetatable(L, -2);
    top = lua_gettop(L);
    if (env.isObject()) {
        lua_pushliteral(L, "LEnv");
        pushGAny(L, env);
        lua_settable(L, top);

        env.call("forEach", [&](const std::string &k, const GAny &v) {
            lua_pushstring(L, k.c_str());
            pushGAny(L, v);
            lua_settable(L, top);
        });
    }
    const char *upName = lua_setupvalue(L, funcIdx, upIdx);

    int eTop = lua_gettop(L);
    if (eTop - bTop > 0) {
        lua_pop(L, eTop - bTop);
    }
}

GAny GAnyLuaVM::getEnvironment(lua_State *L, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    int upIdx = findUpValue(L, funcIdx, "_ENV");

    if (upIdx == 0) {
        return GAny::undefined();
    }
    lua_getupvalue(L, funcIdx, upIdx);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "LEnv");
        if (isGAnyLuaObj(L, -1)) {
            GAny *obj = glua_getcppobject(L, GAny, -1);
            GAny lEnv = obj ? *obj : GAny::undefined();
            lua_pop(L, 2);
            return lEnv;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return GAny::undefined();
}

std::vector<UpValueItem> GAnyLuaVM::dumpUpValue(lua_State *L, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    std::vector<UpValueItem> upValues;
    int upIdx = 1;
    do {
        const char *name = lua_getupvalue(L, funcIdx, upIdx);
        if (name == nullptr) {
            break;
        }
        if (strcmp(name, "_ENV") == 0) {
            lua_pop(L, 1);
            upIdx += 1;
            continue;
        }

        if (isGAnyLuaObj(L, -1)) {
            upValues.push_back({upIdx, *glua_getcppobject(L, GAny, lua_gettop(L)), false});
        } else {
            upValues.push_back({upIdx, makeLuaObjectToGAny(L, lua_gettop(L)), true});
        }

        lua_pop(L, 1);
        upIdx += 1;
    } while (true);

    return upValues;
}

void GAnyLuaVM::storeUpValue(lua_State *L, int funcIdx, const std::vector<UpValueItem> &upValues)
{
    GX_ASSERT(funcIdx > 0);
    for (const auto &item: upValues) {
        if (item.luaType) {
            makeGAnyToLuaObject(L, item.val);
        } else {
            pushGAny(L, item.val);
        }
        if (lua_setupvalue(L, funcIdx, item.upIdx) == nullptr) {
            LogW("GAnyLua, storeUpValue lua_setupvalue error, index: %d", item.upIdx);
            lua_pop(L, 1);
        }
    }
}

GAny GAnyLuaVM::makeLuaFunctionToGAny(lua_State *L, int idx)
{
    /// Dump the LEnv of a function
    std::weak_ptr<GAnyValue> lEnvRef = getEnvironment(L, idx).value();
    /// Dump the upper value of a function
    std::vector<UpValueItem> upValues = dumpUpValue(L, idx);

    /// Dump function
    auto vm = GAnyLuaVM::threadLocal();
    auto lFunc = std::make_shared<LuaFunction>(L, idx);
    vm->addLFunctionRef(lFunc);

    auto funcRef = std::make_shared<GLuaFunctionRef>();
    funcRef->byteCode = lFunc->dump();
    funcRef->func = lFunc;

    std::stringstream fns;
    fns << "LuaFunction<" << std::hex << lFunc.get() << ">";

    std::string fn = fns.str();
    /// Build GAnyFunction, which will proxy the call from GAny to Lua function
    GAnyFunction func = GAnyFunction::createVariadicFunction(
            fn, "",
            [funcRef, lEnvRef, fn, upValues](const GAny **args, int32_t argc) -> GAny {
                auto vm = GAnyLuaVM::threadLocal();
                if (!vm) {
                    HANDLE_EXCEPTION("Failed to get thread local lua vm!");
                }

                lua_State *L = vm->getLuaState();

                auto lFunc = funcRef->func.lock();
                auto lEnv = lEnvRef.lock();

                while (lFunc) {
                    /// If the current thread is the thread created by the Lua function, call it directly
                    if (!lFunc->checkVM()) {
                        break;
                    }

                    lFunc->push(L);
                    if (lEnv) {
                        GAnyLuaVM::setEnvironment(L, GAny(lEnv), lua_gettop(L));
                    }

                    /// Fill arguments
                    for (int32_t i = 0; i < argc; i++) {
                        makeGAnyToLuaObject(L, *args[i]);
                    }

                    /// Call
                    if (lua_pcall(L, argc, 1, 0) != LUA_OK) {
                        const char *err = lua_tostring(L, -1);
                        HANDLE_EXCEPTION(err);
                    }

                    /// Conversion return value
                    GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
                    lua_pop(L, 1);
                    return ret;
                }

                /// If the current thread is not the thread where the Lua function was created,
                /// call the function from the function bytecode.
                if (luaL_loadbuffer(
                            L, (const char *) funcRef->byteCode.data(),
                            (size_t) funcRef->byteCode.size(),
                            (const char *) fn.c_str()) != LUA_OK) {
                    const char *err = lua_tostring(L, -1);
                    HANDLE_EXCEPTION(err);
                }

                if (lEnv) {
                    GAnyLuaVM::setEnvironment(L, GAny(lEnv), lua_gettop(L));
                }
                /// Calling a function from bytecode requires filling in its up value
                GAnyLuaVM::storeUpValue(L, lua_gettop(L), upValues);

                /// Fill arguments
                for (int32_t i = 0; i < argc; i++) {
                    makeGAnyToLuaObject(L, *args[i]);
                }

                /// Call
                if (lua_pcall(L, argc, 1, 0) != LUA_OK) {
                    const char *err = lua_tostring(L, -1);
                    HANDLE_EXCEPTION(err);
                }

                /// Conversion return value
                GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
                lua_pop(L, 1);
                return ret;
            });

    return func;
}

constexpr double EPS = 1e-6;

GAny GAnyLuaVM::makeLuaObjectToGAny(lua_State *L, int idx)
{
    GX_ASSERT(idx > 0);
    int type = lua_type(L, idx);
    switch (type) {
        default:
        case LUA_TNONE:
            return GAny::undefined();
        case LUA_TNIL:
            return GAny::null();
        case LUA_TBOOLEAN:
            return (bool) lua_toboolean(L, idx);
        case LUA_TLIGHTUSERDATA:
            HANDLE_EXCEPTION("Unexpected data type: lightuserdata.");
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, idx);
            if (num - std::floor(num) < EPS) {
                return (int64_t) num;
            }
            return num;
        }
        case LUA_TSTRING:
            return std::string(lua_tostring(L, idx));
        case LUA_TTABLE: {
            return LuaTable(L, idx);
        }
        case LUA_TFUNCTION:
            return makeLuaFunctionToGAny(L, idx);
        case LUA_TUSERDATA:
            GAny *obj = glua_getcppobject(L, GAny, idx);
            return obj ? *obj : GAny::null();
    }
    return GAny::undefined();
}

int GAnyLuaVM::makeGAnyToLuaObject(lua_State *L, const GAny &value, bool useGAnyTable)
{
    if (value.isUndefined() || value.isNull()) {
        lua_pushnil(L);
        return 1;
    }
    if (value.isInt32() || value.isInt8() || value.isInt16()) {
        lua_pushinteger(L, value.toInt32());
        return 1;
    }
    if (value.isInt64() || value.is<long>() || value.is<unsigned long>()) {
        int64_t v = value.toInt64();
        if (v < INT32_MAX) {
            lua_pushinteger(L, (int) v);
        } else {
            lua_pushnumber(L, (double) v);
        }
        return 1;
    }
    if (value.isBoolean()) {
        lua_pushboolean(L, (int) value.toBool());
        return 1;
    }
    if (value.isFloat()) {
        lua_pushnumber(L, (double) value.as<float>());
        return 1;
    }
    if (value.isDouble()) {
        lua_pushnumber(L, value.as<double>());
        return 1;
    }
    if (value.isString()) {
        lua_pushstring(L, value.as<std::string>().c_str());
        return 1;
    }
    if (!useGAnyTable && value.isUserObject() && value.is<LuaTable>()) {
        value.as<LuaTable>().push(L);
        return 1;
    }

    pushGAny(L, value);
    return 1;
}

bool GAnyLuaVM::isGAnyLuaObj(lua_State *L, int idx)
{
    if (!lua_isuserdata(L, idx)) {
        return false;
    }
    lua_getmetatable(L, idx);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "_name");
        if (lua_isstring(L, -1)) {
            const char *name = lua_tostring(L, -1);
            bool is = strcmp(name, "GAny") == 0;
            lua_pop(L, 2);
            return is;
        }
        lua_pop(L, 2);
        return false;
    }

    lua_pop(L, 1);

    return false;
}

/// =======================================

#define toproto(L, i) getproto(s2v(L->top.p+(i)))

static const Proto *combine(lua_State *L)
{
    return toproto(L, -1);
}

static int luaDumpWriter(lua_State *, const void *p, size_t sz, void *ud)
{
    GByteArray &buff = *reinterpret_cast<GByteArray *>(ud);
    buff.write(p, sz);
    return 0;
}

GByteArray GAnyLuaVM::compileCode(const std::string &code, std::string sourcePath, bool strip)
{
    if (sourcePath.empty()) {
        GString scriptStr = code;
        // Ensure that UTF-8 characters are not truncated
        if (scriptStr.count() > 512) {
            scriptStr = scriptStr.left(512) + "...";
        }
        sourcePath = scriptStr.toStdString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    GByteArray buffer;
    buffer.write(code.data(), code.size());
    return compile(buffer, sourcePath, strip);
}

GByteArray GAnyLuaVM::compileFile(const std::string &filePath, bool strip)
{
    GByteArray buffer;
    if (sScriptReader) {
        buffer = sScriptReader(filePath);
    } else {
        GFile file(filePath);

        if (!file.exists()) {
            LogE("Run lua script error: file(%s) does not exist.", filePath.c_str());
            return GByteArray();
        }

        if (file.open(GFile::ReadOnly | GFile::Binary)) {
            buffer = file.read();
            file.close();
        } else {
            LogE("Open file failure.");
            return GByteArray();
        }
    }

    if (!buffer.isEmpty()) {
        return compile(buffer, "@" + filePath, strip);
    }

    return GByteArray();
}

GByteArray GAnyLuaVM::compile(const GByteArray &buffer, const std::string &sourcePath, bool strip)
{
    lua_State *L = mL;
    if (luaL_loadbuffer(L, (const char *) buffer.data(),
                        (size_t) buffer.size(),
                        (const char *) sourcePath.c_str()) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        LogE("%s", err);
        return GByteArray();
    }

    GByteArray buff;
    if (lua_dump(L, luaDumpWriter, (void *) &buff, strip) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        LogE("Dump lua code failure: %s", err);
        buff.clear();
    }

    lua_pop(L, lua_gettop(L));
    return buff;
}

GX_NS_END