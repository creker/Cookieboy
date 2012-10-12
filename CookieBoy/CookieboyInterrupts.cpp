#include "CookieboyInterrupts.h"
#include "CookieboyCPU.h"

/*
Interrupt			Priority	Start Address
V-Blank				1			$0040
LCDC Status			2			$0048 - Modes 0, 1, 2, LYC=LY coincide (selectable)
Timer Overflow		3			$0050
Serial Transfer		4			$0058 - when transfer is complete
Hi-Lo of P10-P13	5			$0060
*/
void Cookieboy::Interrupts::Step(Cookieboy::CPU &CPU)
{
	//We shouldn't check IME here
	if ((IE & IF) & 0x1F)
	{
		//Interrupt disables HALT even if it's not going to be serviced because of the IME
		if (CPU.Halted)
		{
			CPU.Halted = false;
			CPU.PC++;
		}

		//Now we check IME to actually service interrupt. Jump to interrupt vector takes 20 cycles
		if (CPU.IME)
		{
			CPU.IME = 0;

			CPU.StackPushWord(CPU.PC);

			if ((IF & INTERRUPT_VBLANK) && (IE & INTERRUPT_VBLANK))//V-blank interrupt
			{
				IF &= ~INTERRUPT_VBLANK;

				CPU.INTJump(0x40);
			}
			else if ((IF & INTERRUPT_LCDC) && (IE & INTERRUPT_LCDC))//LCDC status
			{
				IF &= ~INTERRUPT_LCDC;

				CPU.INTJump(0x48);
			}
			else if ((IF & INTERRUPT_TIMA) && (IE & INTERRUPT_TIMA))//Timer overflow
			{
				IF &= ~INTERRUPT_TIMA;

				CPU.INTJump(0x50);
			}
			else if ((IF & INTERRUPT_SERIAL) && (IE & INTERRUPT_SERIAL))//Serial transfer
			{
				IF &= ~INTERRUPT_SERIAL;

				CPU.INTJump(0x58);
			}
			else if ((IF & INTERRUPT_JOYPAD) && (IE & INTERRUPT_JOYPAD))//Joypad
			{
				IF &= ~INTERRUPT_JOYPAD;

				CPU.INTJump(0x60);
			}
		}
	}
}