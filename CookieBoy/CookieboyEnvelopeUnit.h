#ifndef COOKIEBOYENVELOPEUNIT_H
#define COOKIEBOYENVELOPEUNIT_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

/*
This unit increments/decrements sound unit volume. When sound unit triggered initial value (NRX2) is set as volume.
Once clocked it's decrements/increments volume. Volume can be in range of 0 to 15. Once it reaches 0 or 15 envelope unit stops.
*/
class EnvelopeUnit
{
public:
	EnvelopeUnit() { Reset(); }

	void Step(BYTE sequencerStep)
	{
		CurrentSequencerStep = sequencerStep;

		//Envelope unit rate is 64 Hz (every 8 frame sequencer steps)
		if (sequencerStep != 1)
		{
			return;
		}

		if (NRX2Period)//Envelope function
		{	
			ClockCounter--;

			if (ClockCounter <= 0)
			{
				ClockCounter += NRX2Period;

				if (NRX2Direction)//Increase mode
				{
					if (EnvelopeValue < 0xF0)
					{
						EnvelopeValue++;
					}
				}
				else//Decrease mode
				{
					if (EnvelopeValue > 0)
					{
						EnvelopeValue--;
					}
				}
			}
		}
		//Envelope unit treats 0 period as 8
		else
		{
			ClockCounter--;
			if (ClockCounter <= 0)
			{
				ClockCounter += 8;
			}
		}
	}

	void Reset() { NRX2 = 0; EnvelopeValue = 0; ClockCounter = 0; CurrentSequencerStep = 0; }

	void NRX2Changed(BYTE value)
	{
		//"Zombie" mode
		if (!NRX2Period && ClockCounter > 0)
		{
			EnvelopeValue++;
		}
		else if (!NRX2Direction)
		{
			EnvelopeValue += 2;
		}

		//If the mode was changed (add to subtract or subtract to add), volume is set to 16-volume
		if ((NRX2 ^ value) & 0x8)
		{
			EnvelopeValue = 0x10 - EnvelopeValue;
		}

		EnvelopeValue &= 0xF;

		NRX2 = value;
		NRX2Initial = (NRX2 >> 4) & 0xF;
		NRX2Direction = (NRX2 >> 3) & 0x1;
		NRX2Period = NRX2 & 0x7;
		DACState = !(NRX2Initial || NRX2Direction);
	}

	BYTE GetNRX2() { return NRX2; }
	int GetEnvelopeValue() { return EnvelopeValue; }

	void NRX4Changed(BYTE value)
	{
		if (value >> 7)
		{
			ClockCounter = NRX2Period;

			EnvelopeValue = NRX2Initial;

			//Envelope unit treats 0 period as 8
			if (!ClockCounter)
			{
				ClockCounter = 8;
			}

			//Next frame sequencer step clocks envelope unit
			if (CurrentSequencerStep == 0)
			{
				ClockCounter++;
			}
		}
	}

	//If initial envelope value is 0 and envelope decrease - sound is turned OFF
	bool DisablesSound() { return DACState; }

private:
	BYTE NRX2;	//Envelope (R/W)
				//Bit 7-4 - Initial volume of envelope
				//Bit 3 - Envelope UP/DOWN
				//	0: Attenuate
				//	1: Amplify
				//Bit 2-0 - Number of envelope sweep (n: 0-7) (If zero, stop envelope operation.)

	BYTE NRX2Initial;
	BYTE NRX2Direction;
	BYTE NRX2Period;
	bool DACState;
				
	int EnvelopeValue;
	int ClockCounter;

	BYTE CurrentSequencerStep;
};

}

#endif