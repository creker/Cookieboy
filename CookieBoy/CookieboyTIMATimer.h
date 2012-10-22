#ifndef COOKIEBOYTIMA_H
#define COOKIEBOYTIMA_H

#include "CookieboyDefs.h"
#include "CookieboyInterrupts.h"

namespace Cookieboy
{

/*
The timer in the GameBoy
has a selectable frequency of 4096, 16384, 65536, or 262144 Hertz. This frequency increments the Timer
Counter (TIMA). When it overflows, it generates an interrupt. It is then loaded with the contents of Timer Modulo (TMA)
*/
class TIMATimer
{
public:
	TIMATimer() { Reset(); }

	void Step(DWORD clockDelta, Interrupts &INT)
	{
		//TIMA enabled
		if (TAC & 0x4)
		{
			ClockCounter += clockDelta;

			if (ClockCounter >= Period)
			{
				int passedPeriods = ClockCounter / Period;
				ClockCounter %= Period;

				//Timer overflow check
				if (TIMA + passedPeriods >= 255)
				{
					TIMA += passedPeriods;
					TIMA += TMA;

					INT.Request(Interrupts::INTERRUPT_TIMA);
				}
				else
				{
					TIMA += passedPeriods;
				}
			}
		}
	}

	void Reset() 
	{
		ClockCounter = 0; 
		TIMA = 0;
		TMA = 0;
		TAC = 0;
		Period = 1024; 
	}

	void EmulateBIOS() { Reset(); }

	void TIMAChanged(BYTE value) { TIMA = value; }
	void TMAChanged(BYTE value) { TMA = value; }

	void TACChanged(BYTE value)
	{
		TAC = value;

		switch (TAC & 0x3)
		{
			//4096 Hz
			case 0:
				Period = 1024;
				break;
			//262144 Hz
			case 1:
				Period = 16;
				break;
			//65536 Hz
			case 2:
				Period = 64;
				break;
			//16384 Hz
			case 3:
				Period = 256;
				break;
		}
	}

	BYTE GetTIMA() { return TIMA; }
	BYTE GetTMA() { return TMA; }
	BYTE GetTAC() { return TAC | 0xF8; }

private:
	DWORD ClockCounter;
	DWORD Period;

	BYTE TIMA;	//Timer counter (R/W)
				//This timer is incremented by a clock frequency specified by the TAC register
				//($FF07). The timer generates an interrupt when it overflows

	BYTE TMA;	//Timer Modulo (R/W)
				//When the TIMA overflows, this data will be loaded.

	BYTE TAC;	//Timer Control (R/W)
				//Bit 2 - Timer Stop
				//	0: Stop Timer
				//	1: Start Timer
				//Bits 1+0 - Input Clock Select
				//	00: 4.096 KHz (~4.194 KHz SGB)
				//	01: 262.144 Khz (~268.4 KHz SGB)
				//	10: 65.536 KHz (~67.11 KHz SGB)
				//	11: 16.384 KHz (~16.78 KHz SGB)
};

}

#endif