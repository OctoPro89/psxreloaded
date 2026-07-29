#include <dynarec/x64/Emitter.h>

#ifdef EXPERIMENTAL_DYNAREC

#include <Host.h>
#include <psx/Bus.h>
#include <core/hp_assert.h>
#include <core/Log.h>

namespace dynarec
{
    //extern "C" void ExecuteFallback(CpuState* cpu, u32 raw, u32 pc);

    void dynarec::Emitter::EmitBlock(const CompiledBlock& block)
    {
        /*
        LOG_INFO("Emit block start=%08X first=%08X count=%zu\n",
            block.startPC,
            block.instructions.front().pc,
            block.instructions.size());
        for (auto& ins : block.instructions)
        {
            LOG_INFO("%08X  %08X [%s]\n", ins.pc, ins.raw, Compiler::DecodeOpcode(ins.raw)->instruction);
        }
        */

        for (const auto& ins : block.instructions)
        {
            if (!EmitInstruction(ins))
            {
                return;
            }
        }
    }

    void dynarec::Emitter::EmitLui(const DecodedInstruction& ins)
    {
        if (ins.rt == 0)
            return; // register zero stays zero

        /*
        u32 value = (u32)(u16)ins.imm << 16;

        // Example if you were writing directly into memory-backed CPU state:
        m_cpu.m_r[ins.rt] = value;
        */

        uint32_t offset = offsetof(R3000, m_r) + ins.rt * 4;

        uint32_t value = (uint16_t)ins.imm << 16;

        m_buffer.Emit8(0xC7);
        m_buffer.Emit8(0x81);

        m_buffer.Emit32(offset);
        m_buffer.Emit32(value);

        m_buffer.Emit8(0xC3); // ret

        using BlockFn = void(*)(R3000*);

        auto fn = reinterpret_cast<BlockFn>(m_buffer.GetCurrent());
        fn(&m_cpu);
    }

    void dynarec::Emitter::EmitOri(const DecodedInstruction& ins)
    {
        if (ins.rt == 0)
            return;

        u32 lhs = m_cpu.m_r[ins.rs];
        u32 rhs = (u16)ins.imm;
        u32 result = lhs | rhs;

        m_cpu.m_r[ins.rt] = result;

        // x64 version:
        // mov eax, [cpu + r[rs]]
        // or eax, imm16
        // mov [cpu + r[rt]], eax
    }

    void dynarec::Emitter::EmitAddiu(const DecodedInstruction& ins)
    {
        if (ins.rt == 0)
            return;

        u32 lhs = m_cpu.m_r[ins.rs];
        s32 rhs = (s16)ins.imm;
        u32 result = lhs + rhs;

        m_cpu.m_r[ins.rt] = result;

        // x64 version:
        // mov eax, [cpu + r[rs]]
        // add eax, imm32(sign-extended from imm16)
        // mov [cpu + r[rt]], eax
    }

    bool dynarec::Emitter::EmitInstruction(const DecodedInstruction& ins)
    {
        // Pre tick bus here, same as interpreter in Bus::StepInstruction
        Host::GetBus().StepPreDynarec();

        if (!m_cpu.ExecutePrelog()) {
            return false;
        }
        if (m_cpu.m_PC != ins.pc)
        {
            printf(
                "Pipeline desync! cpuPC=%08X compiledPC=%08X raw=%08X",
                m_cpu.m_PC,
                ins.pc,
                ins.raw);
            exit(1);
        }

        bool inst = false;

        switch (ins.kind)
        {
        case OP_LUI:
            EmitLui(ins);
            break;

        case OP_ORI:
            EmitOri(ins);
            break;

        case OP_ADDIU:
            EmitAddiu(ins);
            break;
        default:
            inst = EmitFallback(ins);
        }

        // Post tick here, same as interpreter in Bus::StepInstruction
        Host::GetBus().StepDynarec();
        return inst;
    }

    bool Emitter::EmitFallback(const DecodedInstruction& ins)
    {
#if 1
        u32 live = m_cpu.m_pReadWord(ins.pc, m_cpu.m_userdata);

        if (live != ins.raw)
        {
            printf("Self-modifying code!\n");
            //exit(1);
        }
#endif

        // Call interpreter/helper for one instruction
        return m_cpu.ExecuteOp(ins.raw);
    }
}

#endif // EXPERIMENTAL_DYNAREC