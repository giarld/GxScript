# GxScript
[English](README.md)

基于GxAny，一个支持真多线程的Lua脚本引擎。

## 功能
- 通过 GAny 绑定 C++ 类型, 提供给 Lua 使用.
- 通过 GAny 绑定目标语言编写的功能, 提供给 Lua 使用.
- 通过 GAny 将 Lua 中创建类型, 提供给 C++ 或其他语言使用.
- 在 Lua 中使用真多线程异步编程.
- 让 Lua 编写的代码成为 GAny 插件.

## Quick Start
```cpp
#include <cstdlib>
#include <gx/gany_core.h>
#include <gx/reg_gx.h>
#include <gx/reg_script.h>
#include <iostream>

using namespace gx;

int main(int argc, char *argv[])
{
    initGAnyCore();

    GANY_IMPORT_MODULE(Gx);
    GANY_IMPORT_MODULE(GxScript);

// Or
//    loadPlugin("gx/Gx");
//    loadPlugin("gx-script/GxScript");

    auto GAnyLua = GEnv["L.GAnyLuaVM"];
    auto lua = GAnyLua.call("threadLocal");

    GAnyLua.call("setExceptionHandler", [](const GAnyException &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    });

    std::string script = R"(
local GThread = LEnv.GEnv.Gx.GThread;
local GMutex = LEnv.GEnv.Gx.GMutex;

-- The reading and writing of GAnyObject itself is thread safe, and the following lockers only demonstrate its function
local vObj = GAny._object();
vObj.v = 0;

local locker = GMutex:new();

local thread = GThread:new(function()
    for i = 1, 100 do
        locker:lock(function()
            vObj.v = vObj.v + 1;
        end);
    end
end);

for i = 1, 100 do
    locker:lock();
    vObj.v = vObj.v + 1;
    locker:unlock();
end

thread:join();
return vObj.v;
)";

    GAny env = GAny::object();
    env["GEnv"] = GEnv;

    GAny ret = lua.call("script", script, env); // 200

    std::cout << ret << std::endl;

    lua.call("gc");

    return EXIT_SUCCESS;
}
```

## 使用的第三方库
- [lua](https://www.lua.org)
- [gtest](https://github.com/google/googletest)

## 许可
`GxScript` 根据 [MIT许可证](LICENSE.txt) 授权。
