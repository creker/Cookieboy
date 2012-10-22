#ifndef COOKIEBOYJOYPAD_H
#define COOKIEBOYJOYPAD_H

#include "CookieboyDefs.h"
#include <memory.h>

namespace Cookieboy
{

class Interrupts;

class Joypad
{
public:
	struct ButtonsState
	{
		BYTE A;
		BYTE B;
		BYTE start;
		BYTE select;
		BYTE left;
		BYTE right;
		BYTE up;
		BYTE down;
	};

	Joypad() { Reset(); }

	void Step(Interrupts &INT);
	void UpdateJoypad(ButtonsState &buttons) { Buttons = buttons; }

	void Reset() { P1 = 0; memset(&Buttons, 0, sizeof(Buttons)); }

	void EmulateBIOS() { Reset(); }

	void P1Changed(BYTE value) { P1 = value; }
	BYTE GetP1() { return P1 | 0xC0; }

private:

	BYTE P1;//Register for reading joy pad info and determining system type
			//Bit 7 - Not used
			//Bit 6 - Not used
			//Bit 5 - P15 out port
			//Bit 4 - P14 out port
			//Bit 3 - P13 in port
			//Bit 2 - P12 in port
			//Bit 1 - P11 in port
			//Bit 0 - P10 in port
			//
			//		P14			P15
			//		|			|
			//P10-----O-Right-----O-A
			//		|			|
			//P11-----O-Left------O-B
			//		|			|
			//P12-----O-Up--------O-Select
			//		|			|
			//P13-----O-Down------O-Start
			//		|			|

	ButtonsState Buttons;
};

}

#endif