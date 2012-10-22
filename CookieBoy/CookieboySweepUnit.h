#ifndef COOKIEBOYSWEEPUNIT_H
#define COOKIEBOYSWEEPUNIT_H

#include "CookieboyDefs.h"
#include "CookieboyDutyUnit.h"

namespace Cookieboy
{

/*
Sweep unit increments/decrements sound unit frequency.
*/
class SweepUnit
{
public:
	SweepUnit(BYTE &NRX3, BYTE &NRX4, BYTE &StatusBit, DutyUnit &dutyUnit) : NRX3(NRX3), NRX4(NRX4), StatusBit(StatusBit), Duty(dutyUnit)
	{
		Reset();
	}

	void Step(BYTE sequencerStep)
	{
		CurrentSequencerStep = sequencerStep;

		//Sweep unit rate is 128 Hz (every 4 frame sequencer steps)
		if (CurrentSequencerStep != 0 && CurrentSequencerStep != 4)
		{
			return;
		}

		//Sweep time is positive
		if (NRX0SweepPeriod)//Sweep function
		{
			ClockCounter--;
			if (ClockCounter <= 0)
			{
				ClockCounter += NRX0SweepPeriod;
				
				if (SweepEnabled)
				{
					SweepStep();
				}
			}
		}
		//If sweep time is zero timer treats it as 8
		else
		{
			ClockCounter--;
			if (ClockCounter <= 0)
			{
				ClockCounter += 8;
			}
		}
	}

	void Reset()
	{
		NRX0 = 0;
		oldNRX4 = 0;
		ClockCounter = 0;
		ShadowReg = ((NRX4 & 0x7) << 8) | NRX3;
		SweepEnabled = false;
	}

	void NRX0Changed(BYTE value)
	{
		NRX0 = value;
		NRX0SweepPeriod = (NRX0 >> 4) & 0x7;
		NRX0SweepDirection = (NRX0 >> 3) & 0x1;
		NRX0SweepShiftLength = NRX0 & 0x7;

		//Clearing negate mode after at least one calculation was made in negate mode disables channel 
		if (!NRX0SweepDirection && NegateMode)
		{
			StatusBit = 0;
		}
	}

	BYTE GetNRX0() { return NRX0 | 0x80; }

	void NRX4Changed(BYTE value)
	{
		oldNRX4 = value;

		if (value >> 7)
		{
			ShadowReg = ((NRX4 & 0x7) << 8) | NRX3;
			ClockCounter = NRX0SweepPeriod;
			NegateMode = false;

			//If sweep time is zero timer treats it as 8
			if (!NRX0SweepPeriod)
			{
				ClockCounter = 8;
			}

			SweepEnabled = NRX0SweepShiftLength || NRX0SweepPeriod;

			if (NRX0SweepShiftLength)
			{
				CalculateFrequency();
			}
		}
	}

private:
	void SweepStep()
	{
		int newFreq = CalculateFrequency();

		if ((newFreq <= 2047.0) && NRX0SweepShiftLength)
		{
			NRX3 = newFreq & 0xFF;
			NRX4 = (NRX4 & 0xF8) | ((newFreq >> 8) & 0x7);

			Duty.NRX3Changed(NRX3);
			Duty.NRX4Changed(NRX4);

			ShadowReg = newFreq;

			//Frequency calculation and overflow check AGAIN but not saving new frequency
			CalculateFrequency();
		}
	}

	int CalculateFrequency()
	{
		int newFreq = ShadowReg;
		int shadowRegShift = ShadowReg >> NRX0SweepShiftLength;

		if (NRX0SweepDirection)//Decrease mode
		{
			//If next frequency is negative then using current frequency
			if (ShadowReg >= shadowRegShift)
			{
				newFreq = ShadowReg - shadowRegShift;
			}

			NegateMode = true;
		}
		else//Increase mode
		{
			newFreq = ShadowReg + shadowRegShift;

			NegateMode = false;
		}
		
		//If frequency exeeds maximum sound disabled
		if (newFreq > 2047.0)
		{
			StatusBit = 0;
		}

		return newFreq;
	}

	BYTE NRX0;	//Sweep register (R/W)
				//Bit 6-4 - Sweep Time
				//Bit 3 - Sweep Increase/Decrease
				//	0: Addition (frequency increases)
				//	1: Subtraction (frequency decreases)
				//Bit 2-0 - Number of sweep shift (n: 0-7)

	BYTE &NRX3;
	BYTE oldNRX4;
	BYTE &NRX4;

	BYTE NRX0SweepPeriod;
	BYTE NRX0SweepDirection;
	BYTE NRX0SweepShiftLength;

	DutyUnit &Duty;
	int ShadowReg;
	int ClockCounter;
	bool SweepEnabled;
	bool NegateMode;//Indicates that at least one calculation was made in negate mode

	BYTE &StatusBit;
	BYTE CurrentSequencerStep;
};

}

#endif