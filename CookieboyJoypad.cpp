#include "CookieboyJoypad.h"
#include "CookieboyInterrupts.h"

void Cookieboy::Joypad::Step(Interrupts &INT)
{
	P1 |= 0xF;//All keys released

	switch (P1 & 0x30)
	{
		//P14 out port
		case 0x20:
			//If key pressed then in port will be 0
			P1 &= ~Buttons.right;			//P10 - right
			P1 &= ~(Buttons.left << 1);		//P11 - left
			P1 &= ~(Buttons.up << 2);		//P12 - up
			P1 &= ~(Buttons.down << 3);		//P13 - down
			break;

		//P15 out port
		case 0x10:
			P1 &= ~Buttons.A;				//P10 - A
			P1 &= ~(Buttons.B << 1);		//P11 - B
			P1 &= ~(Buttons.select << 2);	//P12 - select
			P1 &= ~(Buttons.start << 3);	//P13 - start
			break;
	}

	if ((P1 & 0xF) != 0xF)
	{
		INT.Request(Interrupts::INTERRUPT_JOYPAD);
	}
}