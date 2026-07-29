#pragma once

#include <core/Types.h>
#include <vector>

class R3000;

namespace dynarec
{
    typedef enum
    {
        OP_UNKNOWN,
        OP_BLTZ,
        OP_BGEZ,
        OP_BLTZAL,
        OP_BGEZAL,
        OP_J,
        OP_JAL,
        OP_BEQ,
        OP_BNE,
        OP_BLEZ,
        OP_BGTZ,
        OP_ADDI,
        OP_ADDIU,
        OP_SLTI,
        OP_SLTIU,
        OP_ANDI,
        OP_ORI,
        OP_XORI,
        OP_LUI,
        OP_MFC0,
        OP_CFC0,
        OP_MTC0,
        OP_CTC0,
        OP_BC0F,
        OP_BC0T,
        OP_TLBR,
        OP_TLBWI,
        OP_TLBWR,
        OP_TLBP,
        OP_RFE,
        OP_MFC1,
        OP_CFC1,
        OP_MTC1,
        OP_CTC1,
        OP_BC1F,
        OP_BC1T,
        OP_MFC2,
        OP_CFC2,
        OP_MTC2,
        OP_CTC2,
        OP_BC2F,
        OP_BC2T,
        OP_MFC3,
        OP_CFC3,
        OP_MTC3,
        OP_CTC3,
        OP_BC3F,
        OP_BC3T,
        OP_LB,
        OP_LH,
        OP_LWL,
        OP_LW,
        OP_LBU,
        OP_LHU,
        OP_LWR,
        OP_SB,
        OP_SH,
        OP_SWL,
        OP_SW,
        OP_SWR,
        OP_LWC0,
        OP_LWC1,
        OP_LWC2,
        OP_LWC3,
        OP_SWC0,
        OP_SWC1,
        OP_SWC2,
        OP_SWC3,
        OP_SLL,
        OP_SRL,
        OP_SRA,
        OP_SLLV,
        OP_SRLV,
        OP_SRAV,
        OP_JR,
        OP_JALR,
        OP_SYSCALL,
        OP_BREAK,
        OP_MFHI,
        OP_MTHI,
        OP_MFLO,
        OP_MTLO,
        OP_MULT,
        OP_MULTU,
        OP_DIV,
        OP_DIVU,
        OP_ADD,
        OP_ADDU,
        OP_SUB,
        OP_SUBU,
        OP_AND,
        OP_OR,
        OP_XOR,
        OP_NOR,
        OP_SLT,
        OP_SLTU,
        OP_GTE_RTPS,
        OP_GTE_NCLIP,
        OP_GTE_OP,
        OP_GTE_DPCS,
        OP_GTE_INTPL,
        OP_GTE_MVMVA,
        OP_GTE_NCDS,
        OP_GTE_CDP,
        OP_GTE_NCDT,
        OP_GTE_NCCS,
        OP_GTE_CC,
        OP_GTE_NCS,
        OP_GTE_NCT,
        OP_GTE_SQR,
        OP_GTE_DCPL,
        OP_GTE_DPCT,
        OP_GTE_AVSZ3,
        OP_GTE_AVSZ4,
        OP_GTE_RTPT,
        OP_GTE_GPF,
        OP_GTE_GPL,
        OP_GTE_NCCT,
    } DecodedOpcode;

    struct Opcode
    {
        u32 mask;
        u32 match;
        u32 cycles;
        DecodedOpcode kind;
        const char* instruction;
    };

    struct DecodedInstruction
    {
        u32 pc;
        u32 raw;
        DecodedOpcode kind = OP_UNKNOWN;
        u8 rs = 0;
        u8 rt = 0;
        u8 rd = 0;
        u8 sa = 0;
        s16 imm = 0;
        u32 target = 0;
    };

    struct CompiledBlock
    {
        u32 startPC;
        u32 endPC;

        std::vector<DecodedInstruction> instructions;

        uint8_t* nativeCode = nullptr;
        size_t nativeSize = 0;
    };

    class Compiler
    {
    public:
        static const Opcode* DecodeOpcode(u32 opcode);

        Compiler() = default;
        Compiler(const Compiler&) = default;
        Compiler(R3000& cpu) : m_cpu(cpu) {}
        CompiledBlock CompileBlock(u32 pcStart);
        bool EndsBlock(u32 opcode);
    private:
        R3000& m_cpu;
    };
}