#ifndef COOKIEBOYSPEEDSWITCHER_H
#define COOKIEBOYSPEEDSWITCHER_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

/*
This register is used to prepare the gameboy to switch between CGB Double Speed Mode and Normal Speed Mode. 
The actual speed switch is performed by executing a STOP command after Bit 0 has been set. After that Bit 0 will be cleared automatically, 
and the gameboy will operate at the 'other' speed.

In Double Speed Mode the following will operate twice as fast as normal:
  The CPU (2.10 MHz, 1 Cycle = approx. 0.5us)
  Timer and Divider Registers
  Serial Port (Link Cable)
  DMA Transfer to OAM
And the following will keep operating as usual:
  LCD Video Controller
  HDMA Transfer to VRAM
  All Sound Timings and Frequencies
*/
class SpeedSwitcher
{
public:
	SpeedSwitcher(const bool &_CGB, const bool &_CGBDoubleSpeed) : CGB(_CGB), CGBDoubleSpeed(_CGBDoubleSpeed) { Reset(); }

	void Reset()
	{
		PrepareSpeedSwitch = 0;
	}

	void EmulateBIOS()
	{
		Reset();
	}

	void KEY1Changed(BYTE value)
	{
		if (CGB)
		{
			PrepareSpeedSwitch = value & 0x1;
		}
	}

	BYTE GetKEY1() 
	{ 
		return ((CGBDoubleSpeed ? 1 : 0) << 7) | 0x7E | (PrepareSpeedSwitch & 0x1);
	}

private:

	const bool &CGB;
	const bool &CGBDoubleSpeed;

	BYTE PrepareSpeedSwitch;//(0=No, 1=Prepare) (Read/Write)
};

}

#endif