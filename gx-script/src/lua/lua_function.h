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

#ifndef GX_SCRIPT_LUA_FUNCTION_H
#define GX_SCRIPT_LUA_FUNCTION_H

#include <gx/gobject.h>

#include <gx/gbytearray.h>

#include <memory>

#include <lua.hpp>


GX_NS_BEGIN

class GAnyLuaVM;

/**
 * @brief Wrapping Lua functions to assist in persisting and multithreading Lua functions
 */
class LuaFunction
{
public:
    /**
     * @brief Constructor
     * @param L     Current Lua State
     * @param idx   Index of functions that need to be persisted in Lua stack
     */
    explicit LuaFunction(lua_State *L, int idx);

    ~LuaFunction();

    LuaFunction(const LuaFunction &) = delete;

    LuaFunction(LuaFunction &&b) noexcept = delete;

    LuaFunction &operator=(const LuaFunction &) = delete;

    LuaFunction &operator=(LuaFunction &&b) noexcept = delete;

public:
    /**
     * @brief Determine whether the current Lua function is valid
     *        (whether it corresponds to the current LuaState and whether the function exists)
     * @return
     */
    bool valid() const;

    /**
     * @brief Determine whether the Lua vm to which the current function belongs is the Lua vm of the current thread
     * @return
     */
    bool checkVM() const;

    /**
     * @brief Push the current Lua function onto the stack
     * @param L
     */
    void push(lua_State *L) const;

    /**
     * @brief Dump current lua function into bytecode
     * @return
     */
    GByteArray dump() const;

private:
    friend class GLuaFunctionRef;

    std::weak_ptr<GAnyLuaVM> mLuaVM;
    int mFunRef = 0;
};

/**
 * @brief Reference to Lua functions held and managed by GAny for lifecycle
 */
struct GLuaFunctionRef
{
    GByteArray byteCode;
    std::weak_ptr<LuaFunction> func;

    ~GLuaFunctionRef();
};

GX_NS_END

#endif //GX_SCRIPT_LUA_FUNCTION_H
