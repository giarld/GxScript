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

#include "lua_function.h"

#include "gany_lua_vm.h"

#include <gx/debug.h>


GX_NS_BEGIN

int luaDumpWriter(lua_State *, const void *p, size_t sz, void *ud)
{
    GByteArray &buff = *reinterpret_cast<GByteArray *>(ud);
    buff.write(p, sz);
    return 0;
}

LuaFunction::LuaFunction(lua_State *L, int idx)
        : mLuaVM(GAnyLuaVM::threadLocal())
{
    if (!lua_isfunction(L, idx)) {
        mLuaVM.reset();
        return;
    }
    lua_pushvalue(L, idx);
    mFunRef = luaL_ref(L, LUA_REGISTRYINDEX);
}

LuaFunction::~LuaFunction()
{
    auto vm = mLuaVM.lock();
    if (vm && mFunRef != 0) {
        luaL_unref(vm->getLuaState(), LUA_REGISTRYINDEX, mFunRef);
    }
}

bool LuaFunction::valid() const
{
    return !mLuaVM.expired() && mFunRef != 0;
}

bool LuaFunction::checkVM() const
{
    return mLuaVM.lock() == GAnyLuaVM::threadLocal();
}

void LuaFunction::push(lua_State *L) const
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, mFunRef);
    GX_ASSERT(lua_isfunction(L, -1));
}

GByteArray LuaFunction::dump() const
{
    auto vm = mLuaVM.lock();
    if (!vm) {
        return GByteArray();
    }
    lua_State *L = vm->getLuaState();
    push(L);
    GByteArray buff;
    bool ok = lua_dump(L, luaDumpWriter, (void *) &buff, false);
    GX_ASSERT_S(ok == LUA_OK, "Dump lua function failure.");
    lua_pop(L, 1);
    return buff;
}

GLuaFunctionRef::~GLuaFunctionRef()
{
    auto funcRef = func.lock();
    if (funcRef) {
        auto vm = funcRef->mLuaVM.lock();
        if (vm) {
            vm->removeLFunctionRef(funcRef);
        }
    }
}

GX_NS_END