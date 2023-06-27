// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "assert.h"

namespace cauldron
{

    /**
     * @class Ring
     *
     * This is the typical ring buffer, it is used by resources that will be reused. 
     * For example the constant buffer pool.
     *
     * @ingroup CauldronMisc
     */
    class Ring
    {
    public:

        /**
         * @brief   Ring buffer creation. Pass in the maximum supported size.
         */
        void Create(uint32_t totalSize)
        {
            m_Head = 0;
            m_AllocatedSize = 0;
            m_TotalSize = totalSize;
        }

        /**
         * @brief   Gets the ring buffer's currently allocated size.
         */
        uint32_t GetSize() { return m_AllocatedSize; }

        /**
         * @brief   Gets the ring buffer's head location.
         */
        uint32_t GetHead() { return m_Head; }

        /**
         * @brief   Gets the ring buffer's tail location.
         */
        uint32_t GetTail() { return (m_Head + m_AllocatedSize) % m_TotalSize; }

        /**
         * @brief   Helper to avoid allocating chunks that wouldn't fit contiguously in the ring.
         */
        uint32_t PaddingToAvoidCrossOver(uint32_t size)
        {
            int32_t tail = GetTail();
            if ((tail + size) > m_TotalSize)
                return (m_TotalSize - tail);
            else
                return 0;
        }

        /**
         * @brief   Allocates a new entry at the tail end of the ring buffer. Asserts if we exceed the ring buffer's capacity.
         */
        bool Alloc(uint32_t size, uint32_t* pOut)
        {
            CauldronAssert(ASSERT_CRITICAL, m_AllocatedSize + size <= m_TotalSize, L"Ring buffer capacity exceeded. Cannot allocate. Please grow the ring buffer size.");
            if (m_AllocatedSize + size <= m_TotalSize)
            {
                if (pOut)
                    *pOut = GetTail();

                m_AllocatedSize += size;
                return true;
            }

            return false;
        }

        /**
         * @brief   Frees memory from the head of the ring buffer.
         */
        bool Free(uint32_t size)
        {
            if (m_AllocatedSize >= size)
            {
                m_Head = (m_Head + size) % m_TotalSize;
                m_AllocatedSize -= size;
                return true;
            }

            return false;
        }

    private:
        uint32_t m_Head;
        uint32_t m_AllocatedSize;
        uint32_t m_TotalSize;
    };

    //////////////////////////////////////////////////////////////////////////
    //  

    /**
     * @class RingWithTabs
     *
     * This class can be thought as ring buffer inside a ring buffer. The outer ring is for
     * the frames and the internal one is for the resources that were allocated for that frame.
     * The size of the outer ring is typically the number of back buffers.
     *
     * When the outer ring is full, for the next allocation it automatically frees the entries
     * of the oldest frame and makes those entries available for the next frame. This happens
     * when you call 'BeginFrame()'
     *
     * @ingroup CauldronMisc
     */
    class RingWithTabs
    {
    public:

        /**
         * @brief   Ring buffer creation. Takes the number of back buffers to support and 
         *          the amount of memory to support per frame.
         */
        void Create(uint32_t numberOfBackBuffers, uint32_t memTotalSize)
        {
            m_BackBufferIndex = 0;
            m_NumberOfBackBuffers = numberOfBackBuffers;

            // Init mem per frame tracker
            m_MemAllocatedInFrame = 0;
            for (int i = 0; i < 4; i++)
                m_AllocatedMemPerBackBuffer[i] = 0;

            m_Mem.Create(memTotalSize);
        }

        /**
         * @brief   Destroys the ring buffer.
         */
        void Destroy()
        {
            m_Mem.Free(m_Mem.GetSize());
        }

        /**
         * @brief   Allocates a new entry at the tail end of the ring buffer for the current frame. 
         *          Asserts if we exceed the ring buffer's capacity.
         */
        bool Alloc(uint32_t size, uint32_t* pOut)
        {
            uint32_t padding = m_Mem.PaddingToAvoidCrossOver(size);
            if (padding > 0)
            {
                m_MemAllocatedInFrame += padding;

                if (m_Mem.Alloc(padding, NULL) == false) // Alloc chunk to avoid crossover, ignore offset        
                {
                    return false;  // No mem, cannot allocate padding
                }
            }

            if (m_Mem.Alloc(size, pOut) == true)
            {
                m_MemAllocatedInFrame += size;
                return true;
            }

            return false;
        }

        /**
         * @brief   Sets up the ring buffer for current frame allocations.
         */
        void BeginFrame()
        {
            m_AllocatedMemPerBackBuffer[m_BackBufferIndex] = m_MemAllocatedInFrame;
            m_MemAllocatedInFrame = 0;

            m_BackBufferIndex = (m_BackBufferIndex + 1) % m_NumberOfBackBuffers;

            // Free all the entries for the oldest buffer in one go
            uint32_t memToFree = m_AllocatedMemPerBackBuffer[m_BackBufferIndex];
            m_Mem.Free(memToFree);
        }

    private:

        // Internal ring buffer
        Ring m_Mem;

        // This is the external ring buffer (Could have reused the Ring class though)
        uint32_t m_BackBufferIndex;
        uint32_t m_NumberOfBackBuffers;

        uint32_t m_MemAllocatedInFrame;
        uint32_t m_AllocatedMemPerBackBuffer[4];
    };

} // namespace cauldron
