#ifndef COOKIEBOYSERIALIO_H
#define COOKIEBOYSERIALIO_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

class Interrupts;

class SerialIO
{
public:
	SerialIO(const bool &_CGB, const bool &_CGBDoubleSpeed) : CGB(_CGB), CGBDoubleSpeed(_CGBDoubleSpeed) { Reset(); }

	void Step(DWORD clockDelta, Interrupts &INT) {}

	void Reset() { SB = 0; SC = 0; }
	
	void EmulateBIOS() { Reset(); }

	void SBChanged(BYTE value) { SB = value; }
	void SCChanged(BYTE value) { SC = value; }

	BYTE GetSB() { return SB; }
	BYTE GetSC() { return SC | 0x7E; }

private:
	const bool &CGB;
	const bool &CGBDoubleSpeed;

	BYTE SB;//Serial transfer data (R/W)
			//8 Bits of data to be read/written

	BYTE SC;//SIO control (R/W)
			//Bit 7	-	Transfer Start Flag
			//			0: Non transfer
			//			1: Start transfer
			//Bit 0 -	Shift Clock
			//			0: External Clock (500KHz Max.)
			//			1: Internal Clock (8192Hz)
};

}

#endif