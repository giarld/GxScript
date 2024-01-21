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

#include "gx/reg_script.h"

#include "gx/gany.h"
#include <gx/gbytearray.h>
#include <gx/gfile.h>
#include <gx/debug.h>

#include "lua/lua_table.h"
#include "lua/gany_lua_vm.h"


using namespace gx;

REGISTER_GANY_MODULE(GxScript)
{
    Class<LuaTable>("L", "LuaTable", "lua table compatible types.")
            .construct<>()
            .construct<const LuaTable &>()
            .func(MetaFunction::ToString, &LuaTable::toString)
            .func(MetaFunction::Length, &LuaTable::length)
            .func(MetaFunction::SetItem, &LuaTable::setItem)
            .func(MetaFunction::GetItem, &LuaTable::getItem)
            .func(MetaFunction::DelItem, &LuaTable::delItem)
            .func(MetaFunction::ToObject, &LuaTable::toObject)
            .func("iterator", &LuaTable::iterator, "Get iterator.");

    // GAny LuaTable iterator, Special provision of reverse iteration function
    GAnyClass::Class < LuaTableIterator > ()
            ->setName("LuaTableIterator")
            .setDoc("Lua table iterator.")
            .func("hasNext", &LuaTableIterator::hasNext)
            .func("next", &LuaTableIterator::next)
            .func("remove", &LuaTableIterator::remove)
            .func("hasPrevious", &LuaTableIterator::hasPrevious)
            .func("previous", &LuaTableIterator::previous)
            .func("toFront", &LuaTableIterator::toFront)
            .func("toBack", &LuaTableIterator::toBack);

    // Add LuaTable serialization and deserialization capabilities to GByteArray
    GAnyClass::Class < GByteArray > ()
            ->func("writeTable", [](GByteArray &self, const LuaTable &value) {
                GByteArray buf;
                LuaTable::writeToByteArray(buf, value);
                self.write(buf);
            })
            .func("readTable", [](GByteArray &self) {
                GByteArray buf;
                self.read(buf);
                return LuaTable::readFromByteArray(buf);
            });

    Class<GAnyLuaVM>("L", "GAnyLuaVM", "GAny lua vm.")
            .staticFunc("threadLocal", &GAnyLuaVM::threadLocal)
            .func("shutdown", &GAnyLuaVM::shutdown,
                  "Actively shut down the virtual machine. \n"
                  "After shutting down, the current virtual machine will become completely outdated. \n"
                  "Do not end a non current thread virtual machine as it will cause unpredictable errors.")
            .func("script", [](GAnyLuaVM &self, const std::string &script) {
                return self.script(script);
            }, "Load and run Lua program from text. \n"
               "arg1: Lua script text; \n"
               "return: Returns the return value of the script.")
            .func("script", [](GAnyLuaVM &self, const std::string &script, const GAny &env) {
                return self.script(script, "", env);
            }, "Load and run Lua program from text. \n"
               "arg1: Lua script text; \n"
               "arg2: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
               "return: Returns the return value of the script.")
            .func("script",
                  [](GAnyLuaVM &self, const std::string &script, const std::string &sourcePath, const GAny &env) {
                      return self.script(script, sourcePath, env);
                  }, "Load and run Lua program from text. \n"
                     "arg1: Lua script text; \n"
                     "arg2: Code source path (file path or URI); \n"
                     "arg3: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
                     "return: Returns the return value of the script.")
            .func("scriptFile", [](GAnyLuaVM &self, const std::string &filePath) {
                return self.scriptFile(filePath);
            }, "Loading and Running Lua Programs from Files. \n"
               "arg1: Lua script or bytecode file path; \n"
               "return: Returns the return value of the script.")
            .func("scriptFile", [](GAnyLuaVM &self, const std::string &filePath, const GAny &env) {
                return self.scriptFile(filePath, env);
            }, "Loading and Running Lua Programs from Files. \n"
               "arg1: Lua script or bytecode file path; \n"
               "arg2: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
               "return: Returns the return value of the script.")
            .func("scriptBuffer", [](GAnyLuaVM &self, const GByteArray &buffer) {
                return self.scriptBuffer(buffer);
            }, "Loading and Running Lua Programs from Bytes Arrays. \n"
               "arg1: Lua script or bytecode data stream Bytes Arrays; \n"
               "return: Returns the return value of the script.")
            .func("scriptBuffer", [](GAnyLuaVM &self, const GByteArray &buffer, const GAny &env) {
                return self.scriptBuffer(buffer, "", env);
            }, "Loading and Running Lua Programs from Bytes Arrays. \n"
               "arg1: Lua script or bytecode data stream Bytes Arrays; \n"
               "arg2: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
               "return: Returns the return value of the script.")
            .func("scriptBuffer",
                  [](GAnyLuaVM &self, const GByteArray &buffer, const std::string &sourcePath, const GAny &env) {
                      return self.scriptBuffer(buffer, sourcePath, env);
                  }, "Loading and Running Lua Programs from Bytes Arrays. \n"
                     "arg1: Lua script or bytecode data stream Bytes Arrays; \n"
                     "arg2: Code source path (file path or URI); \n"
                     "arg3: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
                     "return: Returns the return value of the script.")
            .func("gc", &GAnyLuaVM::gc, "Trigger garbage collection for Lua virtual machine.")
            .func("gcStep", &GAnyLuaVM::gcStep, "GC step, Only incremental mode is valid.")
            .func("gcSetStepMul", &GAnyLuaVM::gcSetStepMul, "Set GC step rate, Only incremental mode is valid.")
            .func("gcSetPause", &GAnyLuaVM::gcSetPause, "Set GC step interval rate, Only incremental mode is valid.")
            .func("gcStop", &GAnyLuaVM::gcStop, "Stop garbage collector.")
            .func("gcRestart", &GAnyLuaVM::gcRestart, "Restart the garbage collector.")
            .func("gcIsRunning", &GAnyLuaVM::gcIsRunning, "Returns whether the garbage collector is running.")
            .func("gcGetCount", &GAnyLuaVM::gcGetCount,
                  "Returns the amount of memory used by the current Lua virtual machine (in kb).")
            .func("gcModeGen", &GAnyLuaVM::gcModeGen, "Switch garbage collector to generational mode.")
            .func("gcModeInc", &GAnyLuaVM::gcModeInc, "Switch the garbage collector to incremental mode.")
            .staticFunc("setExceptionHandler",
                        [](const GAny &handler) {
                            if (handler.isFunction()) {
                                GAnyLuaVM::setExceptionHandler([handler](const std::string &e) {
                                    try {
                                        handler(e);
                                    } catch (GAnyException &) {
                                    }
                                });
                            } else {
                                GAnyLuaVM::setExceptionHandler(nullptr);
                            }
                        },
                        "Set the exception handler, after which all exception information will be returned from handlerFunc. \n"
                        "If not set, you can handle the exception yourself.")
            .staticFunc("setScriptReader",
                        [](const GAny &reader) {
                            if (reader.isFunction()) {
                                GAnyLuaVM::setScriptReader([reader](const std::string &path) {
                                    try {
                                        return reader(path).as<GByteArray>();
                                    } catch (GAnyException &) {
                                    }
                                    return GByteArray();
                                });
                            } else {
                                GAnyLuaVM::setScriptReader(nullptr);
                            }
                        },
                        "Set up a script reader. If a custom script reader is set up, "
                        "the custom reader will be called when using \"scriptFile\" and \"requireLs\" to read the script file.")
            .func("compileCode", &GAnyLuaVM::compileCode,
                  "Compile from code to generate bytecode.\n"
                  "arg1: Lua source code;\n"
                  "arg2: Code source path (file path or URI);\n"
                  "arg3: Strip debug information;\n"
                  "return: bytecode.")
            .func("compileFile", &GAnyLuaVM::compileFile,
                  "Load code from source code file and generate bytecode.\n"
                  "arg1: Path to Lua source code file;\n"
                  "arg2: Strip debug information;\n"
                  "return: bytecode.")
            .func(MetaFunction::EqualTo, [](GAnyLuaVM &self, const GAnyLuaVM &rhs) {
                return self == rhs;
            });

    // Set Lua plugin loader
    GAny::Import("setPluginLoaders")("Ls", [](const std::string &searchPath, const std::string &pluginName) {
        GFile dir(searchPath);

        GFile scriptFile;
        do {
            scriptFile = GFile(dir, pluginName + ".lua");
            if (scriptFile.exists() && scriptFile.isFile()) {
                break;
            }
            scriptFile = GFile(dir, pluginName + ".lsc");
        } while (false);
        if (!scriptFile.exists() || !scriptFile.isFile()) {
            return false;
        }

        auto lua = GAnyLuaVM::threadLocal();

        GAny env = GAny::object();
        try {
            lua->scriptFile(scriptFile.absoluteFilePath(), env);
            return true;
        } catch (std::exception &e) {
            LogE("Load lua plugin error: %s", e.what());
        }

        return false;
    });
}
