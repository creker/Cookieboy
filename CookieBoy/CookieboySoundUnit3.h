#ifndef COOKIEBOYSOUNDUNIT3_H
#define COOKIEBOYSOUNDUNIT3_H

#include "CookieboyDefs.h"
#include "CookieboyLengthCounter.h"

namespace Cookieboy
{

class Sound;

/*
Outputs voluntary wave patterns from Wave RAM

The wave channel plays a 32-entry wave table made up of 4-bit samples. Each byte encodes two samples, the first in the high bits.
The wave channel's frequency timer period is set to (2048-frequency)*2
*/
class SoundUnit3
{
public:
	SoundUnit3(Sound &soundController);
	~SoundUnit3();

	void TimerStep(DWORD clockDelta);
	void FrameSequencerStep(BYTE sequencerStep);

	short GetWaveLeftOutput();
	short GetWaveRightOutput();

	void Reset();

	void EmulateBIOS();

	BYTE Status() { return StatusBit; }

	BYTE GetNR30() { return NR30 | 0x7F; }
	BYTE GetNR31() { return LengthCounter.GetNRX1(); }
	BYTE GetNR32() { return NR32 | 0x9F; }
	BYTE GetNR33() { return 0xFF; }
	BYTE GetNR34() { return NR34 | 0xBF; }
	BYTE GetWaveRAM(BYTE pos);

	void NR30Changed(BYTE value, bool override = false);
	void NR31Changed(BYTE value, bool override = false);
	void NR32Changed(BYTE value, bool override = false);
	void NR33Changed(BYTE value, bool override = false);
	void NR34Changed(BYTE value, bool override = false);
	void WaveRAMChanged(BYTE pos, BYTE value);

	void NR52Changed(BYTE value);

private:

	void CalculatePeriod() { Period = (2048 - (((NR34 & 0x7) << 8) | NR33)) * 2; }

	Sound &SoundController;

	//Sound 3 I\O registers
	BYTE NR30;	//Sound on/off (R/W)
				//Only bit 7 can be read
				//Bit 7 - Sound OFF
				//	0: Sound 3 output stop
				//	1: Sound 3 output OK
				
	BYTE NR32;	//Select output level (R/W)
				//Only bits 6-5 can be read
				//Bit 6-5 - Select output level
				//	00: Mute
				//	01: Produce Wave Pattern RAM. Data as it is(4 bit length)
				//	10: Produce Wave Pattern RAM. Data shifted once to the RIGHT (1/2) (4 bit length)
				//	11: Produce Wave Pattern RAM. Data shifted twice to the RIGHT (1/4) (4 bit length)
	
	BYTE NR33;	//Frequency's lower data (W)
				//Lower 8 bits of an 11 bit frequency

	BYTE NR34;	//Only bit 6 can be read.
				//Bit 7 - Initial (when set,sound restarts)
				//Bit 6 - Counter/consecutive flag
				//Bit 2-0 - Frequency's higher 3 bits

	BYTE WaveRAM[0x10];	//Waveform storage for arbitrary sound data
						//This storage area holds 32 4-bit samples that are played back upper 4 bits first
						
	BYTE StatusBit;

	//Sample buffer stuff
	BYTE SampleBuffer;
	int SampleIndex;
	int ClockCounter;
	int Period;
	BYTE Output;

	LengthCounter LengthCounter;
};

}

#endif