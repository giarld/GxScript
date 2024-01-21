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

#include <gtest/gtest.h>

#include <gx/gany.h>


using namespace gx;

TEST(GxScriptTest, Script)
{
    const std::string script = R"(
LEnv.fromLua = 123;
return function(a, b)
    local array = LEnv.cppFunc(a, b);
    local retArray = GAny._array();
    for _, v in pairs(array) do
        retArray:_pushBack(v * 2);
    end
    return retArray;
end
)";

    auto tGAnyLuaVM = GAny::Import("L.GAnyLuaVM");


    auto lua = tGAnyLuaVM.call("threadLocal");

    lua.call("gcSetPause", 100);

    GAny env = GAny::object();

    env["cppFunc"] = [](int32_t begin, int32_t end) {
        std::vector<int32_t> array;
        for (int32_t i = begin; i <= end; i++) {
            array.push_back(i);
        }
        return array;
    };

    auto retFunc = lua.call("script", script, env);
    EXPECT_TRUE(retFunc.isFunction());
    EXPECT_EQ(env.getItem("fromLua"), 123);

    GAny ret = retFunc(1, 10);
    EXPECT_EQ(ret.toJsonString(), "[2,4,6,8,10,12,14,16,18,20]");
}
