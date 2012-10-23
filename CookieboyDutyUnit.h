#ifndef COOKIEBOYSOUNDDUTYUNIT_H
#define COOKIEBOYSOUNDDUTYUNIT_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

class DutyUnit
{
public:
	DutyUnit() { Reset(); }
	
	void Reset()
	{
		ClockCounter = 0;
		Period = 2048 * 4;
		Output = 0;
		DutyPhase = 0;
		DutyCycleIndex = 0;
		Enabled = false;
	}

	void Step(DWORD clockDelta)
	{
		static const BYTE DutyCycles[4][8] = {
			{0, 0, 0, 0, 0, 0, 0, 1},
			{1, 0, 0, 0, 0, 0, 0, 1},
			{1, 0, 0, 0, 0, 1, 1, 1},
			{0, 1, 1, 1, 1, 1, 1, 0}
		};

		ClockCounter += clockDelta;
		if (ClockCounter >= Period)
		{
			int passedPeriods = ClockCounter / Period;
			ClockCounter %= Period;
			
			if (Enabled)
			{
				DutyPhase = (DutyPhase + passedPeriods) & 0x7;
			}
			else
			{
				DutyPhase = 0;
			}

			Output = DutyCycles[DutyCycleIndex][DutyPhase];
		}
	}

	int GetOutput() { return Output; }

	void NRX1Changed(BYTE value)
	{
		DutyCycleIndex = value >> 6;
	}

	void NRX3Changed(BYTE value)
	{
		NRX3 = value;

		CalculatePeriod();
	}

	void NRX4Changed(BYTE value)
	{
		NRX4 = value;

		//Square duty sequence clocking is disabled until the first trigger
		if (NRX4 & 0x80)
		{
			ClockCounter = 0;
			Enabled = true;
		}

		CalculatePeriod();
	}

private:
	void CalculatePeriod()
	{
		Period = (2048 - (((NRX4 & 0x7) << 8) | NRX3)) * 4;
	}

	BYTE NRX3;
	BYTE NRX4;

	int ClockCounter;
	int Period;
	int Output;
	int DutyPhase;
	int DutyCycleIndex;

	bool Enabled;
};

}

#endif