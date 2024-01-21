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

#ifndef GX_SCRIPT_LUA_GANY_LUA_VM_H
#define GX_SCRIPT_LUA_GANY_LUA_VM_H

#include "gx/gobject.h"

#include <gx/gany.h>
#include <gx/gbytearray.h>
#include <gx/gmutex.h>

#include <lua.hpp>


#define glua_getcppobject(L, CLASS, i)  (lua_isuserdata(L, i) ? *(CLASS**)lua_touserdata(L, i) : nullptr)

GX_NS_BEGIN

class LuaFunction;

struct UpValueItem
{
    int upIdx{};
    GAny val;
    bool luaType{};
};

/**
 * @class GAnyLuaVM
 * @brief Enhanced by GAny, Lua virtual machine supports true multithreading. <br>
 * Features: <br>
 * 1. Start and stop the Lua virtual machine corresponding to the thread; <br>
 * 2. Execute Lua scripts from text, files, or binary streams; <br>
 * 3. Provide Lua virtual machine function setting interface; <br>
 * 4. Exception handling; <br>
 * 5. Directly load and use the GAny plugin; <br>
 * 6. Independent environmental variables and sharing of selectable environmental variables with maximum degrees of freedom; <br>
 * 7. Provide requireLs, which are more convenient and powerful than require; <br>
 * 8. You can directly call the types or returned functions created in Lua through GAny.
 */
class GAnyLuaVM
{
public:
    using ScriptReader = std::function<GByteArray(const std::string &path)>;

    using ExceptionHandler = std::function<void(const std::string &exception)>;

public:
    explicit GAnyLuaVM();

    ~GAnyLuaVM();

    /**
     * @brief Get the current thread's GAnyLuaVM
     * @return
     */
    static std::shared_ptr<GAnyLuaVM> threadLocal();

    lua_State *getLuaState() const;

    /**
     * @brief Actively shut down the virtual machine. After shutting down,
     *        the current virtual machine will become completely outdated.
     *        Do not end a non current thread virtual machine as it will cause unpredictable errors
     */
    void shutdown();

public:
    /**
     * @brief Load Lua script file from the set G Any plugin search path and execute it
     * @param name  Script file name (may not have a suffix)
     * @param env   Transferred environment variables
     * @return
     */
    GAny requireLs(const std::string &name, const GAny &env);

    /**
     * @brief Load and run Lua program from text
     * @param script        Lua script text
     * @param sourcePath    Code source path (file path or URI)
     * @param env           The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Returns the return value of the script
     */
    GAny script(const std::string &script, std::string sourcePath = "", const GAny &env = GAny::object());

    /**
     * @brief Loading and Running Lua Programs from Files
     * @param filePath  Lua script or bytecode file path
     * @param env       The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Returns the return value of the script
     */
    GAny scriptFile(const std::string &filePath, const GAny &env = GAny::object());

    /**
     * @brief Loading and Running Lua Programs from Bytes Arrays
     * @param buffer    Lua script or bytecode data stream Bytes Arrays
     * @param sourcePath    Code source path (file path or URI)
     * @param env       The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Returns the return value of the script
     */
    GAny scriptBuffer(const GByteArray &buffer, std::string sourcePath = "", const GAny &env = GAny::object());

    /**
     * @brief Trigger garbage collection for Lua virtual machine
     */
    void gc();

    /**
     * @brief GC step, Only incremental mode is valid
     * @param kb
     * @return
     */
    bool gcStep(int32_t kb);

    /**
     * @brief Set GC step rate, Only incremental mode is valid
     * @param mul
     * @return
     */
    int32_t gcSetStepMul(int32_t mul);

    /**
     * @brief Set GC step interval rate, Only incremental mode is valid
     * @param pause
     * @return
     */
    int32_t gcSetPause(int32_t pause);

    /**
     * @brief Stop garbage collector
     */
    void gcStop();

    /**
     * @brief Restart the garbage collector
     */
    void gcRestart();

    /**
     * @brief Returns whether the garbage collector is running
     * @return
     */
    bool gcIsRunning();

    /**
     * @brief Returns the amount of memory used by the current Lua virtual machine (in kb)
     * @return
     */
    int32_t gcGetCount();

    /**
     * @brief Switch garbage collector to generational mode
     */
    void gcModeGen();

    /**
     * @brief Switch the garbage collector to incremental mode
     */
    void gcModeInc();

    bool operator==(const GAnyLuaVM &rhs) const
    {
        return this->mL == rhs.mL;
    }

    /**
     * @brief Set the exception handler, after which all exception information will be returned from handlerFunc.
     *        If not set, you can handle the exception yourself
     * @param handler
     */
    static void setExceptionHandler(ExceptionHandler handler);

    /**
     * @brief   Set up a script reader. If a custom script reader is set up,
     *          the custom reader will be called when using "scriptFile" and "requireLs" to read the script file.
     * @param reader
     */
    static void setScriptReader(ScriptReader reader);

private:
    GAny loadScriptFromBuffer(const GByteArray &buffer, const std::string &sourcePath, const GAny &env);

    void addLFunctionRef(const std::shared_ptr<LuaFunction> &ref);

    void removeLFunctionRef(const std::shared_ptr<LuaFunction> &ref);

public:    /// Tools
    /**
     * @brief Place a GAny object on the specified Lua stack
     * @param L
     * @param v
     */
    static void pushGAny(lua_State *L, const GAny &v);

    /**
     * @brief Find the up value of the specified name from the specified function on the specified Lua stack
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static int findUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Obtain the up value of the specified Lua function, return true if successful,
     *        and place the up value at the top of the stack
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static bool getUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Set the top finger of the stack to the specified upper value
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static bool setUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Set the specified env(LEnv) to the env(LEnv) of the specified Lua function
     * @param L
     * @param env
     * @param funcIdx
     */
    static void setEnvironment(lua_State *L, const GAny &env, int funcIdx);

    /**
     * @brief Get env(LEnv) of the specified Lua function
     * @param L
     * @param funcIdx
     * @return
     */
    static GAny getEnvironment(lua_State *L, int funcIdx);

    /**
     * @brief Dump all up values except for global variables
     * @param L
     * @param funcIdx
     * @return
     */
    static std::vector<UpValueItem> dumpUpValue(lua_State *L, int funcIdx);

    /**
     * @brief Fill the function's up value table with the up value held
     * @param L
     * @param funcIdx
     * @param upValues
     */
    static void storeUpValue(lua_State *L, int funcIdx, const std::vector<UpValueItem> &upValues);

    /**
     * @brief Make Lua function as GAny function
     * @param L
     * @param idx
     * @return
     */
    static GAny makeLuaFunctionToGAny(lua_State *L, int idx);

    /**
     * @brief Make Lua object as GAny object
     * @param L
     * @param idx
     * @return
     */
    static GAny makeLuaObjectToGAny(lua_State *L, int idx);

    /**
     * @brief Make GAny object as Lua object
     * @param L
     * @param v
     * @param useGAnyTable
     * @return
     */
    static int makeGAnyToLuaObject(lua_State *L, const GAny &v, bool useGAnyTable = false);

    /**
     * @brief Determine whether the corresponding Lua object is a GAny object
     * @param L
     * @param idx
     * @return
     */
    static bool isGAnyLuaObj(lua_State *L, int idx);

public:
    /**
     * @brief Compile from code to generate bytecode
     * @param code          Lua source code
     * @param sourcePath    Code source path (file path or URI)
     * @param strip         Strip debug information
     * @return  bytecode
     */
    GByteArray compileCode(const std::string &code, std::string sourcePath, bool strip);

    /**
     * @brief Load code from source code file and generate bytecode
     * @param filePath  Path to Lua source code file
     * @param strip     Strip debug information
     * @return  bytecode
     */
    GByteArray compileFile(const std::string &filePath, bool strip);

private:
    GByteArray compile(const GByteArray &buffer, const std::string &sourcePath, bool strip);

private:
    friend class GLuaFunctionRef;

    lua_State *mL = nullptr;

    std::vector<std::shared_ptr<LuaFunction>> mLFuncs;
    GMutex mFuncsLock;

    static ScriptReader sScriptReader;
    static ExceptionHandler sExceptionHandler;
};

GX_NS_END

#endif //GX_SCRIPT_LUA_GANY_LUA_VM_H
