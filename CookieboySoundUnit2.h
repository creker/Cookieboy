#ifndef COOKIEBOYSOUNDUNIT2_H
#define COOKIEBOYSOUNDUNIT2_H

#include "CookieboyDefs.h"
#include "CookieboyDutyUnit.h"
#include "CookieboyEnvelopeUnit.h"
#include "CookieboyLengthCounter.h"

namespace Cookieboy
{

class Sound;

/*
Produces quadrangular waves with an envelope. Works exactly line sound unit 1 except there is no frequency sweep unit.

A square channel's frequency timer period is set to (2048-frequency)*4. Four duty cycles are available, 
each waveform taking 8 frequency timer clocks to cycle through.
*/
class SoundUnit2
{
public:
	SoundUnit2(const bool &_CGB, Sound &soundController);
	~SoundUnit2();

	void TimerStep(DWORD clockDelta);
	void FrameSequencerStep(BYTE sequencerStep);

	short GetWaveLeftOutput();
	short GetWaveRightOutput();

	void Reset();

	void EmulateBIOS();

	BYTE Status() { return StatusBit; }

	BYTE GetNR21() { return LengthCounter.GetNRX1(); }
	BYTE GetNR22() { return Envelope.GetNRX2(); }
	BYTE GetNR23() { return 0xFF; }
	BYTE GetNR24() { return NR24 | 0xBF; }

	void NR21Changed(BYTE value, bool override = false);
	void NR22Changed(BYTE value, bool override = false);
	void NR23Changed(BYTE value, bool override = false);
	void NR24Changed(BYTE value, bool override = false);

	void NR52Changed(BYTE value);

private:

	const bool &CGB;

	Sound &SoundController;

	//Sound 2 I\O registers
	BYTE NR21;	//Sound Length; Wave Pattern Duty (R/W)
				//Only bits 7-6 can be read.
				//Bit 7-6 - Wave pattern duty
				//Bit 5-0 - Sound length data (t1: 0-63)
				
	BYTE NR23;	//Frequency lo data (W) 
				//Frequency's lower 8 bits of 11 bit data

	BYTE NR24;	//Only bit 6 can be read.
				//Bit 7 - Initial (when set, sound restarts)
				//Bit 6 - Counter/consecutive selection
				//Bit 2-0 - Frequency's higher 3 bits

	BYTE StatusBit;

	DutyUnit Duty;
	EnvelopeUnit Envelope;
	LengthCounter LengthCounter;

	DWORD WaveOutput;
};

}

#endif