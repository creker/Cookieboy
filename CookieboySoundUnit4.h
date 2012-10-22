#ifndef COOKIEBOYSOUNDUNIT4_H
#define COOKIEBOYSOUNDUNIT4_H

#include "CookieboyDefs.h"
#include "CookieboyLFSR.h"
#include "CookieboyEnvelopeUnit.h"
#include "CookieboyLengthCounter.h"

namespace Cookieboy
{

class Sound;

/*
Produces white noise with an envelope.

The linear feedback shift register (LFSR) generates a pseudo-random bit sequence. It has a 15-bit shift register with feedback. 
When clocked by the frequency timer, the low two bits (0 and 1) are XORed, all bits are shifted right by one, and the result of the XOR
is put into the now-empty high bit. If width mode is 1 (NR43), the XOR result is ALSO put into bit 6 AFTER the shift,
resulting in a 7-bit LFSR. The waveform output is bit 0 of the LFSR, INVERTED.

LFSR has period of 2^n - 1 where n is LFSR register length in bits. LFSR produces exactly the same sequence every period. Gameboy LFSR can work as 
7-bit (7 steps) or 15-bit (15 steps) LFSR. To optimize Sound unit 4 we can save bit sequence for 7 steps (LFSR7Table[127]) and 15 steps (LFSR15Table[32767]).
*/
class SoundUnit4
{
public:
	SoundUnit4(Sound &soundController);
	~SoundUnit4();

	void TimerStep(DWORD clockDelta);
	void FrameSequencerStep(BYTE sequencerStep);

	short GetWaveLeftOutput();
	short GetWaveRightOutput();

	void Reset();

	void EmulateBIOS();

	BYTE Status() { return StatusBit; }

	BYTE GetNR41() { return LengthCounter.GetNRX1(); }
	BYTE GetNR42() { return Envelope.GetNRX2(); }
	BYTE GetNR43() { return lfsr.GetNR43(); }
	BYTE GetNR44() { return NR44 | 0xBF; }

	void NR41Changed(BYTE value, bool override = false);
	void NR42Changed(BYTE value, bool override = false);
	void NR43Changed(BYTE value, bool override = false);
	void NR44Changed(BYTE value, bool override = false);
	
	void NR52Changed(BYTE value);

private:

	Sound &SoundController;

	//Sound 4 I\O registers
	BYTE NR44;	//Only bit 6 can be read.
				//Bit 7 - Initial (when set, sound restarts)
				//Bit 6 - Counter/consecutive selection

	BYTE StatusBit;

	LFSR lfsr;
	EnvelopeUnit Envelope;
	LengthCounter LengthCounter;
};

}

#endif