#pragma once

#include <core/Types.h>

#ifdef EXPERIMENTAL_DYNAREC

#include <dynarec/Dynarec.h>
#include <dynarec/CodeBuffer.h>
#include <psx/R3000.h>

namespace dynarec
{
    class Emitter
    {
    public:
        Emitter() = default;
        explicit Emitter(R3000& r3000, CodeBuffer& buffer)
            : m_cpu(r3000), m_buffer(buffer)
        {}

        void EmitBlock(const CompiledBlock& block);

    private:
        bool EmitInstruction(const DecodedInstruction& ins);
        bool EmitFallback(const DecodedInstruction& ins);

        void EmitLui(const DecodedInstruction& ins);
        void EmitOri(const DecodedInstruction& ins);
        void EmitAddiu(const DecodedInstruction& ins);

    private:
        R3000& m_cpu;
        CodeBuffer& m_buffer;
        // X64Writer* m_writer = nullptr;
    };
}

#endif // EXPERIMENTAL_DYNAREC