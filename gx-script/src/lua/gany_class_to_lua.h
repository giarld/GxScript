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

#ifndef GX_SCRIPT_GANY_CLASS_TO_LUA_H
#define GX_SCRIPT_GANY_CLASS_TO_LUA_H

#include "gx/gany.h"

#include <lua.hpp>


GX_NS_BEGIN

/**
 * @class GAnyClassToLua
 * @brief Bind GAnyClass to Lua
 */
class GAnyClassToLua
{
public:
    static void toLua(lua_State *L);

private:
    static void pushGAnyClass(lua_State *L, const GAny &v);

private:
    static int noneNewIndex(lua_State *L);

    static int regClass(lua_State *L);

    static int regGC(lua_State *L);

    static int regInherit(lua_State *L);

    static int regRegisterToEnv(lua_State *L);

    static int regFunc(lua_State *L);

    static int regStaticFunc(lua_State *L);

    static int regDefEnum(lua_State *L);

    static int regProperty(lua_State *L);

    static int regNew(lua_State *L);
};

GX_NS_END

#endif //GX_SCRIPT_GANY_CLASS_TO_LUA_H
