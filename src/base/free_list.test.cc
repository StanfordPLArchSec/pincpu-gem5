/*
 * Copyright (c) 2024 The Board of Trustees of the Leland Stanford
 * Junior University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtest/gtest.h>

#include "base/free_list.hh"

using namespace gem5;

TEST(FreeListTest, Simple)
{
    FreeList<int> l;
    EXPECT_EQ(l.size(), 0);
    EXPECT_EQ(l.ranges().size(), 0);
    l.insert(0, 16);
    EXPECT_EQ(l.size(), 16);
    EXPECT_EQ(l.ranges().size(), 1);
    int x;
    const bool ok = l.allocate(16, x);
    EXPECT_EQ(ok, true);
    EXPECT_EQ(l.size(), 0);
    EXPECT_EQ(l.ranges().size(), 0);
}

TEST(FreeListTest, FailedAllocation)
{
    FreeList<int> l(0, 16);
    EXPECT_EQ(l.size(), 16);
    int x;
    bool ok = l.allocate(17, x);
    ASSERT_EQ(ok, false);
    ASSERT_EQ(l.size(), 16);
}

TEST(FreeListTest, SucceededAllocation)
{
    FreeList<int> l(0, 16);
    int x;
    bool ok = l.allocate(8, x);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(l.size(), 8);
}

TEST(FreeListTest, MergeLeft)
{
    FreeList<int> l(0, 16);
    l.insert(16, 8);
    ASSERT_EQ(l.size(), 24);
    ASSERT_EQ(l.ranges().size(), 1);
}

TEST(FreeListTest, MergeRight)
{
    FreeList<int> l(8, 16);
    l.insert(0, 8);
    ASSERT_EQ(l.size(), 24);
    ASSERT_EQ(l.ranges().size(), 1);
}

TEST(FreeListTest, MergeBoth)
{
    FreeList<int> l;
    l.insert(0, 8);
    l.insert(16, 8);
    ASSERT_EQ(l.size(), 16);
    ASSERT_EQ(l.ranges().size(), 2);
    l.insert(8, 8);
    ASSERT_EQ(l.size(), 24);
    ASSERT_EQ(l.ranges().size(), 1);
}
