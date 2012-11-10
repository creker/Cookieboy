#ifndef COOKIEBOYCPU_H
#define COOKIEBOYCPU_H

#include "CookieboyDefs.h"

//#define DEBUG_PRINTCPU

#ifdef DEBUG_PRINTCPU
	#define PRINT_INSTRUCTION sprintf_s
#else
	#define PRINT_INSTRUCTION(...)
#endif

namespace Cookieboy
{

class Memory;
class GPU;
class DividerTimer;
class TIMATimer;
class Joypad;
class Sound;
class SerialIO;
class Interrupts;

/*
The GameBoy uses a computer chip similar to an Intel 8080. It contains all of the instructions of an 8080
except there are no exchange instructions. In many ways the processor is more similar to the Zilog Z80
processor. Compared to the Z80, some instructions have been added and some have been taken away
*/
class CPU
{
public:
	/*
	Needed for combined registers i.e. BC, DE
	Provides easy way to access individual bytes as well as whole 16-bit register
	*/
	union WordRegister
	{
		struct
		{
			BYTE L;
			BYTE H;
		} bytes;
		WORD word;
	};

	CPU(const bool &CGB, bool &_CGBDoubleSpeed, Memory &MMU, GPU &GPU, DividerTimer &DIV, TIMATimer &TIMA, Joypad &joypad, Sound &sound, SerialIO &serial, Interrupts &INT);

	void Step();

	void Reset();
	void EmulateBIOS();

	friend class Interrupts;

private:

	BYTE MemoryRead(WORD addr);
	WORD MemoryReadWord(WORD addr);
	void MemoryWrite(WORD addr, BYTE value);
	void MemoryWriteWord(WORD addr, WORD value);

	void INTJump(WORD address);

	//CPU Microcode
	void StackPushByte(BYTE value);
	void StackPushWord(WORD value);
	BYTE StackPopByte();
	WORD StackPopWord();

	const bool &CGB;
	bool &CGBDoubleSpeed;

	//CPU registers
	BYTE A;
	BYTE F;//Flags register - ZNHC0000
	WordRegister BC;
	WordRegister DE;
	WordRegister HL;
	WORD PC;
	WORD SP;
	BYTE IME;//Interrupt master enable register

	bool Halted;
	bool HaltBug;

	BYTE DIDelay;
	BYTE EIDelay;

	Memory &MMU;
	GPU &GPU;
	DividerTimer &DIV;
	TIMATimer &TIMA;
	Joypad &Joypad;
	Sound &Sound;
	SerialIO &Serial;
	Interrupts &INT;
};

}

#endif