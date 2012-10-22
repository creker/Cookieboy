#ifndef COOKIEBOYINTERRUPTS_H
#define COOKIEBOYINTERRUPTS_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

class CPU;
class Memory;

/*
The IME (interrupt master enable) flag is reset by DI and prohibits all interrupts. It is set by EI and
acknowledges the interrupt setting by the IE register.

1. When an interrupt is generated, the IF flag will be set.
2. If the IME flag is set & the corresponding IE flag is set, the following 3 steps are performed.
3. Reset the IME flag and prevent all interrupts.
4. The PC (program counter) is pushed onto the stack.
5. Jump to the starting address of the interrupt.
*/
class Interrupts
{
public:
	enum InterruptsEnum
	{
		INTERRUPT_VBLANK =	0x01,
		INTERRUPT_LCDC =	0x02,
		INTERRUPT_TIMA =	0x04,
		INTERRUPT_SERIAL =	0x08,
		INTERRUPT_JOYPAD =	0x10
	};

	Interrupts() { Reset(); }

	void Step(CPU &CPU);

	void Reset() { IF = 0; IE = 0; }
	void EmulateBIOS() { Reset(); }

	void Request(InterruptsEnum INT) { IF |= INT; }

	void IFChanged(BYTE value) { IF = value; }
	void IEChanged(BYTE value) { IE = value; }

	BYTE GetIF() { return IF | 0xE0; }
	BYTE GetIE() { return IE | 0xE0; }

private:
	BYTE IF;//Interrupt Flag (R/W)
			//Bit 4: Transition from High to Low of Pin number P10-P13
			//Bit 3: Serial I/O transfer complete
			//Bit 2: Timer Overflow
			//Bit 1: LCDC (see STAT)
			//Bit 0: V-Blank

	BYTE IE;//Interrupt Enable (R/W)
			//Bit 4: Transition from High to Low of Pin number P10-P13.
			//Bit 3: Serial I/O transfer complete
			//Bit 2: Timer Overflow
			//Bit 1: LCDC (see STAT)
			//Bit 0: V-Blank
			//
			//0: disable
			//1: enable
};

}

#endif