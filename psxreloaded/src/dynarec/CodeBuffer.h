#pragma once

#include <cstdint>
#include <cstddef>

namespace dynarec
{
    class CodeBuffer
    {
    public:
        explicit CodeBuffer(size_t size = 1024 * 1024);
        ~CodeBuffer();

        uint8_t* Allocate(size_t bytes);

        void Emit8(uint8_t value);
        void Emit32(uint32_t value);

        uint8_t* GetCurrent() const
        {
            return m_current;
        }

    private:
        uint8_t* m_memory = nullptr;
        uint8_t* m_current = nullptr;
        uint8_t* m_end = nullptr;
    };
}