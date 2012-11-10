#ifndef COOKIEBOYSOUNDUNIT1_H
#define COOKIEBOYSOUNDUNIT1_H

#include "CookieboyDefs.h"
#include "CookieboyDutyUnit.h"
#include "CookieboySweepUnit.h"
#include "CookieboyEnvelopeUnit.h"
#include "CookieboyLengthCounter.h"

namespace Cookieboy
{

class Sound;

/*
Produces quadrangular waves with sweep and envelope functions.

A square channel's frequency timer period is set to (2048-frequency)*4. Four duty cycles are available, 
each waveform taking 8 frequency timer clocks to cycle through.
*/
class SoundUnit1
{
public:
	SoundUnit1(const bool &_CGB, Sound &soundController);
	~SoundUnit1();

	void TimerStep(DWORD clockDelta);
	void FrameSequencerStep(BYTE sequencerStep);

	short GetWaveLeftOutput();
	short GetWaveRightOutput();

	void Reset();

	void EmulateBIOS();

	BYTE Status() { return StatusBit; }

	BYTE GetNR10() { return Sweep.GetNRX0(); }
	BYTE GetNR11() { return NR11 | 0x3F; }
	BYTE GetNR12() { return Envelope.GetNRX2(); }
	BYTE GetNR13() { return 0xFF; }
	BYTE GetNR14() { return NR14 | 0xBF; }

	void NR10Changed(BYTE value, bool override = false);
	void NR11Changed(BYTE value, bool override = false);
	void NR12Changed(BYTE value, bool override = false);
	void NR13Changed(BYTE value, bool override = false);
	void NR14Changed(BYTE value, bool override = false);
	
	void NR52Changed(BYTE value);

private:

	const bool &CGB;

	Sound &SoundController;

	//Sound 1 I\O registers
	BYTE NR11;	//Sound length/Wave pattern duty (R/W)
				//Only Bits 7-6 can be read.
				//Bit 7-6 - Wave Pattern Duty
				//Bit 5-0 - Sound length data (t1: 0-63)

	BYTE NR13;	//Frequency lo (W)
				//Lower 8 bits of 11 bit frequency
				//Next 3 bit are in NR X4

	BYTE NR14;	//Only Bit 6 can be read.
				//Bit 7 - Initial (when set, sound restarts)
				//Bit 6 - Counter/consecutive selection
				//Bit 2-0 - Frequency's higher 3 bits

	BYTE StatusBit;

	DutyUnit Duty;
	SweepUnit Sweep;
	EnvelopeUnit Envelope;
	LengthCounter LengthCounter;

	DWORD WaveOutput;
};

}

#endif