#include "CodeBuffer.h"

#include <Windows.h>
#include <cassert>
#include <cstring>

namespace dynarec
{

    CodeBuffer::CodeBuffer(size_t size)
    {
        m_memory = static_cast<uint8_t*>(
            VirtualAlloc(
                nullptr,
                size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));

        assert(m_memory);

        m_current = m_memory;
        m_end = m_memory + size;
    }

    CodeBuffer::~CodeBuffer()
    {
        if (m_memory)
            VirtualFree(m_memory, 0, MEM_RELEASE);
    }

    uint8_t* CodeBuffer::Allocate(size_t bytes)
    {
        assert(m_current + bytes <= m_end);

        uint8_t* ptr = m_current;
        m_current += bytes;

        return ptr;
    }

    void CodeBuffer::Emit8(uint8_t value)
    {
        *m_current++ = value;
    }

    void CodeBuffer::Emit32(uint32_t value)
    {
        std::memcpy(m_current, &value, sizeof(value));
        m_current += sizeof(value);
    }

}