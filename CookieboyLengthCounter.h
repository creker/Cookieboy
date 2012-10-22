#ifndef COOKIEBOYLENGTHCOUNTER_H
#define COOKIEBOYLENGTHCOUNTER_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

/*
Length counter controls sound unit state. When it reaches zero sound unit is stopped.
*/
class LengthCounter
{
public:
	LengthCounter(BYTE mask, BYTE &statusBit, int NRX1ReadMask = -1) : Mask(mask), StatusBit(statusBit)
	{
		Reset(); 

		if (NRX1ReadMask < 0)
		{
			this->NRX1ReadMask = Mask;
		}
		else
		{
			this->NRX1ReadMask = NRX1ReadMask & 0xFF;
		}
	}

	void Step(BYTE sequencerStep)
	{
		CurrentSequencerStep = sequencerStep;

		//Length counter rate is 256 Hz (every 2 frame sequencer steps)
		if (sequencerStep % 2)
		{
			return;
		}
		
		if (NRX4CounterMode)//Counter mode
		{
			ClockLengthCounter();
		}
	}

	void Reset()
	{
		NRX1 = 0;
		NRX4CounterMode = 0;
		ClockCounter = 0;
		CurrentSequencerStep = 0;
	}

	void PowerOFF()
	{
		NRX1 &= Mask;
	}

	BYTE GetLengthMask() { return Mask; }

	BYTE GetNRX1() { return NRX1 | NRX1ReadMask; }

	void NRX1Changed(BYTE value)
	{
		NRX1 = value;

		//Write to NRX1 resets length counter
		ReloadLengthCounter();
	}

	void NRX4Changed(BYTE value)
	{
		//If Length counter previously was disabled and now enabled
		//If current frame sequencer step clocks length counter and length counter non-zero
		if (!NRX4CounterMode && (value & 0x40) && !(CurrentSequencerStep % 2) && ClockCounter)
		{
			BYTE oldStatusBit = StatusBit;
			ClockLengthCounter();//extra length clock

			//Sound disabled only if trigger is clear
			if (value & 0x80)
			{
				StatusBit = oldStatusBit;
			}
		}

		//If triggered and length counter reached zero
		if ((value & 0x80) && !ClockCounter)
		{
			NRX1Changed(NRX1 & (~Mask));//Loading max value to length counter

			//If length counter being enabled and current frame sequencer step clocks length counter
			if ((value & 0x40) && !(CurrentSequencerStep % 2))
			{
				ClockLengthCounter();//extra length clock
			}
		}

		NRX4 = value;
		NRX4CounterMode = (NRX4 >> 6) & 0x1;
	}
	
private:
	void ReloadLengthCounter()
	{
		ClockCounter = (~NRX1 & Mask) + 1;
	}

	void ClockLengthCounter()
	{
		ClockCounter--;

		//Duration counter stops sound
		if (ClockCounter <= 0)
		{
			ClockCounter = 0;
			
			StatusBit = 0;
		}

		BYTE newNRX1 = ~(ClockCounter - 1) & Mask;
		NRX1 = (NRX1 & (~Mask)) | newNRX1;
	}

	BYTE NRX1;	//Sound length. Each sound unit has diffirent bit mapping
	BYTE NRX4;

	BYTE Mask;
	BYTE NRX1ReadMask;
	BYTE NRX4CounterMode;
	BYTE &StatusBit;
	int ClockCounter;

	BYTE CurrentSequencerStep;
};

}

#endif