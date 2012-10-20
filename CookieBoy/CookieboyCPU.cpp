#include "CookieboyCPU.h"
#include <stdio.h>
#include <assert.h>
#include <Windows.h>

#include "CookieboyMemory.h"
#include "CookieboyInterrupts.h"
#include "CookieboyDividerTimer.h"
#include "CookieboyTIMATimer.h"
#include "CookieboyGPU.h"
#include "CookieboySound.h"
#include "CookieboyJoypad.h"
#include "CookieboySerialIO.h"

/*
Passed tests
	CPU opcodes - Passed
		01-special
		02-interrupts - very tricky:
						1. My HALT was wrong
						2. My interrupts handling routine also was wrong - check GameboyInterrupts.cpp
		03-op sp,hl
		04-op r,imm
		05-op rp
		06-ld r,r
		07-jr,jp,call,ret,rst
		09-op r,r - you need to reset Z in RLCA RLA RRCA RRA to pass
		10-bit ops
		11-op a,(hl)

	CPU timings - Passed
		instr_timing - some ops are wrong, some ops depend on branch being taken

	Memory timings - Passed
		01-read_timing
		02-write_timing
		03-modify_timing
*/

#define SYNC_WITH_CPU(clockDelta)	\
	GPU.Step(clockDelta, INT);		\
	DIV.Step(clockDelta);			\
	TIMA.Step(clockDelta, INT);		\
	Serial.Step(clockDelta, INT);	\
	Joypad.Step(INT);				\
	Sound.Step(clockDelta);

//Zero flag (7 bit) - set when the result of a math operation is zero or two values match when using the CP instruction.
#define SET_FLAG_Z(value) (F = ((value) << 7) | (F & 0x7F))

//Substract flag (6 bit) - set if a subtraction was performed in the last math instruction.
#define SET_FLAG_N(value) (F = ((value) << 6) | (F & 0xBF))

//Half carry flag (5 bit) - set if a carry occurred from the lower nibble in the last math operation
#define SET_FLAG_H(value) (F = ((value) << 5) | (F & 0xDF))

//Carry flag (4 bit) - set if a carry occurred from the last math operation or if register A is the smaller value when executing the CP instruction
#define SET_FLAG_C(value) (F = ((value) << 4) | (F & 0xEF))

#define GET_FLAG_Z() ((F >> 7) & 0x1)
#define GET_FLAG_N() ((F >> 6) & 0x1)
#define GET_FLAG_H() ((F >> 5) & 0x1)
#define GET_FLAG_C() ((F >> 4) & 0x1)

#define ADD_N(value)										\
	{SET_FLAG_Z(((A + (value)) & 0xFF) == 0);				\
	SET_FLAG_N(0);											\
	SET_FLAG_H((((A & 0xF) + ((value) & 0xF)) >> 4) & 0x1);	\
	SET_FLAG_C(((A + (value)) >> 8) & 0x1);					\
															\
	A += (value);}

#define ADC_N(value)												\
	{int FlagC = GET_FLAG_C();										\
																	\
	SET_FLAG_Z(((A + (value) + FlagC) & 0xFF) == 0);				\
	SET_FLAG_N(0);													\
	SET_FLAG_H((((A & 0xF) + ((value) & 0xF) + FlagC) >> 4) & 0x1);	\
	SET_FLAG_C(((A + (value) + FlagC) >> 8) & 0x1);					\
																	\
	A += (value) + FlagC;}

#define SBC_N(value)										\
	{int result = A - (value) - GET_FLAG_C();				\
															\
	SET_FLAG_N(1);											\
	SET_FLAG_H((A & 0xF) < ((value) & 0xF) + GET_FLAG_C());	\
	SET_FLAG_C(result < 0);									\
															\
	A = result;												\
															\
	SET_FLAG_Z(A == 0);}

#define SUB_N(value)						\
	{SET_FLAG_Z(A == (value));				\
	SET_FLAG_N(1);							\
	SET_FLAG_H((A & 0xF) < ((value) & 0xF));\
	SET_FLAG_C(A < (value));}				\
											\
	A -= (value);

#define AND_N(value)	\
	{A &= (value);		\
						\
	SET_FLAG_Z(A == 0);	\
	SET_FLAG_N(0);		\
	SET_FLAG_H(1);		\
	SET_FLAG_C(0);}

#define OR_N(value)		\
	{A |= (value);		\
						\
	SET_FLAG_Z(A == 0);	\
	SET_FLAG_N(0);		\
	SET_FLAG_H(0);		\
	SET_FLAG_C(0);}

#define XOR_N(value)	\
	{A ^= (value);		\
						\
	SET_FLAG_Z(A == 0);	\
	SET_FLAG_N(0);		\
	SET_FLAG_H(0);		\
	SET_FLAG_C(0);}

#define CP_N(value)							\
	{SET_FLAG_Z(A == (value));				\
	SET_FLAG_N(1);							\
	SET_FLAG_H((A & 0xF) < ((value) & 0xF));\
	SET_FLAG_C(A < (value));}

#define INC_N(value)								\
	{SET_FLAG_Z((((value) + 1) & 0xFF) == 0);		\
	SET_FLAG_N(0);									\
	SET_FLAG_H(((((value) & 0xF) + 1) >> 4) & 0x1);	\
													\
	value++;}

#define DEC_N(value)					\
	{SET_FLAG_Z((value) == 1);			\
	SET_FLAG_N(1);						\
	SET_FLAG_H(((value) & 0xF) < 1);	\
										\
	(value)--;}

#define ADD_HL(value)													\
	{SET_FLAG_N(0);														\
	SET_FLAG_H((((HL.word & 0xFFF) + ((value) & 0xFFF)) >> 12) & 0x1);	\
	SET_FLAG_C(((HL.word + (value)) >> 16) & 0x1);						\
																		\
	HL.word += (value);													\
																		\
	SYNC_WITH_CPU(4);}

#define INC_NN(value)	\
	{(value)++;			\
						\
	SYNC_WITH_CPU(4);}

#define DEC_NN(value)	\
	{(value)--;			\
						\
	SYNC_WITH_CPU(4);}

#define JP_CC_NN(condition)			\
	{WORD addr = MemoryReadWord(PC);\
	PC += 2;						\
									\
	if ((condition))				\
	{								\
		DelayedPCChange(addr);		\
	}}

#define JR_CC_NN(condition)				\
	{signed char addr = MemoryRead(PC);	\
	PC++;								\
										\
	if ((condition))					\
	{									\
		DelayedPCChange(PC + addr);		\
	}}

#define CALL_CC_NN(condition)		\
	{WORD addr = MemoryReadWord(PC);\
	PC += 2;						\
									\
	if ((condition))				\
	{								\
		StackPushWord(PC);			\
									\
		PC = addr;					\
	}}

#define RST_N(addr)		\
	{StackPushWord(PC);	\
						\
	PC = addr;}

#define RET_CC(condition)					\
	{SYNC_WITH_CPU(4);						\
											\
	if ((condition))						\
	{										\
		DelayedPCChange(StackPopWord());	\
	}}

#define SWAP_N(value)											\
	{(value) = (((value) & 0xF) << 4) | (((value) & 0xF0) >> 4);\
																\
	SET_FLAG_Z((value) == 0);									\
	SET_FLAG_N(0);												\
	SET_FLAG_H(0);												\
	SET_FLAG_C(0);}

#define RLC_N(value)				\
	{BYTE MSB = (value) >> 7;		\
									\
	(value) = ((value) << 1) | MSB;	\
									\
	SET_FLAG_Z((value) == 0);		\
	SET_FLAG_H(0);					\
	SET_FLAG_N(0);					\
	SET_FLAG_C(MSB);}

#define RL_N(value)							\
	{BYTE MSB = (value) >> 7;				\
											\
	(value) = ((value) << 1) | GET_FLAG_C();\
											\
	SET_FLAG_Z((value) == 0);				\
	SET_FLAG_N(0);							\
	SET_FLAG_H(0);							\
	SET_FLAG_C(MSB);}

#define RRC_N(value)						\
	{BYTE LSB = (value) & 0x1;				\
											\
	(value) = ((value) >> 1) | (LSB << 7);	\
											\
	SET_FLAG_Z((value) == 0);				\
	SET_FLAG_H(0);							\
	SET_FLAG_N(0);							\
	SET_FLAG_C(LSB);}

#define RR_N(value)									\
	{BYTE LSB = (value) & 0x1;						\
													\
	(value) = ((value) >> 1) | (GET_FLAG_C() << 7);	\
													\
	SET_FLAG_Z((value) == 0);						\
	SET_FLAG_H(0);									\
	SET_FLAG_N(0);									\
	SET_FLAG_C(LSB);}

#define SLA_N(value)			\
	{BYTE MSB = (value) >> 7;	\
								\
	(value) <<= 1;				\
								\
	SET_FLAG_Z((value) == 0);	\
	SET_FLAG_H(0);				\
	SET_FLAG_N(0);				\
	SET_FLAG_C(MSB);}

#define SRA_N(value)							\
	{BYTE LSB = (value) & 0x1;					\
												\
	(value) = ((value) >> 1) | ((value) & 0x80);\
												\
	SET_FLAG_Z((value) == 0);					\
	SET_FLAG_H(0);								\
	SET_FLAG_N(0);								\
	SET_FLAG_C(LSB);}

#define SRL_N(value)			\
	{BYTE LSB = (value) & 0x1;	\
								\
	(value) >>= 1;				\
								\
	SET_FLAG_Z((value) == 0);	\
	SET_FLAG_H(0);				\
	SET_FLAG_N(0);				\
	SET_FLAG_C(LSB);}

#define BITX_N(BitIndex, value)						\
	{SET_FLAG_Z(((~(value)) >> (BitIndex)) & 0x1);	\
	SET_FLAG_N(0);									\
	SET_FLAG_H(1);}

Cookieboy::CPU::CPU(Cookieboy::Memory &MMU,
					Cookieboy::GPU &GPU,
					Cookieboy::DividerTimer &DIV,
					Cookieboy::TIMATimer &TIMA,
					Cookieboy::Joypad &joypad, 
					Cookieboy::Sound &sound,
					Cookieboy::SerialIO &serial,
					Cookieboy::Interrupts &INT):
MMU(MMU),
GPU(GPU),
DIV(DIV),
TIMA(TIMA),
Joypad(joypad),
Sound(sound),
Serial(serial),
INT(INT)
{
	Reset();
}

void Cookieboy::CPU::Reset()
{
	A = 0;
	F = 0;
	BC.word = 0;
	DE.word = 0;
	HL.word = 0;
	SP = 0xFFFE;
	PC = 0;
	IME = 0;
	Halted = false;
	HaltBug = false;
	DIDelay = 0;
	EIDelay = 0;
}

void Cookieboy::CPU::EmulateBIOS()
{
	Reset();

	A = 0x01;
	F = 0xB0;
	BC.word = 0x0013;
	DE.word = 0x00D8;
	HL.word = 0x014D;
	SP = 0xFFFE;

	PC = 0x100;
}

void Cookieboy::CPU::Step()
{
	WORD immValueW = 0;
	BYTE immValueB = 0;
	
	BYTE opcode = MemoryRead(PC);
	PC++;

#ifdef DEBUG_PRINTCPU
	static char disassembly[63] = {'\0'};
#endif

	//Halt bug occurs when HALT executed while IME=0
	//CPU continues to run but instruction after HALT 
	//is executed twice.
	if (!Halted && HaltBug)
	{
		HaltBug = false;
		PC--;
	}

	//Delaying until next instruction is executed
	if (DIDelay > 0)
	{
		if (DIDelay == 1)
		{
			IME = 0;
		}

		DIDelay--;
	}
	if (EIDelay > 0)
	{
		if (EIDelay == 1)
		{
			IME = 1;
		}

		EIDelay--;
	}

	switch (opcode)
	{
		#pragma region LD r, n
		case 0x06:
			BC.bytes.H = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD B, 0x%X", BC.bytes.H);
			break;
		case 0x0E:
			BC.bytes.L = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD C, 0x%X", BC.bytes.L);
			break;
		case 0x16:
			DE.bytes.H = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD D, 0x%X", DE.bytes.H);
			break;
		case 0x1E:
			DE.bytes.L = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD E, 0x%X", DE.bytes.L);
			break;
		case 0x26:
			HL.bytes.H = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD H, 0x%X", HL.bytes.H);
			break;
		case 0x2E:
			HL.bytes.L = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD L, 0x%X", HL.bytes.L);
			break;
		#pragma endregion
		#pragma region LD r, r
		case 0x7F:
			A = A;

			PRINT_INSTRUCTION(disassembly, "LD A, A");
			break;
		case 0x78:
			A = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD A, B");
			break;
		case 0x79:
			A = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD A, C");
			break;
		case 0x7A:
			A = DE.bytes.H;	
			PRINT_INSTRUCTION(disassembly, "LD A, D");
			break;
		case 0x7B:
			A = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD A, E");
			break;
		case 0x7C:
			A = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD A, H");
			break;
		case 0x7D:
			A = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD A, L");
			break;
		case 0x7E:
			A = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD A, (HL)");
			break;
		case 0x40:
			BC.bytes.H = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD B, B");
			break;
		case 0x41:
			BC.bytes.H = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD B, C");
			break;
		case 0x42:
			BC.bytes.H = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD B, D");
			break;
		case 0x43:
			BC.bytes.H = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD B, E");
			break;
		case 0x44:
			BC.bytes.H = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD B, H");
			break;
		case 0x45:
			BC.bytes.H = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD B, L");
			break;
		case 0x46:
			BC.bytes.H = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD B, (HL)");
			break;
		case 0x48:
			BC.bytes.L = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD C, B");
			break;
		case 0x49:
			BC.bytes.L = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD C, C");
			break;
		case 0x4A:
			BC.bytes.L = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD C, D");
			break;
		case 0x4B:
			BC.bytes.L = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD C, E");
			break;
		case 0x4C:
			BC.bytes.L = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD C, H");
			break;
		case 0x4D:
			BC.bytes.L = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD C, L");
			break;
		case 0x4E:
			BC.bytes.L = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD C, (HL)");
			break;
		case 0x50:
			DE.bytes.H = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD D, B");
			break;
		case 0x51:
			DE.bytes.H = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD D, C");
			break;
		case 0x52:
			DE.bytes.H = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD D, D");
			break;
		case 0x53:
			DE.bytes.H = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD D, E");
			break;
		case 0x54:
			DE.bytes.H = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD D, H");
			break;
		case 0x55:
			DE.bytes.H = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD D, L");
			break;
		case 0x56:
			DE.bytes.H = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD D, (HL)");
			break;
		case 0x58:
			DE.bytes.L = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD E, B");
			break;
		case 0x59:
			DE.bytes.L = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD E, C");
			break;
		case 0x5A:
			DE.bytes.L = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD E, D");
			break;
		case 0x5B:
			DE.bytes.L = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD E, E");
			break;
		case 0x5C:
			DE.bytes.L = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD E, H");
			break;
		case 0x5D:
			DE.bytes.L = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD E, L");
			break;
		case 0x5E:
			DE.bytes.L = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD E, (HL)");
			break;
		case 0x60:
			HL.bytes.H = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD H, B");
			break;
		case 0x61:
			HL.bytes.H = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD H, C");
			break;
		case 0x62:
			HL.bytes.H = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD H, D");
			break;
		case 0x63:
			HL.bytes.H = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD H, E");
			break;
		case 0x64:
			HL.bytes.H = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD H, H");
			break;
		case 0x65:
			HL.bytes.H = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD H, L");
			break;
		case 0x66:
			HL.bytes.H = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD H, (HL)");
			break;
		case 0x68:
			HL.bytes.L = BC.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD L, B");
			break;
		case 0x69:
			HL.bytes.L = BC.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD L, C");
			break;
		case 0x6A:
			HL.bytes.L = DE.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD L, D");
			break;
		case 0x6B:
			HL.bytes.L = DE.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD L, E");
			break;
		case 0x6C:
			HL.bytes.L = HL.bytes.H;

			PRINT_INSTRUCTION(disassembly, "LD L, H");
			break;
		case 0x6D:
			HL.bytes.L = HL.bytes.L;

			PRINT_INSTRUCTION(disassembly, "LD L, L");
			break;
		case 0x6E:
			HL.bytes.L = MemoryRead(HL.word);

			PRINT_INSTRUCTION(disassembly, "LD L, (HL)");
			break;
		case 0x70:
			MemoryWrite(HL.word, BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "LD (HL), B");
			break;
		case 0x71:
			MemoryWrite(HL.word, BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "LD (HL), C");
			break;
		case 0x72:
			MemoryWrite(HL.word, DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "LD (HL), D");
			break;
		case 0x73:
			MemoryWrite(HL.word, DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "LD (HL), E");
			break;
		case 0x74:
			MemoryWrite(HL.word, HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "LD (HL), H");
			break;
		case 0x75:
			MemoryWrite(HL.word, HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "LD (HL), L");
			break;
		case 0x36:
			immValueB = MemoryRead(PC);
			PC++;

			MemoryWrite(HL.word, immValueB);

			PRINT_INSTRUCTION(disassembly, "LD (HL), 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region LD A, n
		case 0x0A:
			A = MemoryRead(BC.word);

			PRINT_INSTRUCTION(disassembly, "LD A, (BC)");
			break;
		case 0x1A:
			A = MemoryRead(DE.word);

			PRINT_INSTRUCTION(disassembly, "LD A, (DE)");
			break;
		case 0xFA:
			immValueW = MemoryReadWord(PC);
			PC += 2;

			A = MemoryRead(immValueW);
			
			PRINT_INSTRUCTION(disassembly, "LD A, (0x%X)", immValueW);
			break;
		case 0x3E:
			A = MemoryRead(PC);
			PC++;

			PRINT_INSTRUCTION(disassembly, "LD A, 0x%X", A);
			break;
		#pragma endregion
		#pragma region LD n, A
		case 0x47:
			BC.bytes.H = A;

			PRINT_INSTRUCTION(disassembly, "LD B, A");
			break;
		case 0x4F:
			BC.bytes.L = A;

			PRINT_INSTRUCTION(disassembly, "LD C, A");
			break;
		case 0x57:
			DE.bytes.H = A;

			PRINT_INSTRUCTION(disassembly, "LD D, A");
			break;
		case 0x5F:
			DE.bytes.L = A;

			PRINT_INSTRUCTION(disassembly, "LD E, A");
			break;
		case 0x67:
			HL.bytes.H = A;

			PRINT_INSTRUCTION(disassembly, "LD H, A");
			break;
		case 0x6F:
			HL.bytes.L = A;

			PRINT_INSTRUCTION(disassembly, "LD L, A");
			break;
		case 0x02:
			MemoryWrite(BC.word, A);

			PRINT_INSTRUCTION(disassembly, "LD (BC), A");
			break;
		case 0x12:
			MemoryWrite(DE.word, A);

			PRINT_INSTRUCTION(disassembly, "LD (DE), A");
			break;
		case 0x77:
			MemoryWrite(HL.word, A);

			PRINT_INSTRUCTION(disassembly, "LD (HL), A");
			break;
		case 0xEA:
			immValueW = MemoryReadWord(PC);
			PC += 2;

			MemoryWrite(immValueW, A);

			PRINT_INSTRUCTION(disassembly, "LD (0x%X), A", immValueW);
			break;
		#pragma endregion
		#pragma region LD A, (C)
		case 0xF2:
			A = MemoryRead(0xFF00 + BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "LD A, (0xFF00 + C)");
			break;
		#pragma endregion
		#pragma region LD (C), A
		case 0xE2:
			MemoryWrite(0xFF00 + BC.bytes.L, A);

			PRINT_INSTRUCTION(disassembly, "LD (0xFF00 + C), A");
			break;
		#pragma endregion
		#pragma region LDD A, (HL)
		case 0x3A:
			A = MemoryRead(HL.word);
			HL.word--;

			PRINT_INSTRUCTION(disassembly, "LDD A, (HL)");
			break;
		#pragma endregion
		#pragma region LDD (HL), A
		case 0x32:
			MemoryWrite(HL.word, A);
			HL.word--;

			PRINT_INSTRUCTION(disassembly, "LDD (HL), A");
			break;
		#pragma endregion
		#pragma region LDI A, (HL)
		case 0x2A:
			A = MemoryRead(HL.word);
			HL.word++;

			PRINT_INSTRUCTION(disassembly, "LDI A, (HL)");
			break;
		#pragma endregion
		#pragma region LDI (HL), A
		case 0x22:
			MemoryWrite(HL.word, A);
			HL.word++;

			PRINT_INSTRUCTION(disassembly, "LDI (HL), A");
			break;
		#pragma endregion
		#pragma region LDH (n), A
		case 0xE0:
			immValueB = MemoryRead(PC);
			PC++;
			
			MemoryWrite(0xFF00 + immValueB, A);

			PRINT_INSTRUCTION(disassembly, "LDH (0xFF00 + 0x%X), A", immValueB);
			break;
		#pragma endregion
		#pragma region LDH A, (n)
		case 0xF0:
			immValueB = MemoryRead(PC);
			PC++;

			A = MemoryRead(0xFF00 + immValueB);
	
			PRINT_INSTRUCTION(disassembly, "LDH A, (0xFF00 + 0x%X)", immValueB);
			break;
		#pragma endregion
		#pragma region LD n, nn
		case 0x01:
			BC.word = MemoryReadWord(PC);
			PC += 2;

			PRINT_INSTRUCTION(disassembly, "LD BC, 0x%X", BC.word);
			break;
		case 0x11:
			DE.word = MemoryReadWord(PC);
			PC += 2;

			PRINT_INSTRUCTION(disassembly, "LD DE, 0x%X", DE.word);
			break;
		case 0x21:
			HL.word = MemoryReadWord(PC);
			PC += 2;

			PRINT_INSTRUCTION(disassembly, "LD HL, 0x%X", HL.word);
			break;
		case 0x31:
			SP = MemoryReadWord(PC);
			PC += 2;

			PRINT_INSTRUCTION(disassembly, "LD SP, 0x%X", SP);
			break;
		#pragma endregion
		#pragma region LD SP, HL
		case 0xF9:
			SP = HL.word;

			SYNC_WITH_CPU(4);
			PRINT_INSTRUCTION(disassembly, "LD SP, HL");
			break;
		#pragma endregion
		#pragma region LDHL SP, n
		case 0xF8:
			immValueB = MemoryRead(PC);
			PC++;

			immValueW =  SP + (signed char)immValueB;
			
			SET_FLAG_Z(0);
			SET_FLAG_N(0);
			SET_FLAG_C((immValueW  & 0xFF) < (SP & 0xFF));
			SET_FLAG_H((immValueW & 0xF) < (SP & 0xF));

			HL.word = immValueW;

			SYNC_WITH_CPU(4);

			PRINT_INSTRUCTION(disassembly, "LDHL SP, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region LD (nn), SP
		case 0x08:
			immValueW = MemoryReadWord(PC);
			PC += 2;

			MemoryWriteWord(immValueW, SP);

			PRINT_INSTRUCTION(disassembly, "LD (0x%X), SP", immValueW);
			break;
		#pragma endregion
		#pragma region PUSH nn
		case 0xF5:
			StackPushByte(A);
			StackPushByte(F);

			SYNC_WITH_CPU(4);
			PRINT_INSTRUCTION(disassembly, "PUSH AF");
			break;
		case 0xC5:
			StackPushWord(BC.word);

			PRINT_INSTRUCTION(disassembly, "PUSH BC");
			break;
		case 0xD5:
			StackPushWord(DE.word);

			PRINT_INSTRUCTION(disassembly, "PUSH DE");
			break;
		case 0xE5:
			StackPushWord(HL.word);

			PRINT_INSTRUCTION(disassembly, "PUSH HL");
			break;
		#pragma endregion
		#pragma region POP nn
		case 0xF1:
			F = StackPopByte();
			A = StackPopByte();

			//Required to pass some CPU ops tests
			F &= 0xF0;

			PRINT_INSTRUCTION(disassembly, "POP AF");
			break;
		case 0xC1:
			BC.word = StackPopWord();

			PRINT_INSTRUCTION(disassembly, "POP BC");
			break;
		case 0xD1:
			DE.word = StackPopWord();

			PRINT_INSTRUCTION(disassembly, "POP DE");
			break;
		case 0xE1:
			HL.word = StackPopWord();

			PRINT_INSTRUCTION(disassembly, "POP HL");
			break;
		#pragma endregion
		#pragma region ADD A, n
		case 0x87:
			ADD_N(A);

			PRINT_INSTRUCTION(disassembly, "ADD A, A");
			break;
		case 0x80:
			ADD_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADD A, B");
			break;
		case 0x81:
			ADD_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "ADD A, C");
			break;
		case 0x82:
			ADD_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADD A, D");
			break;
		case 0x83:
			ADD_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "ADD A, E");
			break;
		case 0x84:
			ADD_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADD A, H");
			break;
		case 0x85:
			ADD_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "ADD A, L");
			break;
		case 0x86:
			immValueB = MemoryRead(HL.word);
			ADD_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "ADD A, (HL)");
			break;
		case 0xC6:
			immValueB = MemoryRead(PC);
			PC++;

			ADD_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "ADD A, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region ADC A, n
		case 0x8F:
			ADC_N(A);
			
			PRINT_INSTRUCTION(disassembly, "ADC A, A");
			break;
		case 0x88:
			ADC_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADC A, B");
			break;
		case 0x89:
			ADC_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "ADC A, C");
			break;
		case 0x8A:
			ADC_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADC A, D");
			break;
		case 0x8B:
			ADC_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "ADC A, E");
			break;
		case 0x8C:
			ADC_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "ADC A, H");
			break;
		case 0x8D:
			ADC_N(HL.bytes.L);
			
			PRINT_INSTRUCTION(disassembly, "ADC A, L");
			break;
		case 0x8E:
			immValueB = MemoryRead(HL.word);
			ADC_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "ADC A, (HL)");
			break;
		case 0xCE:
			immValueB = MemoryRead(PC);
			PC++;

			ADC_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "ADC A, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region SUB n
		case 0x97:
			SUB_N(A);

			PRINT_INSTRUCTION(disassembly, "SUB A");
			break;
		case 0x90:
			SUB_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "SUB B");
			break;
		case 0x91:
			SUB_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "SUB C");
			break;
		case 0x92:
			SUB_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "SUB D");
			break;
		case 0x93:
			SUB_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "SUB E");
			break;
		case 0x94:
			SUB_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "SUB H");
			break;
		case 0x95:
			SUB_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "SUB L");
			break;
		case 0x96:
			immValueB = MemoryRead(HL.word);
			SUB_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "SUB (HL)");
			break;
		case 0xD6:
			immValueB = MemoryRead(PC);
			PC++;

			SUB_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "SUB 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region SBC A, n
		case 0x9F:
			SBC_N(A);

			PRINT_INSTRUCTION(disassembly, "SBC A, A");
			break;
		case 0x98:
			SBC_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "SBC A, B");
			break;
		case 0x99:
			SBC_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "SBC A, C");
			break;
		case 0x9A:
			SBC_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "SBC A, D");
			break;
		case 0x9B:
			SBC_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "SBC A, E");
			break;
		case 0x9C:
			SBC_N(HL.bytes.H);
			
			PRINT_INSTRUCTION(disassembly, "SBC A, H");
			break;
		case 0x9D:
			SBC_N(HL.bytes.L);
			
			PRINT_INSTRUCTION(disassembly, "SBC A, L");
			break;
		case 0x9E:
			immValueB = MemoryRead(HL.word);
			SBC_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "SBC A, (HL)");
			break;
		case 0xDE:
			immValueB = MemoryRead(PC);
			PC++;

			SBC_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "SBC A, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region AND n
		case 0xA7:
			AND_N(A);

			PRINT_INSTRUCTION(disassembly, "AND A");
			break;
		case 0xA0:
			AND_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "AND B");
			break;
		case 0xA1:
			AND_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "AND C");
			break;
		case 0xA2:
			AND_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "AND D");
			break;
		case 0xA3:
			AND_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "AND E");
			break;
		case 0xA4:
			AND_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "AND H");
			break;
		case 0xA5:
			AND_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "AND L");
			break;
		case 0xA6:
			immValueB = MemoryRead(HL.word);
			AND_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "AND (HL)");
			break;
		case 0xE6:
			immValueB = MemoryRead(PC);
			PC++;

			AND_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "AND 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region OR n
		case 0xB7:
			OR_N(A);

			PRINT_INSTRUCTION(disassembly, "OR A");
			break;
		case 0xB0:
			OR_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "OR B");
			break;
		case 0xB1:
			OR_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "OR C");
			break;
		case 0xB2:
			OR_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "OR D");
			break;
		case 0xB3:
			OR_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "OR E");
			break;
		case 0xB4:
			OR_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "OR H");
			break;
		case 0xB5:
			OR_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "OR L");
			break;
		case 0xB6:
			immValueB = MemoryRead(HL.word);
			OR_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "OR (HL)");
			break;
		case 0xF6:
			immValueB = MemoryRead(PC);
			PC++;

			OR_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "OR 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region XOR n
		case 0xAF:
			XOR_N(A);

			PRINT_INSTRUCTION(disassembly, "XOR A");
			break;
		case 0xA8:
			XOR_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "XOR B");
			break;
		case 0xA9:
			XOR_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "XOR C");
			break;
		case 0xAA:
			XOR_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "XOR D");
			break;
		case 0xAB:
			XOR_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "XOR E");
			break;
		case 0xAC:
			XOR_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "XOR H");
			break;
		case 0xAD:
			XOR_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "XOR L");
			break;
		case 0xAE:
			immValueB = MemoryRead(HL.word);
			XOR_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "XOR (HL)");
			break;
		case 0xEE:
			immValueB = MemoryRead(PC);
			PC++;

			XOR_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "XOR 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region CP n
		case 0xBF:
			CP_N(A);

			PRINT_INSTRUCTION(disassembly, "CP A");
			break;
		case 0xB8:
			CP_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "CP B");
			break;
		case 0xB9:
			CP_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "CP C");
			break;
		case 0xBA:
			CP_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "CP D");
			break;
		case 0xBB:
			CP_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "CP E");
			break;
		case 0xBC:
			CP_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "CP H");
			break;
		case 0xBD:
			CP_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "CP L");
			break;
		case 0xBE:
			immValueB = MemoryRead(HL.word);
			CP_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "CP (HL)");
			break;
		case 0xFE:
			immValueB = MemoryRead(PC);
			PC++;

			CP_N(immValueB);

			PRINT_INSTRUCTION(disassembly, "CP 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region INC n
		case 0x3C:
			INC_N(A);

			PRINT_INSTRUCTION(disassembly, "INC A");
			break;
		case 0x04:
			INC_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "INC B");
			break;
		case 0x0C:
			INC_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "INC C");
			break;
		case 0x14:
			INC_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "INC D");
			break;
		case 0x1C:
			INC_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "INC E");
			break;
		case 0x24:
			INC_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "INC H");
			break;
		case 0x2C:
			INC_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "INC L");
			break;
		case 0x34:
			immValueB = MemoryRead(HL.word);

			INC_N(immValueB);

			MemoryWrite(HL.word, immValueB);

			PRINT_INSTRUCTION(disassembly, "INC (HL)");
			break;
		#pragma endregion
		#pragma region DEC n
		case 0x3D:
			DEC_N(A);

			PRINT_INSTRUCTION(disassembly, "DEC A");
			break;
		case 0x05:
			DEC_N(BC.bytes.H);

			PRINT_INSTRUCTION(disassembly, "DEC B");
			break;
		case 0x0D:
			DEC_N(BC.bytes.L);

			PRINT_INSTRUCTION(disassembly, "DEC C");
			break;
		case 0x15:
			DEC_N(DE.bytes.H);

			PRINT_INSTRUCTION(disassembly, "DEC D");
			break;
		case 0x1D:
			DEC_N(DE.bytes.L);

			PRINT_INSTRUCTION(disassembly, "DEC E");
			break;
		case 0x25:
			DEC_N(HL.bytes.H);

			PRINT_INSTRUCTION(disassembly, "DEC H");
			break;
		case 0x2D:
			DEC_N(HL.bytes.L);

			PRINT_INSTRUCTION(disassembly, "DEC L");
			break;
		case 0x35:
			immValueB = MemoryRead(HL.word);

			DEC_N(immValueB);

			MemoryWrite(HL.word, immValueB);

			PRINT_INSTRUCTION(disassembly, "DEC (HL)");
			break;
		#pragma endregion
		#pragma region ADD HL, n
		case 0x09:
			ADD_HL(BC.word);
			
			PRINT_INSTRUCTION(disassembly, "ADD HL, BC");
			break;
		case 0x19:
			ADD_HL(DE.word);

			PRINT_INSTRUCTION(disassembly, "ADD HL, DE");
			break;
		case 0x29:
			ADD_HL(HL.word);

			PRINT_INSTRUCTION(disassembly, "ADD HL, HL");
			break;
		case 0x39:
			ADD_HL(SP);

			PRINT_INSTRUCTION(disassembly, "ADD HL, SP");
			break;
		#pragma endregion
		#pragma region ADD SP, n
		case 0xE8:
			immValueB = MemoryRead(PC);
			PC++;

			immValueW = SP + (signed char)immValueB;
			
			SET_FLAG_Z(0);
			SET_FLAG_N(0);
			SET_FLAG_H((immValueW & 0xF) < (SP & 0xF));
			SET_FLAG_C((immValueW & 0xFF) < (SP & 0xFF));

			SP = immValueW;

			SYNC_WITH_CPU(8);

			PRINT_INSTRUCTION(disassembly, "ADD SP, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region INC nn
		case 0x03:
			INC_NN(BC.word);
			
			PRINT_INSTRUCTION(disassembly, "INC BC");
			break;
		case 0x13:
			INC_NN(DE.word);
			
			PRINT_INSTRUCTION(disassembly, "INC DE");
			break;
		case 0x23:
			INC_NN(HL.word);

			PRINT_INSTRUCTION(disassembly, "INC HL");
			break;
		case 0x33:
			INC_NN(SP);

			PRINT_INSTRUCTION(disassembly, "INC SP");
			break;
		#pragma endregion
		#pragma region DEC nn
		case 0x0B:
			DEC_NN(BC.word);
			
			PRINT_INSTRUCTION(disassembly, "DEC BC");
			break;
		case 0x1B:
			DEC_NN(DE.word);

			PRINT_INSTRUCTION(disassembly, "DEC DE");
			break;
		case 0x2B:
			DEC_NN(HL.word);

			PRINT_INSTRUCTION(disassembly, "DEC HL");
			break;
		case 0x3B:
			DEC_NN(SP);

			PRINT_INSTRUCTION(disassembly, "DEC SP");
			break;
		#pragma endregion
		#pragma region DAA
		case 0x27:
			{
				int A = this->A;

				if (GET_FLAG_N() == 0)
				{
					if (GET_FLAG_H() != 0 || (A & 0xF) > 9)
					{
						A += 0x06;
					}
					if (GET_FLAG_C() != 0 || A > 0x9F) 
					{
						A += 0x60;
					}
				}
				else
				{
					if (GET_FLAG_H() != 0)
					{
						A = (A - 6) & 0xFF;
					}
					if (GET_FLAG_C() != 0)
					{
						A -= 0x60;
					}
				}

				SET_FLAG_H(0);
				if ((A & 0x100) == 0x100) 
				{
					SET_FLAG_C(1);
				}
				A &= 0xFF;
				SET_FLAG_Z(A == 0);
				this->A = A;
			}

			PRINT_INSTRUCTION(disassembly, "DAA");
			break;
		#pragma endregion
		#pragma region CPL
		case 0x2F:
			A = ~A;

			SET_FLAG_N(1);
			SET_FLAG_H(1);

			PRINT_INSTRUCTION(disassembly, "CPL");
			break;
		#pragma endregion
		#pragma region CCF
		case 0x3F:
			SET_FLAG_N(0);
			SET_FLAG_H(0);
			SET_FLAG_C(1 - GET_FLAG_C());

			PRINT_INSTRUCTION(disassembly, "CCF");
			break;
		#pragma endregion
		#pragma region SCF
		case 0x37:
			SET_FLAG_N(0);
			SET_FLAG_H(0);
			SET_FLAG_C(1);

			PRINT_INSTRUCTION(disassembly, "SCF");
			break;
		#pragma endregion
		#pragma region NOP
		case 0x00:
			PRINT_INSTRUCTION(disassembly, "NOP");
			break;
		#pragma endregion
		#pragma region HALT
		case 0x76:
			if (!Halted)
			{
				if (IME == 0 && (INT.GetIE() & INT.GetIF() & 0x1F))
				{
					HaltBug = true;
				}
				else
				{
					Halted = true;
					PC--;
				}

				PRINT_INSTRUCTION(disassembly, "HALT");
			}
			else
			{
				PC--;
			}

			break;
		#pragma endregion
		#pragma region STOP
		case 0x10:
			immValueB = MemoryRead(PC);
			//Ignoring STOP for now
			break;
		#pragma endregion
		#pragma region DI
		case 0xF3:
			//Delaying until next instruction is executed
			DIDelay = 2;

			PRINT_INSTRUCTION(disassembly, "DI");
			break;
		#pragma endregion
		#pragma region EI
		case 0xFB:
			//Delaying until next instruction is executed
			EIDelay = 2;

			PRINT_INSTRUCTION(disassembly, "EI");
			break;
		#pragma endregion
		#pragma region RLCA
		case 0x07:
			RLC_N(A);

			SET_FLAG_Z(0);

			PRINT_INSTRUCTION(disassembly, "RLCA");
			break;
		#pragma endregion
		#pragma region RLA
		case 0x17:
			RL_N(A);

			SET_FLAG_Z(0);

			PRINT_INSTRUCTION(disassembly, "RLA");
			break;
		#pragma endregion
		#pragma region RRCA
		case 0x0F:
			RRC_N(A);

			SET_FLAG_Z(0);

			PRINT_INSTRUCTION(disassembly, "RRCA");
			break;
		#pragma endregion
		#pragma region RRA
		case 0x1F:
			RR_N(A);

			SET_FLAG_Z(0);

			PRINT_INSTRUCTION(disassembly, "RRA");
			break;
		#pragma endregion
		#pragma region JP nn
		case 0xC3:
			immValueW = MemoryReadWord(PC);
			PC += 2;

			DelayedPCChange(immValueW);

			PRINT_INSTRUCTION(disassembly, "JP 0x%X", immValueW);
			break;
		#pragma endregion
		#pragma region JP cc, nn
		case 0xC2:
			JP_CC_NN(GET_FLAG_Z() == 0);
			
			PRINT_INSTRUCTION(disassembly, "JP NZ, 0x%X", immValueW);
			break;
		case 0xCA:
			JP_CC_NN(GET_FLAG_Z() == 1);
			
			PRINT_INSTRUCTION(disassembly, "JP Z, 0x%X", immValueW);
			break;
		case 0xD2:
			JP_CC_NN(GET_FLAG_C() == 0);
			
			PRINT_INSTRUCTION(disassembly, "JP NC, 0x%X", immValueW);
			break;
		case 0xDA:
			JP_CC_NN(GET_FLAG_C() == 1);
			
			PRINT_INSTRUCTION(disassembly, "JP C, 0x%X", immValueW);
			break;
		#pragma endregion
		#pragma region JP (HL)
		case 0xE9:
			PC = HL.word;

			PRINT_INSTRUCTION(disassembly, "JP (HL)");
			break;
		#pragma endregion
		#pragma region JR n
		case 0x18:
			immValueB = MemoryRead(PC);
			PC++;

			DelayedPCChange(PC + (signed char)immValueB);

			PRINT_INSTRUCTION(disassembly, "JR 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region JR cc, n
		case 0x20:
			JR_CC_NN(GET_FLAG_Z() == 0);
						
			PRINT_INSTRUCTION(disassembly, "JR NZ, 0x%X", immValueB);
			break;
		case 0x28:
			JR_CC_NN(GET_FLAG_Z() == 1);
			
			PRINT_INSTRUCTION(disassembly, "JR Z, 0x%X", immValueB);
			break;
		case 0x30:
			JR_CC_NN(GET_FLAG_C() == 0);
			
			PRINT_INSTRUCTION(disassembly, "JR NC, 0x%X", immValueB);
			break;
		case 0x38:
			JR_CC_NN(GET_FLAG_C() == 1);
			
			PRINT_INSTRUCTION(disassembly, "JR C, 0x%X", immValueB);
			break;
		#pragma endregion
		#pragma region CALL nn
		case 0xCD:
			immValueW = MemoryReadWord(PC);
			PC += 2;

			StackPushWord(PC);

			PC = immValueW;

			PRINT_INSTRUCTION(disassembly, "CALL 0x%X", immValueW);
			break;
		#pragma endregion
		#pragma region CALL cc, nn
		case 0xC4:
			CALL_CC_NN(GET_FLAG_Z() == 0);
			
			PRINT_INSTRUCTION(disassembly, "CALL NZ, 0x%X", immValueW);
			break;
		case 0xCC:
			CALL_CC_NN(GET_FLAG_Z() == 1);
			
			PRINT_INSTRUCTION(disassembly, "CALL Z, 0x%X", immValueW);
			break;
		case 0xD4:
			CALL_CC_NN(GET_FLAG_C() == 0);
			
			PRINT_INSTRUCTION(disassembly, "CALL NC, 0x%X", immValueW);
			break;
		case 0xDC:
			CALL_CC_NN(GET_FLAG_C() == 1);
			
			PRINT_INSTRUCTION(disassembly, "CALL C, 0x%X", immValueW);
			break;
		#pragma endregion
		#pragma region RST n
		case 0xC7:
			RST_N(0x00);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x00");
			break;
		case 0xCF:
			RST_N(0x08);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x08");
			break;
		case 0xD7:
			RST_N(0x10);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x10");
			break;
		case 0xDF:
			RST_N(0x18);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x18");
			break;
		case 0xE7:
			RST_N(0x20);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x20");
			break;
		case 0xEF:
			RST_N(0x28);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x28");
			break;
		case 0xF7:
			RST_N(0x30);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x30");
			break;
		case 0xFF:
			RST_N(0x38);
			
			PRINT_INSTRUCTION(disassembly, "RST 0x38");
			break;
		#pragma endregion
		#pragma region RET
		case 0xC9:
			DelayedPCChange(StackPopWord());
			
			PRINT_INSTRUCTION(disassembly, "RET");
			break;
		#pragma endregion
		#pragma region RET cc
		case 0xC0:
			RET_CC(GET_FLAG_Z() == 0);
			
			PRINT_INSTRUCTION(disassembly, "RET NZ");
			break;
		case 0xC8:
			RET_CC(GET_FLAG_Z() == 1);
			
			PRINT_INSTRUCTION(disassembly, "RET Z");
			break;
		case 0xD0:
			RET_CC(GET_FLAG_C() == 0);
			
			PRINT_INSTRUCTION(disassembly, "RET NC");
			break;
		case 0xD8:
			RET_CC(GET_FLAG_C() == 1);
			
			PRINT_INSTRUCTION(disassembly, "RET C");
			break;
		#pragma endregion
		#pragma region RETI
		case 0xD9:
			DelayedPCChange(StackPopWord());
			IME = 1;

			PRINT_INSTRUCTION(disassembly, "RETI");
			break;
		#pragma endregion
		#pragma region CB prefix
		case 0xCB:
			opcode = MemoryRead(PC);
			PC++;

			switch (opcode)
			{
				#pragma region SWAP n
				case 0x37:
					SWAP_N(A);

					PRINT_INSTRUCTION(disassembly, "SWAP A");
					break;
				case 0x30:
					SWAP_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SWAP B");
					break;
				case 0x31:
					SWAP_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SWAP C");
					break;
				case 0x32:
					SWAP_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SWAP D");
					break;
				case 0x33:
					SWAP_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SWAP E");
					break;
				case 0x34:
					SWAP_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SWAP H");
					break;
				case 0x35:
					SWAP_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SWAP L");
					break;
				case 0x36:
					immValueB = MemoryRead(HL.word);
	
					SWAP_N(immValueB);

					MemoryWrite(HL.word, immValueB);

					PRINT_INSTRUCTION(disassembly, "SWAP (HL)");
					break;
				#pragma endregion
				#pragma region RLC n
				case 0x07:
					RLC_N(A);

					PRINT_INSTRUCTION(disassembly, "RLC A");
					break;
				case 0x00:
					RLC_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RLC B");
					break;
				case 0x01:
					RLC_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RLC C");
					break;
				case 0x02:
					RLC_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RLC D");
					break;
				case 0x03:
					RLC_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RLC E");
					break;
				case 0x04:
					RLC_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RLC H");
					break;
				case 0x05:
					RLC_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RLC H");
					break;
				case 0x06:
					immValueB = MemoryRead(HL.word);

					RLC_N(immValueB);

					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RLC (HL)");
					break;
				#pragma endregion
				#pragma region RL n
				case 0x17:
					RL_N(A);

					PRINT_INSTRUCTION(disassembly, "RL A");
					break;
				case 0x10:
					RL_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RL B");
					break;
				case 0x11:
					RL_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RL C");
					break;
				case 0x12:
					RL_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RL D");
					break;
				case 0x13:
					RL_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RL E");
					break;
				case 0x14:
					RL_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RL H");
					break;
				case 0x15:
					RL_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RL L");
					break;
				case 0x16:
					immValueB = MemoryRead(HL.word);	

					RL_N(immValueB);
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RL (HL)");
					break;
				#pragma endregion
				#pragma region RRC n
				case 0x0F:
					RRC_N(A);

					PRINT_INSTRUCTION(disassembly, "RRC A");
					break;
				case 0x08:
					RRC_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RRC B");
					break;
				case 0x09:
					RRC_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RRC C");
					break;
				case 0x0A:
					RRC_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RRC D");
					break;
				case 0x0B:
					RRC_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RRC E");
					break;
				case 0x0C:
					RRC_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RRC H");
					break;
				case 0x0D:
					RRC_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RRC H");
					break;
				case 0x0E:
					immValueB = MemoryRead(HL.word);
					
					RRC_N(immValueB);
					
					MemoryWrite(HL.word, immValueB);
						
					PRINT_INSTRUCTION(disassembly, "RRC (HL)");
					break;
				#pragma endregion
				#pragma region RR n
				case 0x1F:
					RR_N(A);

					PRINT_INSTRUCTION(disassembly, "RR A");
					break;
				case 0x18:
					RR_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RR B");
					break;
				case 0x19:
					RR_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RR C");
					break;
				case 0x1A:
					RR_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RR D");
					break;
				case 0x1B:
					RR_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RR E");
					break;
				case 0x1C:
					RR_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "RR H");
					break;
				case 0x1D:
					RR_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "RR H");
					break;
				case 0x1E:
					immValueB = MemoryRead(HL.word);

					RR_N(immValueB);

					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RR (HL)");
					break;
				#pragma endregion
				#pragma region SLA n
				case 0x27:
					SLA_N(A);

					PRINT_INSTRUCTION(disassembly, "SLA A");
					break;
				case 0x20:
					SLA_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SLA B");
					break;
				case 0x21:
					SLA_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SLA C");
					break;
				case 0x22:
					SLA_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SLA D");
					break;
				case 0x23:
					SLA_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SLA E");
					break;
				case 0x24:
					SLA_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SLA H");
					break;
				case 0x25:
					SLA_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SLA H");
					break;
				case 0x26:
					immValueB = MemoryRead(HL.word);
					
					SLA_N(immValueB);
										
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SLA (HL)");
					break;
				#pragma endregion
				#pragma region SRA n
				case 0x2F:
					SRA_N(A);

					PRINT_INSTRUCTION(disassembly, "SRA A");
					break;
				case 0x28:
					SRA_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRA B");
					break;
				case 0x29:
					SRA_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRA C");
					break;
				case 0x2A:
					SRA_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRA D");
					break;
				case 0x2B:
					SRA_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRA E");
					break;
				case 0x2C:
					SRA_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRA H");
					break;
				case 0x2D:
					SRA_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRA H");
					break;
				case 0x2E:
					immValueB = MemoryRead(HL.word);
					
					SRA_N(immValueB);

					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SRA (HL)");
					break;
				#pragma endregion
				#pragma region SRL n
				case 0x3F:
					SRL_N(A);

					PRINT_INSTRUCTION(disassembly, "SRL A");
					break;
				case 0x38:
					SRL_N(BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRL B");
					break;
				case 0x39:
					SRL_N(BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRL C");
					break;
				case 0x3A:
					SRL_N(DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRL D");
					break;
				case 0x3B:
					SRL_N(DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRL E");
					break;
				case 0x3C:
					SRL_N(HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "SRL H");
					break;
				case 0x3D:
					SRL_N(HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "SRL H");
					break;
				case 0x3E:
					immValueB = MemoryRead(HL.word);
					
					SRL_N(immValueB);

					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SRL (HL)");
					break;
				#pragma endregion
				#pragma region BIT 0, n
				case 0x47:
					BITX_N(0, A);

					PRINT_INSTRUCTION(disassembly, "BIT 0, A");
					break;
				case 0x40:
					BITX_N(0, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 0, B");
					break;
				case 0x41:
					BITX_N(0, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 0, C");
					break;
				case 0x42:
					BITX_N(0, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 0, D");
					break;
				case 0x43:
					BITX_N(0, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 0, E");
					break;
				case 0x44:
					BITX_N(0, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 0, H");
					break;
				case 0x45:
					BITX_N(0, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 0, L");
					break;
				case 0x46:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(0, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "BIT 0, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 1, n
				case 0x4F:
					BITX_N(1, A);

					PRINT_INSTRUCTION(disassembly, "BIT 1, A");
					break;
				case 0x48:
					BITX_N(1, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 1, B");
					break;
				case 0x49:
					BITX_N(1, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 1, C");
					break;
				case 0x4A:
					BITX_N(1, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 1, D");
					break;
				case 0x4B:
					BITX_N(1, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 1, E");
					break;
				case 0x4C:
					BITX_N(1, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 1, H");
					break;
				case 0x4D:
					BITX_N(1, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 1, L");
					break;
				case 0x4E:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(1, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 1, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 2, n
				case 0x57:
					BITX_N(2, A);

					PRINT_INSTRUCTION(disassembly, "BIT 2, A");
					break;
				case 0x50:
					BITX_N(2, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 2, B");
					break;
				case 0x51:
					BITX_N(2, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 2, C");
					break;
				case 0x52:
					BITX_N(2, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 2, D");
					break;
				case 0x53:
					BITX_N(2, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 2, E");
					break;
				case 0x54:
					BITX_N(2, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 2, H");
					break;
				case 0x55:
					BITX_N(2, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 2, L");
					break;
				case 0x56:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(2, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 2, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 3, n
				case 0x5F:
					BITX_N(3, A);

					PRINT_INSTRUCTION(disassembly, "BIT 3, A");
					break;
				case 0x58:
					BITX_N(3, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 3, B");
					break;
				case 0x59:
					BITX_N(3, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 3, C");
					break;
				case 0x5A:
					BITX_N(3, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 3, D");
					break;
				case 0x5B:
					BITX_N(3, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 3, E");
					break;
				case 0x5C:
					BITX_N(3, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 3, H");
					break;
				case 0x5D:
					BITX_N(3, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 3, L");
					break;
				case 0x5E:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(3, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 3, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 4, n
				case 0x67:
					BITX_N(4, A);

					PRINT_INSTRUCTION(disassembly, "BIT 4, A");
					break;
				case 0x60:
					BITX_N(4, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 4, B");
					break;
				case 0x61:
					BITX_N(4, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 4, C");
					break;
				case 0x62:
					BITX_N(4, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 4, D");
					break;
				case 0x63:
					BITX_N(4, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 4, E");
					break;
				case 0x64:
					BITX_N(4, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 4, H");
					break;
				case 0x65:
					BITX_N(4, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 4, L");
					break;
				case 0x66:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(4, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 4, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 5, n
				case 0x6F:
					BITX_N(5, A);

					PRINT_INSTRUCTION(disassembly, "BIT 5, A");
					break;
				case 0x68:
					BITX_N(5, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 5, B");
					break;
				case 0x69:
					BITX_N(5, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 5, C");
					break;
				case 0x6A:
					BITX_N(5, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 5, D");
					break;
				case 0x6B:
					BITX_N(5, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 5, E");
					break;
				case 0x6C:
					BITX_N(5, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 5, H");
					break;
				case 0x6D:
					BITX_N(5, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 5, L");
					break;
				case 0x6E:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(5, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 5, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 6, n
				case 0x77:
					BITX_N(6, A);

					PRINT_INSTRUCTION(disassembly, "BIT 6, A");
					break;
				case 0x70:
					BITX_N(6, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 6, B");
					break;
				case 0x71:
					BITX_N(6, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 6, C");
					break;
				case 0x72:
					BITX_N(6, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 6, D");
					break;
				case 0x73:
					BITX_N(6, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 6, E");
					break;
				case 0x74:
					BITX_N(6, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 6, H");
					break;
				case 0x75:
					BITX_N(6, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 6, L");
					break;
				case 0x76:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(6, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 6, (HL)");
					break;
				#pragma endregion
				#pragma region BIT 7, n
				case 0x7F:
					BITX_N(7, A);

					PRINT_INSTRUCTION(disassembly, "BIT 7, A");
					break;
				case 0x78:
					BITX_N(7, BC.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 7, B");
					break;
				case 0x79:
					BITX_N(7, BC.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 7, C");
					break;
				case 0x7A:
					BITX_N(7, DE.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 7, D");
					break;
				case 0x7B:
					BITX_N(7, DE.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 7, E");
					break;
				case 0x7C:
					BITX_N(7, HL.bytes.H);

					PRINT_INSTRUCTION(disassembly, "BIT 7, H");
					break;
				case 0x7D:
					BITX_N(7, HL.bytes.L);

					PRINT_INSTRUCTION(disassembly, "BIT 7, L");
					break;
				case 0x7E:
					immValueB = MemoryRead(HL.word);
					
					BITX_N(7, immValueB);

					PRINT_INSTRUCTION(disassembly, "BIT 7, (HL)");
					break;
				#pragma endregion
				#pragma region SET 0, n
				case 0xC7:
					A |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, A");
					break;
				case 0xC0:
					BC.bytes.H |= 0x1;
					
					PRINT_INSTRUCTION(disassembly, "SET 0, B");
					break;
				case 0xC1:
					BC.bytes.L |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, C");
					break;
				case 0xC2:
					DE.bytes.H |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, D");
					break;
				case 0xC3:
					DE.bytes.L |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, E");
					break;
				case 0xC4:
					HL.bytes.H |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, H");
					break;
				case 0xC5:
					HL.bytes.L |= 0x1;

					PRINT_INSTRUCTION(disassembly, "SET 0, L");
					break;
				case 0xC6:
					immValueB = MemoryRead(HL.word) | 0x1;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 0, (HL)");
					break;
				#pragma endregion
				#pragma region SET 1, n
				case 0xCF:
					A |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, A");
					break;
				case 0xC8:
					BC.bytes.H |= 0x2;
					
					PRINT_INSTRUCTION(disassembly, "SET 1, B");
					break;
				case 0xC9:
					BC.bytes.L |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, C");
					break;
				case 0xCA:
					DE.bytes.H |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, D");
					break;
				case 0xCB:
					DE.bytes.L |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, E");
					break;
				case 0xCC:
					HL.bytes.H |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, H");
					break;
				case 0xCD:
					HL.bytes.L |= 0x2;

					PRINT_INSTRUCTION(disassembly, "SET 1, L");
					break;
				case 0xCE:
					immValueB = MemoryRead(HL.word) | 0x2;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 1, (HL)");
					break;
				#pragma endregion
				#pragma region SET 2, n
				case 0xD7:
					A |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, A");
					break;
				case 0xD0:
					BC.bytes.H |= 0x4;
					
					PRINT_INSTRUCTION(disassembly, "SET 2, B");
					break;
				case 0xD1:
					BC.bytes.L |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, C");
					break;
				case 0xD2:
					DE.bytes.H |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, D");
					break;
				case 0xD3:
					DE.bytes.L |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, E");
					break;
				case 0xD4:
					HL.bytes.H |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, H");
					break;
				case 0xD5:
					HL.bytes.L |= 0x4;

					PRINT_INSTRUCTION(disassembly, "SET 2, L");
					break;
				case 0xD6:
					immValueB = MemoryRead(HL.word) | 0x4;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 2, (HL)");
					break;
				#pragma endregion
				#pragma region SET 3, n
				case 0xDF:
					A |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, A");
					break;
				case 0xD8:
					BC.bytes.H |= 0x8;
					
					PRINT_INSTRUCTION(disassembly, "SET 3, B");
					break;
				case 0xD9:
					BC.bytes.L |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, C");
					break;
				case 0xDA:
					DE.bytes.H |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, D");
					break;
				case 0xDB:
					DE.bytes.L |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, E");
					break;
				case 0xDC:
					HL.bytes.H |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, H");
					break;
				case 0xDD:
					HL.bytes.L |= 0x8;

					PRINT_INSTRUCTION(disassembly, "SET 3, L");
					break;
				case 0xDE:
					immValueB = MemoryRead(HL.word) | 0x8;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 3, (HL)");
					break;
				#pragma endregion
				#pragma region SET 4, n
				case 0xE7:
					A |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, A");
					break;
				case 0xE0:
					BC.bytes.H |= 0x10;
					
					PRINT_INSTRUCTION(disassembly, "SET 4, B");
					break;
				case 0xE1:
					BC.bytes.L |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, C");
					break;
				case 0xE2:
					DE.bytes.H |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, D");
					break;
				case 0xE3:
					DE.bytes.L |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, E");
					break;
				case 0xE4:
					HL.bytes.H |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, H");
					break;
				case 0xE5:
					HL.bytes.L |= 0x10;

					PRINT_INSTRUCTION(disassembly, "SET 4, L");
					break;
				case 0xE6:
					immValueB = MemoryRead(HL.word) | 0x10;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 4, (HL)");
					break;
				#pragma endregion
				#pragma region SET 5, n
				case 0xEF:
					A |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, A");
					break;
				case 0xE8:
					BC.bytes.H |= 0x20;
					
					PRINT_INSTRUCTION(disassembly, "SET 5, B");
					break;
				case 0xE9:
					BC.bytes.L |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, C");
					break;
				case 0xEA:
					DE.bytes.H |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, D");
					break;
				case 0xEB:
					DE.bytes.L |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, E");
					break;
				case 0xEC:
					HL.bytes.H |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, H");
					break;
				case 0xED:
					HL.bytes.L |= 0x20;

					PRINT_INSTRUCTION(disassembly, "SET 5, L");
					break;
				case 0xEE:
					immValueB = MemoryRead(HL.word) | 0x20;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 5, (HL)");
					break;
				#pragma endregion
				#pragma region SET 6, n
				case 0xF7:
					A |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, A");
					break;
				case 0xF0:
					BC.bytes.H |= 0x40;
					
					PRINT_INSTRUCTION(disassembly, "SET 6, B");
					break;
				case 0xF1:
					BC.bytes.L |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, C");
					break;
				case 0xF2:
					DE.bytes.H |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, D");
					break;
				case 0xF3:
					DE.bytes.L |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, E");
					break;
				case 0xF4:
					HL.bytes.H |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, H");
					break;
				case 0xF5:
					HL.bytes.L |= 0x40;

					PRINT_INSTRUCTION(disassembly, "SET 6, L");
					break;
				case 0xF6:
					immValueB = MemoryRead(HL.word) | 0x40;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 6, (HL)");
					break;
				#pragma endregion
				#pragma region SET 7, n
				case 0xFF:
					A |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, A");
					break;
				case 0xF8:
					BC.bytes.H |= 0x80;
					
					PRINT_INSTRUCTION(disassembly, "SET 7, B");
					break;
				case 0xF9:
					BC.bytes.L |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, C");
					break;
				case 0xFA:
					DE.bytes.H |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, D");
					break;
				case 0xFB:
					DE.bytes.L |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, E");
					break;
				case 0xFC:
					HL.bytes.H |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, H");
					break;
				case 0xFD:
					HL.bytes.L |= 0x80;

					PRINT_INSTRUCTION(disassembly, "SET 7, L");
					break;
				case 0xFE:
					immValueB = MemoryRead(HL.word) | 0x80;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "SET 7, (HL)");
					break;
				#pragma endregion
				#pragma region RES 0, n
				case 0x87:
					A &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, A");
					break;
				case 0x80:
					BC.bytes.H &= 0xFE;
					
					PRINT_INSTRUCTION(disassembly, "RES 0, B");
					break;
				case 0x81:
					BC.bytes.L &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, C");
					break;
				case 0x82:
					DE.bytes.H &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, D");
					break;
				case 0x83:
					DE.bytes.L &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, E");
					break;
				case 0x84:
					HL.bytes.H &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, H");
					break;
				case 0x85:
					HL.bytes.L &= 0xFE;

					PRINT_INSTRUCTION(disassembly, "RES 0, L");
					break;
				case 0x86:
					immValueB = MemoryRead(HL.word) & 0xFE;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 0, (HL)");
					break;
				#pragma endregion
				#pragma region RES 1, n
				case 0x8F:
					A &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, A");
					break;
				case 0x88:
					BC.bytes.H &= 0xFD;
					
					PRINT_INSTRUCTION(disassembly, "RES 1, B");
					break;
				case 0x89:
					BC.bytes.L &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, C");
					break;
				case 0x8A:
					DE.bytes.H &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, D");
					break;
				case 0x8B:
					DE.bytes.L &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, E");
					break;
				case 0x8C:
					HL.bytes.H &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, H");
					break;
				case 0x8D:
					HL.bytes.L &= 0xFD;

					PRINT_INSTRUCTION(disassembly, "RES 1, L");
					break;
				case 0x8E:
					immValueB = MemoryRead(HL.word) & 0xFD;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 1, (HL)");
					break;
				#pragma endregion
				#pragma region RES 2, n
				case 0x97:
					A &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, A");
					break;
				case 0x90:
					BC.bytes.H &= 0xFB;
					
					PRINT_INSTRUCTION(disassembly, "RES 2, B");
					break;
				case 0x91:
					BC.bytes.L &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, C");
					break;
				case 0x92:
					DE.bytes.H &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, D");
					break;
				case 0x93:
					DE.bytes.L &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, E");
					break;
				case 0x94:
					HL.bytes.H &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, H");
					break;
				case 0x95:
					HL.bytes.L &= 0xFB;

					PRINT_INSTRUCTION(disassembly, "RES 2, L");
					break;
				case 0x96:
					immValueB = MemoryRead(HL.word) & 0xFB;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 2, (HL)");
					break;
				#pragma endregion
				#pragma region RES 3, n
				case 0x9F:
					A &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, A");
					break;
				case 0x98:
					BC.bytes.H &= 0xF7;
					
					PRINT_INSTRUCTION(disassembly, "RES 3, B");
					break;
				case 0x99:
					BC.bytes.L &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, C");
					break;
				case 0x9A:
					DE.bytes.H &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, D");
					break;
				case 0x9B:
					DE.bytes.L &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, E");
					break;
				case 0x9C:
					HL.bytes.H &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, H");
					break;
				case 0x9D:
					HL.bytes.L &= 0xF7;

					PRINT_INSTRUCTION(disassembly, "RES 3, L");
					break;
				case 0x9E:
					immValueB = MemoryRead(HL.word) & 0xF7;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 3, (HL)");
					break;
				#pragma endregion
				#pragma region RES 4, n
				case 0xA7:
					A &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, A");
					break;
				case 0xA0:
					BC.bytes.H &= 0xEF;
					
					PRINT_INSTRUCTION(disassembly, "RES 4, B");
					break;
				case 0xA1:
					BC.bytes.L &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, C");
					break;
				case 0xA2:
					DE.bytes.H &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, D");
					break;
				case 0xA3:
					DE.bytes.L &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, E");
					break;
				case 0xA4:
					HL.bytes.H &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, H");
					break;
				case 0xA5:
					HL.bytes.L &= 0xEF;

					PRINT_INSTRUCTION(disassembly, "RES 4, L");
					break;
				case 0xA6:
					immValueB = MemoryRead(HL.word) & 0xEF;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 4, (HL)");
					break;
				#pragma endregion
				#pragma region RES 5, n
				case 0xAF:
					A &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, A");
					break;
				case 0xA8:
					BC.bytes.H &= 0xDF;
					
					PRINT_INSTRUCTION(disassembly, "RES 5, B");
					break;
				case 0xA9:
					BC.bytes.L &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, C");
					break;
				case 0xAA:
					DE.bytes.H &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, D");
					break;
				case 0xAB:
					DE.bytes.L &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, E");
					break;
				case 0xAC:
					HL.bytes.H &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, H");
					break;
				case 0xAD:
					HL.bytes.L &= 0xDF;

					PRINT_INSTRUCTION(disassembly, "RES 5, L");
					break;
				case 0xAE:
					immValueB = MemoryRead(HL.word) & 0xDF;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 5, (HL)");
					break;
				#pragma endregion
				#pragma region RES 6, n
				case 0xB7:
					A &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, A");
					break;
				case 0xB0:
					BC.bytes.H &= 0xBF;
					
					PRINT_INSTRUCTION(disassembly, "RES 6, B");
					break;
				case 0xB1:
					BC.bytes.L &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, C");
					break;
				case 0xB2:
					DE.bytes.H &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, D");
					break;
				case 0xB3:
					DE.bytes.L &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, E");
					break;
				case 0xB4:
					HL.bytes.H &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, H");
					break;
				case 0xB5:
					HL.bytes.L &= 0xBF;

					PRINT_INSTRUCTION(disassembly, "RES 6, L");
					break;
				case 0xB6:
					immValueB = MemoryRead(HL.word) & 0xBF;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 6, (HL)");
					break;
				#pragma endregion
				#pragma region RES 7, n
				case 0xBF:
					A &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, A");
					break;
				case 0xB8:
					BC.bytes.H &= 0x7F;
					
					PRINT_INSTRUCTION(disassembly, "RES 7, B");
					break;
				case 0xB9:
					BC.bytes.L &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, C");
					break;
				case 0xBA:
					DE.bytes.H &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, D");
					break;
				case 0xBB:
					DE.bytes.L &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, E");
					break;
				case 0xBC:
					HL.bytes.H &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, H");
					break;
				case 0xBD:
					HL.bytes.L &= 0x7F;

					PRINT_INSTRUCTION(disassembly, "RES 7, L");
					break;
				case 0xBE:
					immValueB = MemoryRead(HL.word) & 0x7F;
					
					MemoryWrite(HL.word, immValueB);
					
					PRINT_INSTRUCTION(disassembly, "RES 7, (HL)");
					break;
				#pragma endregion
			}

			opcode |= 0xCB00;

			break;
		#pragma endregion

		default:
			PRINT_INSTRUCTION(disassembly, "UNKNOWN INSTRUCTION");
			break;
	}

#ifdef DEBUG_PRINTCPU
	printf(disassembly);
	printf("\n");
#endif

	INT.Step(*this);
}

BYTE Cookieboy::CPU::MemoryRead(WORD addr)
{
	BYTE value = MMU.Read(addr);
	SYNC_WITH_CPU(4);

	return value;
}

WORD Cookieboy::CPU::MemoryReadWord(WORD addr)
{
	WORD value = MemoryRead(addr);
	value |= MemoryRead(addr + 1) << 8;

	return value;
}

void Cookieboy::CPU::MemoryWrite(WORD addr, BYTE value)
{
	MMU.Write(addr, value);
	SYNC_WITH_CPU(4);
}

void Cookieboy::CPU::MemoryWriteWord(WORD addr, WORD value)
{
	MemoryWrite(addr, value & 0xFF);
	MemoryWrite(addr + 1, (value & 0xFF00) >> 8);
}

void Cookieboy::CPU::DelayedPCChange(WORD value)
{
	PC = value;

	SYNC_WITH_CPU(4);
}

void Cookieboy::CPU::INTJump(WORD address)
{
	PC = address;

	SYNC_WITH_CPU(8);
}

void Cookieboy::CPU::StackPushByte(BYTE value)
{
	SP--;
	MemoryWrite(SP, value);
}

void Cookieboy::CPU::StackPushWord(WORD value)
{
	StackPushByte((value >> 8) & 0xFF);
	StackPushByte(value & 0xFF);

	SYNC_WITH_CPU(4);
}

BYTE Cookieboy::CPU::StackPopByte()
{
	BYTE result = MemoryRead(SP);
	SP++;

	return result;
}

WORD Cookieboy::CPU::StackPopWord()
{
	WORD result = StackPopByte();
	result |= StackPopByte() << 8;

	return result;
}