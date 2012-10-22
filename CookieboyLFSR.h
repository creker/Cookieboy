#ifndef COOKIEBOYLFSR_H
#define COOKIEBOYLFSR_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

class LFSR
{
public:
	LFSR() 
	{
		LFSRPeriods[0] = 32768;
		LFSRPeriods[1] = 128;

		Reset();
	}

	void Step(DWORD clockDelta)
	{
		static BYTE LFSR7Table[127] = 
		{
			#include "LFSR7.inc"
		};
		static BYTE LFSR15Table[32767] = 
		{
			#include "LFSR15.inc"
		};
		static BYTE* LFSRTables[2] = {LFSR15Table, LFSR7Table};

		//Using a noise channel clock shift of 14 or 15 results in the LFSR receiving no clocks
		if (Period == 0 || NR43Clock == 14 + 4 || NR43Clock == 15 + 4)
		{
			return;
		}

		ClockCounter += clockDelta;
		if (ClockCounter >= Period)
		{
			int passedPeriods = ClockCounter / Period;
			ClockCounter %= Period;

			SampleIndex += passedPeriods;
			SampleIndex %= LFSRPeriods[NR43Step];

			Output = LFSRTables[NR43Step][SampleIndex];
		}
	}

	BYTE GetOutput() { return Output; }

	void Reset()
	{
		NR43Changed(0);
		ClockCounter = 0;
		SampleIndex = 0;
		Output = 0;
	}

	BYTE GetNR43() { return NR43; }

	void NR43Changed(BYTE value)
	{
		NR43 = value;

		NR43Clock = (NR43 >> 4) + 4;
		NR43Step = (NR43 >> 3) & 0x1;
		NR43Ratio = NR43 & 0x7;

		if (!NR43Ratio)
		{
			NR43Ratio = 1;
			NR43Clock--;
		}

		Period = NR43Ratio << NR43Clock;

		//Using a noise channel clock shift of 14 or 15 results in the LFSR receiving no clocks
		if (Period == 0 || NR43Clock == 14 + 4 || NR43Clock == 15 + 4)
		{
			ClockCounter = 0;
			SampleIndex = 0;
		}
		else
		{
			ClockCounter %= Period;
		}
	}

	void NR44Changed(BYTE value)
	{
		if (value & 0x80)
		{
			ClockCounter = 0;
			SampleIndex = 0;
		}
	}

private:

	BYTE NR43;	//Polynomial counter (R/W)
				//Bit 7-4 - Selection of the shift clock frequency of the polynomial counter
				//Bit 3 - Selection of the polynomial	counter's step
				//Bit 2-0 - Selection of the dividing ratio of frequencies:
				//	000: f * 1/2^3 * 2
				//	001: f * 1/2^3 * 1
				//	010: f * 1/2^3 * 1/2
				//	011: f * 1/2^3 * 1/3
				//	100: f * 1/2^3 * 1/4
				//	101: f * 1/2^3 * 1/5
				//	110: f * 1/2^3 * 1/6
				//	111: f * 1/2^3 * 1/7
				//	f = 4.194304 Mhz
				//
				//Selection of the polynomial counter step:
				//	0: 15 steps
				//	1: 7 steps
				//
				//Selection of the shift clock frequency of the polynomial counter:
				//	0000: dividing ratio of frequencies * 1/2
				//	0001: dividing ratio of frequencies * 1/2^2
				//	0010: dividing ratio of frequencies * 1/2^3
				//	0011: dividing ratio of frequencies * 1/2^4
				//	: :
				//	: :
				//	: :
				//	0101: dividing ratio of frequencies * 1/2^14
				//	1110: prohibited code
				//	1111: prohibited code
	BYTE NR43Clock;
	BYTE NR43Step;
	BYTE NR43Ratio;

	BYTE Output;
	int ClockCounter;
	int SampleIndex;
	int Period;

	int LFSRPeriods[2];
};

}

#endif