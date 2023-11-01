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


#define HANDLE_EXCEPTION(e) \
    do {    \
        if (sExceptionHandler) {    \
            sExceptionHandler(e);   \
            return GAny::undefined();    \
        } else {    \
            throw e;  \
        }   \
    } while(false)


GX_NS_BEGIN

GAny GAnyLuaVM::sExceptionHandler = GAny::undefined();

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

GAny GAnyLuaVM::script(const std::string &script, const GAny &env)
{
    return loadScriptFunc(script, "text", env);
}

GAny GAnyLuaVM::scriptFile(const std::string &filePath, const GAny &env)
{
    return loadScriptFunc(filePath, "file", env);
}

GAny GAnyLuaVM::scriptBuffer(const GByteArray &buffer, const GAny &env)
{
    return loadScriptFunc(buffer, "buffer", env);
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

void GAnyLuaVM::setExceptionHandler(const GAny &handlerFunc)
{
    sExceptionHandler = handlerFunc;
}


GAny GAnyLuaVM::requireLs(const std::string &path, const std::string &name, const GAny &env)
{
    GFile file = [&]() {
        std::string tPath = path.empty() ? "./" : path;
        GFile dir(tPath);
        if (!dir.isDirectory()) {
            return GFile();
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
        return GFile();
    }();
    if (!file.exists()) {
        LogE("requireLs: %s is not found", name.c_str());
        return GAny::undefined();
    }
    return scriptFile(file.absoluteFilePath(), env);
}

GAny GAnyLuaVM::requireLs(const std::string &name, const GAny &env)
{
    GFile file = [&]() {
        auto searchPaths = GEnv.getItem("getPluginSearchPaths")().castAs<std::vector<std::string>>();

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
        LogE("requireLs: %s is not found", name.c_str());
        return GAny::undefined();
    }
    return scriptFile(file.absoluteFilePath(), env);
}

GAny GAnyLuaVM::loadScriptFunc(const GAny &source, const std::string &type, const GAny &env)
{
    if (!env.isObject()) {
        HANDLE_EXCEPTION(GAnyException("GAnyLua environment must be an GAnyObject."));
    }

    using LoadFunc = std::function<GAny(lua_State *L, const GAny &source, const GAny &env)>;

    auto loadFromBuffer = [this](lua_State *L,
                                 const GByteArray &buffer,
                                 const GAny &env) -> GAny {
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
                            (const char *) GByteArray::md5Sum(data).toHexString().c_str()) != LUA_OK) {
                    HANDLE_EXCEPTION(GAnyException("GAnyLua load buffer failure"));
                }

                GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

                if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                    const char *err = lua_tostring(L, -1);
                    HANDLE_EXCEPTION(GAnyException(err));
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
                    (const char *) GByteArray::md5Sum(buffer).toHexString().c_str()) != LUA_OK) {
            HANDLE_EXCEPTION(GAnyException("GAnyLua load buffer failure"));
        }

        GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            HANDLE_EXCEPTION(GAnyException(err));
        }
        GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
        lua_pop(L, 1);
        return ret;
    };

    LoadFunc loadFunc;
    if (type == "file") {
        GFile file(source.toString());

        if (!file.exists()) {
            std::stringstream ess;
            ess << "GAnyLua, run lua script error: file(" << source.toString() << ") " << "does not exist";
            HANDLE_EXCEPTION(GAnyException(ess.str()));
        }

        if (file.open(GFile::ReadOnly | GFile::Binary)) {
            if (file.fileSize() > 4) {
                char head[4];
                file.read(head, 4);
                if (head[0] == (char) 0xff && head[1] == 'l' && head[2] == 's' && head[3] == (char) 0xee) {
                    loadFunc = [loadFromBuffer](lua_State *L, const GAny &source, const GAny &env) {
                        GFile file(source.toString());
                        if (file.open(GFile::ReadOnly | GFile::Binary)) {
                            GByteArray buffer = file.read();
                            file.close();

                            return loadFromBuffer(L, buffer, env);
                        } else {
                            HANDLE_EXCEPTION(GAnyException("GAnyLua, open file failure"));
                        }
                        return GAny::undefined();
                    };
                }
            }
            file.close();
        }
        if (!loadFunc) {
            loadFunc = [](lua_State *L, const GAny &source, const GAny &env) {
                if (luaL_loadfile(L, source.toString().c_str()) != LUA_OK) {
                    std::stringstream ess;
                    ess << "GAnyLua, load file failure: " << source.toString();
                    HANDLE_EXCEPTION(GAnyException(ess.str()));
                }

                GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

                if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                    const char *err = lua_tostring(L, -1);
                    HANDLE_EXCEPTION(GAnyException(err));
                }
                GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
                lua_pop(L, 1);
                return ret;
            };
        }
    } else if (type == "text") {
        loadFunc = [](lua_State *L, const GAny &source, const GAny &env) {
            GString script = source.toString();
            GString name;
            if (script.count() <= 512) {
                name = script;
            } else {
                name = script.left(512) + "...";
            }
            if (luaL_loadbuffer(L, (const char *) script.data(),
                                (size_t) script.count(),
                                name.c_str()) != LUA_OK) {
                std::stringstream ess;
                ess << "GAnyLua, load text failure: " << name;
                HANDLE_EXCEPTION(GAnyException(ess.str()));
            }

            GAnyLuaVM::setEnvironment(L, env, lua_gettop(L));

            if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                HANDLE_EXCEPTION(GAnyException(err));
            }
            GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
            lua_pop(L, 1);
            return ret;
        };
    } else if (type == "buffer") {
        loadFunc = [loadFromBuffer](lua_State *L, const GAny &source, const GAny &env) {
            return loadFromBuffer(L, source.as<GByteArray>(), env);
        };
    }

    if (!loadFunc) {
        HANDLE_EXCEPTION(GAnyException(std::string("Run lua script error: Unknown type: ") + type));
    }

    return loadFunc(mL, source, env);
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
                    HANDLE_EXCEPTION(GAnyException("Failed to get thread local lua vm!"));
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
                        HANDLE_EXCEPTION(GAnyException(err));
                    }

                    /// Conversion return value
                    GAny ret =  makeLuaObjectToGAny(L, lua_gettop(L));
                    lua_pop(L, 1);
                    return ret;
                }

                /// If the current thread is not the thread where the Lua function was created,
                /// call the function from the function bytecode.
                if (luaL_loadbuffer(
                            L, (const char *) funcRef->byteCode.data(),
                            (size_t) funcRef->byteCode.size(),
                            (const char *) GByteArray::md5Sum(funcRef->byteCode).toHexString().c_str()) != LUA_OK) {
                    std::stringstream ess;
                    ess << "Load function: \"" << fn << "\" bytecode error.";
                    HANDLE_EXCEPTION(GAnyException(ess.str()));
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
                    HANDLE_EXCEPTION(GAnyException(err));
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
            HANDLE_EXCEPTION(GAnyException("GAnyLuaVM, Unexpected data type: lightuserdata"));
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

GX_NS_END