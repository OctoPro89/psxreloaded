#include "Dynarec.h"
#include <Host.h>
#include <cstddef>

#ifdef EXPERIMENTAL_DYNAREC

namespace dynarec
{
	static constexpr Opcode kOpcodes[] =
	{
		// Mask       Match      Cyc OP  Instruction
		{ 0xfc1f0000, 0x04000000, 2, OP_BLTZ,  "bltz" },
		{ 0xfc1f0000, 0x04010000, 2, OP_BGEZ,  "bgez" },
		{ 0xfc1f0000, 0x04100000, 2, OP_BLTZAL,  "bltzal" },
		{ 0xfc1f0000, 0x04110000, 2, OP_BGEZAL,  "bgezal" },
		{ 0xfc000000, 0x08000000, 2, OP_J,  "j" },
		{ 0xfc000000, 0x0c000000, 2, OP_JAL,  "jal" },
		{ 0xfc000000, 0x10000000, 2, OP_BEQ,  "beq" },
		{ 0xfc000000, 0x14000000, 2, OP_BNE,  "bne" },
		{ 0xfc000000, 0x18000000, 2, OP_BLEZ,  "blez" },
		{ 0xfc000000, 0x1c000000, 2, OP_BGTZ,  "bgtz" },
		{ 0xfc000000, 0x20000000, 2, OP_ADDI,  "addi" },
		{ 0xfc000000, 0x24000000, 2, OP_ADDIU,  "addiu" },
		{ 0xfc000000, 0x28000000, 2, OP_SLTI,  "slti" },
		{ 0xfc000000, 0x2c000000, 2, OP_SLTIU,  "sltiu" },
		{ 0xfc000000, 0x30000000, 2, OP_ANDI,  "andi" },
		{ 0xfc000000, 0x34000000, 2, OP_ORI,  "ori" },
		{ 0xfc000000, 0x38000000, 2, OP_XORI,  "xori" },
		{ 0xfc000000, 0x3c000000, 2, OP_LUI,  "lui" },
		{ 0xffe0003f, 0x40000000, 2, OP_MFC0,  "mfc0" },
		{ 0xffe0003f, 0x40400000, 2, OP_CFC0,  "cfc0" },
		{ 0xffe0003f, 0x40800000, 2, OP_MTC0,  "mtc0" },
		{ 0xffe0003f, 0x40c00000, 2, OP_CTC0,  "ctc0" },
		{ 0xffff0000, 0x41000000, 2, OP_BC0F,  "bc0f" },
		{ 0xffff0000, 0x41010000, 2, OP_BC0T,  "bc0t" },
		{ 0xffe0003f, 0x42000001, 2, OP_TLBR,  "tlbr" },
		{ 0xffe0003f, 0x42000002, 2, OP_TLBWI,  "tlbwi" },
		{ 0xffe0003f, 0x42000006, 2, OP_TLBWR,  "tlbwr" },
		{ 0xffe0003f, 0x42000008, 2, OP_TLBP,  "tlbp" },
		{ 0xffe0003f, 0x42000010, 2, OP_RFE,  "rfe" },
		{ 0xffe0003f, 0x44000000, 2, OP_MFC1,  "mfc1" },
		{ 0xffe0003f, 0x44400000, 2, OP_CFC1,  "cfc1" },
		{ 0xffe0003f, 0x44800000, 2, OP_MTC1,  "mtc1" },
		{ 0xffe0003f, 0x44c00000, 2, OP_CTC1,  "ctc1" },
		{ 0xffff0000, 0x45000000, 2, OP_BC1F,  "bc1f" },
		{ 0xffff0000, 0x45010000, 2, OP_BC1T,  "bc1t" },
		{ 0xffe0003f, 0x48000000, 2, OP_MFC2,  "mfc2" },
		{ 0xffe0003f, 0x48400000, 2, OP_CFC2,  "cfc2" },
		{ 0xffe0003f, 0x48800000, 2, OP_MTC2,  "mtc2" },
		{ 0xffe0003f, 0x48c00000, 2, OP_CTC2,  "ctc2" },
		{ 0xffff0000, 0x49000000, 2, OP_BC2F,  "bc2f" },
		{ 0xffff0000, 0x49010000, 2, OP_BC2T,  "bc2t" },
		{ 0xffe0003f, 0x4c000000, 2, OP_MFC3,  "mfc3" },
		{ 0xffe0003f, 0x4c400000, 2, OP_CFC3,  "cfc3" },
		{ 0xffe0003f, 0x4c800000, 2, OP_MTC3,  "mtc3" },
		{ 0xffe0003f, 0x4cc00000, 2, OP_CTC3,  "ctc3" },
		{ 0xffff0000, 0x4d000000, 2, OP_BC3F,  "bc3f" },
		{ 0xffff0000, 0x4d010000, 2, OP_BC3T,  "bc3t" },
		{ 0xfc000000, 0x80000000, 2, OP_LB,  "lb" },
		{ 0xfc000000, 0x84000000, 2, OP_LH,  "lh" },
		{ 0xfc000000, 0x88000000, 2, OP_LWL,  "lwl" },
		{ 0xfc000000, 0x8c000000, 2, OP_LW,  "lw" },
		{ 0xfc000000, 0x90000000, 2, OP_LBU,  "lbu" },
		{ 0xfc000000, 0x94000000, 2, OP_LHU,  "lhu" },
		{ 0xfc000000, 0x98000000, 2, OP_LWR,  "lwr" },
		{ 0xfc000000, 0xa0000000, 2, OP_SB,  "sb" },
		{ 0xfc000000, 0xa4000000, 2, OP_SH,  "sh" },
		{ 0xfc000000, 0xa8000000, 2, OP_SWL,  "swl" },
		{ 0xfc000000, 0xac000000, 2, OP_SW,  "sw" },
		{ 0xfc000000, 0xb8000000, 2, OP_SWR,  "swr" },
		{ 0xfc000000, 0xc0000000, 2, OP_LWC0,  "lwc0" },
		{ 0xfc000000, 0xc4000000, 2, OP_LWC1,  "lwc1" },
		{ 0xfc000000, 0xc8000000, 2, OP_LWC2,  "lwc2" },
		{ 0xfc000000, 0xcc000000, 2, OP_LWC3,  "lwc3" },
		{ 0xfc000000, 0xe0000000, 2, OP_SWC0,  "swc0" },
		{ 0xfc000000, 0xe4000000, 2, OP_SWC1,  "swc1" },
		{ 0xfc000000, 0xe8000000, 2, OP_SWC2,  "swc2" },
		{ 0xfc000000, 0xec000000, 2, OP_SWC3,  "swc3" },
		{ 0xfc00003f, 0x00000000, 2, OP_SLL,  "sll" },
		{ 0xfc00003f, 0x00000002, 2, OP_SRL,  "srl" },
		{ 0xfc00003f, 0x00000003, 2, OP_SRA,  "sra" },
		{ 0xfc00003f, 0x00000004, 2, OP_SLLV,  "sllv" },
		{ 0xfc00003f, 0x00000006, 2, OP_SRLV,  "srlv" },
		{ 0xfc00003f, 0x00000007, 2, OP_SRAV,  "srav" },
		{ 0xfc00003f, 0x00000008, 2, OP_JR,  "jr" },
		{ 0xfc00003f, 0x00000009, 2, OP_JALR,  "jalr" },
		{ 0xfc00003f, 0x0000000c, 2, OP_SYSCALL,  "syscall" },
		{ 0xfc00003f, 0x0000000d, 2, OP_BREAK,  "break" },
		{ 0xfc00003f, 0x00000010, 2, OP_MFHI,  "mfhi" },
		{ 0xfc00003f, 0x00000011, 2, OP_MTHI,  "mthi" },
		{ 0xfc00003f, 0x00000012, 2, OP_MFLO,  "mflo" },
		{ 0xfc00003f, 0x00000013, 2, OP_MTLO,  "mtlo" },
		{ 0xfc00003f, 0x00000018, 2, OP_MULT,  "mult" },
		{ 0xfc00003f, 0x00000019, 2, OP_MULTU,  "multu" },
		{ 0xfc00003f, 0x0000001a, 2, OP_DIV,  "div" },
		{ 0xfc00003f, 0x0000001b, 2, OP_DIVU,  "divu" },
		{ 0xfc00003f, 0x00000020, 2, OP_ADD,  "add" },
		{ 0xfc00003f, 0x00000021, 2, OP_ADDU,  "addu" },
		{ 0xfc00003f, 0x00000022, 2, OP_SUB,  "sub" },
		{ 0xfc00003f, 0x00000023, 2, OP_SUBU,  "subu" },
		{ 0xfc00003f, 0x00000024, 2, OP_AND,  "and" },
		{ 0xfc00003f, 0x00000025, 2, OP_OR, "or" },
		{ 0xfc00003f, 0x00000026, 2, OP_XOR, "xor" },
		{ 0xfc00003f, 0x00000027, 2, OP_NOR,  "nor" },
		{ 0xfc00003f, 0x0000002a, 2, OP_SLT,  "slt" },
		{ 0xfc00003f, 0x0000002b, 2, OP_SLTU,  "sltu" },
		{ 0xfe00003f, 0x4a000001, 15, OP_GTE_RTPS,  "gte_rtps" },
		{ 0xfe00003f, 0x4a000006, 8, OP_GTE_NCLIP,  "gte_nclip" },
		{ 0xfe00003f, 0x4a00000c, 6, OP_GTE_OP,  "gte_op" },
		{ 0xfe00003f, 0x4a000010, 8, OP_GTE_DPCS,  "gte_dpcs" },
		{ 0xfe00003f, 0x4a000011, 8, OP_GTE_INTPL,  "gte_intpl" },
		{ 0xfe00003f, 0x4a000012, 8, OP_GTE_MVMVA,  "gte_mvmva" },
		{ 0xfe00003f, 0x4a000013, 19, OP_GTE_NCDS,  "gte_ncds" },
		{ 0xfe00003f, 0x4a000014, 13, OP_GTE_CDP,  "gte_cdp" },
		{ 0xfe00003f, 0x4a000016, 44, OP_GTE_NCDT,  "gte_ncdt" },
		{ 0xfe00003f, 0x4a00001b, 17, OP_GTE_NCCS,  "gte_nccs" },
		{ 0xfe00003f, 0x4a00001c, 11, OP_GTE_CC,  "gte_cc" },
		{ 0xfe00003f, 0x4a00001e, 14, OP_GTE_NCS,  "gte_ncs" },
		{ 0xfe00003f, 0x4a000020, 30, OP_GTE_NCT,  "gte_nct" },
		{ 0xfe00003f, 0x4a000028, 5, OP_GTE_SQR,  "gte_sqr" },
		{ 0xfe00003f, 0x4a000029, 8, OP_GTE_DCPL,  "gte_dcpl" },
		{ 0xfe00003f, 0x4a00002a, 17, OP_GTE_DPCT,  "gte_dpct" },
		{ 0xfe00003f, 0x4a00002d, 5, OP_GTE_AVSZ3,  "gte_avsz3" },
		{ 0xfe00003f, 0x4a00002e, 6, OP_GTE_AVSZ4,  "gte_avsz4" },
		{ 0xfe00003f, 0x4a000030, 23, OP_GTE_RTPT,  "gte_rtpt" },
		{ 0xfe00003f, 0x4a00003d, 5, OP_GTE_GPF,  "gte_gpf" },
		{ 0xfe00003f, 0x4a00003e, 5, OP_GTE_GPL,  "gte_gpl" },
		{ 0xfe00003f, 0x4a00003f, 39, OP_GTE_NCCT,  "gte_ncct" }
	};

	typedef enum
	{
		Unknown,
		AluImm,
		AluReg,
		Load,
		Store,
		Branch,
		Jump,
		Syscall,
		Cop0,
		Cop1,
		Gte,
	} InstrClass;

	void DecodeInstruction(DecodedInstruction& ins)
	{
		ins.rs = (ins.raw >> 21) & 0x1F;
		ins.rt = (ins.raw >> 16) & 0x1F;
		ins.rd = (ins.raw >> 11) & 0x1F;
		ins.sa = (ins.raw >> 6) & 0x1F;
		ins.imm = (s16)(ins.raw & 0xFFFF);
		ins.target = (ins.raw & 0x03FFFFFF) << 2;
	}

	const Opcode* Compiler::DecodeOpcode(u32 opcode)
	{
		for (const auto& entry : kOpcodes)
		{
			if ((opcode & entry.mask) == entry.match)
				return &entry;
		}

		return nullptr;
	}

	CompiledBlock Compiler::CompileBlock(u32 pcStart)
	{
		CompiledBlock block{};
		block.startPC = pcStart;

		u32 pc = pcStart;
		constexpr size_t kMaxBlockInsns = 256;

		for (size_t i = 0; i < kMaxBlockInsns; ++i)
		{
			u32 raw = m_cpu.m_pReadWord(pc, m_cpu.m_userdata);

			DecodedInstruction ins{};
			ins.pc = pc;
			ins.raw = raw;

			const Opcode* def = DecodeOpcode(raw);
			if (!def)
			{
				block.instructions.push_back(ins);
				break; // unknown opcode: end block safely
			}

			ins.kind = def->kind;
			DecodeInstruction(ins);
			block.instructions.push_back(ins);

			if (EndsBlock(raw))
			{
				pc += 4;

				raw = m_cpu.m_pReadWord(pc, m_cpu.m_userdata);

				DecodedInstruction delay{};
				delay.pc = pc;
				delay.raw = raw;

				if (const Opcode* delayDef = DecodeOpcode(raw))
					delay.kind = delayDef->kind;

				DecodeInstruction(delay);
				block.instructions.push_back(delay);

				pc += 4;
				break;
			}

			pc += 4;
		}

		block.endPC = pc;
		return block;
	}

    bool Compiler::EndsBlock(u32 opcode)
    {
        u32 primary = opcode >> 26;

        switch (primary)
        {
        case 0x02: // J
        case 0x03: // JAL
        case 0x04: // BEQ
        case 0x05: // BNE
        case 0x06: // BLEZ
        case 0x07: // BGTZ
            return true;

        case 0x01: // BcondZ
            return true;

        case 0x00:
        {
            u32 funct = opcode & 0x3f;

            switch (funct)
            {
            case 0x08: // JR
            case 0x09: // JALR
            case 0x0C: // SYSCALL
            case 0x0D: // BREAK
                return true;
            }

            break;
        }
        }

        return false;
    }
} // namespace dynarec

#endif // EXPERIMENTAL_DYNAREC