# GxScript
[中文](README-zh.md)

Based on GxAny, a Lua script engine that truly supports multithreading.

## Features
- Bind C++ types through GAny and provide them for Lua to use.
- Provide functionality written in the target language through GAny binding for Lua to use.
- Create types in Lua through GAny and provide them for use in C++or other languages.
- Using True Multi threaded Asynchronous Programming in Lua.
- Make the code written by Lua a GAny plugin.

## 快速开始
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

## Third party libraries used
- [lua](https://www.lua.org)
- [gtest](https://github.com/google/googletest)

## 许可
`GxScript` is licensed under the [MIT License](LICENSE.txt).
