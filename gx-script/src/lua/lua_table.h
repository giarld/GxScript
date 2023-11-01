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

#ifndef GX_SCRIPT_LUA_TABLE_H
#define GX_SCRIPT_LUA_TABLE_H

#include "gx/gany.h"
#include <gx/gbytearray.h>

#include <lua.hpp>


GX_NS_BEGIN

class LuaTableIterator;

/**
 * @class LuaTable
 * @brief The GAny packaging of Lua table data structure aims to provide a table that is out of the control of Lua garbage collector,
 *        allowing table data to be shared and passed between different threads
 */
class LuaTable
{
public:
    LuaTable();

    /**
     * @brief Build from Lua stack
     * @param L     Lua State
     * @param idx   The index of the table on the stack
     */
    explicit LuaTable(lua_State *L, int idx);

    LuaTable(const LuaTable &b);

    LuaTable(LuaTable &&b) noexcept;

    LuaTable &operator=(const LuaTable &b);

    LuaTable &operator=(LuaTable &&b) noexcept;

    static LuaTable fromGAnyObject(const GAny &obj);

public:
    GAny getItem(const GAny &key) const;

    void setItem(const GAny &key, const GAny &value);

    void delItem(const GAny &key);

    std::string toString() const;

    size_t length() const;

    /**
     * @brief Convert to Lua Table and push to Lua stack
     * @param L
     */
    void push(lua_State *L) const;

    /**
     * @brief Convert to GAnyObject, if the structure is an array,
     *        it will be converted to GAnyArray. If the key is of a non string type, it will be converted to a string
     * @return
     */
    GAny toObject() const;

    /**
     * @brief Get iterator
     * @return
     */
    std::unique_ptr<LuaTableIterator> iterator();

public:
    /**
     * @brief Serializing Write to GByteArray
     * @param ba
     * @param table
     */
    static void writeToByteArray(GByteArray &ba, const LuaTable &table);

    /**
     * @brief Read from GByteArray as LuaTable
     * @param ba
     * @return
     */
    static LuaTable readFromByteArray(GByteArray &ba);

private:
    void parse(lua_State *L, int idx);

    bool isArray() const;

    GAny toArray() const;

    static bool compareKey(const GAny &k1, const GAny &k2);

    static bool isNonStringType(const GAny &v);

private:
    std::vector<std::pair<GAny, GAny>> mTable;
};

/**
 * @class LuaTableIterator
 * @brief LuaTable iterator implemented according to the GAny iterator standard
 */
class LuaTableIterator
{
public:
    using TableType = std::vector<std::pair<GAny, GAny>>;
    using TableItem = std::pair<GAny, GAny>;

public:
    explicit LuaTableIterator(TableType &table)
            : mTable(table)
    {
        mIter = mTable.begin();
        mOpIter = mTable.end();
    }

    bool hasNext() const
    {
        return mIter != mTable.end();
    }

    TableItem next()
    {
        if (mIter == mTable.end()) {
            return std::make_pair(nullptr, nullptr);
        }
        TableItem v = *mIter;
        mOpIter = mIter;
        ++mIter;
        return v;
    }

    void remove()
    {
        if (mOpIter != mTable.end()) {
            mIter = mTable.erase(mOpIter);
            mOpIter = mTable.end();
        }
    }

    bool hasPrevious() const
    {
        return mIter != mTable.begin();
    }

    TableItem previous()
    {
        if (mIter == mTable.begin()) {
            return std::make_pair(nullptr, nullptr);
        }
        --mIter;
        mOpIter = mIter;
        TableItem v = *mIter;
        return v;
    }

    void toFront()
    {
        mIter = mTable.begin();
        mOpIter = mTable.end();
    }

    void toBack()
    {
        mIter = mTable.end();
        mOpIter = mTable.end();
    }

private:
    TableType &mTable;
    TableType::iterator mIter;
    TableType::iterator mOpIter;
};

GX_NS_END

#endif //GX_SCRIPT_LUA_TABLE_H
