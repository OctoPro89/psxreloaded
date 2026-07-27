#pragma once

#include <core/Types.h>

#ifdef EXPERIMENTAL_DYNAREC

#include <dynarec/Dynarec.h>
#include <psx/R3000.h>

namespace dynarec
{
    typedef struct
    {
        u32 r[32];
        u32 pc;
    } CpuState;

    class Emitter
    {
    public:
        Emitter() = default;
        explicit Emitter(R3000& r3000)
            : m_cpu(r3000)
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
        // X64Writer* m_writer = nullptr;
    };
}

#endif // EXPERIMENTAL_DYNAREC