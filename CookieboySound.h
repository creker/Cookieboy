#ifndef COOKIEBOYSOUND_H
#define COOKIEBOYSOUND_H

#include "CookieboyDefs.h"
#include "CookieboySoundUnit1.h"
#include "CookieboySoundUnit2.h"
#include "CookieboySoundUnit3.h"
#include "CookieboySoundUnit4.h"

namespace Cookieboy
{

/*
Passed tests
	01-registers
	02-len ctr
	03-trigger
	04-sweep
	05-sweep details
	06-overflow on trigger
	07-len sweep period sync
	08-len ctr during power
	11-regs after power

	Passed all tests except wave pattern RAM test 
*/

/*
	Frame Sequencer

	Step    Length Ctr  Vol Env     Sweep
	- - - - - - - - - - - - - - - - - - - -
	0       Clock       -           Clock
	1       -           Clock       -
	2       Clock       -           -
	3       -           -           -
	4       Clock       -           Clock
	5       -           -           -
	6       Clock       -           -
	7       -           -           -
	- - - - - - - - - - - - - - - - - - - -
	Rate    256 Hz      64 Hz       128 Hz

	Frame sequencer on reset starts at step 1
*/

class Sound
{
public:
	Sound(const bool &_CGB, int sampleRate = 44100, int sampleBufferLength = 1024);
	~Sound();

	void Step(DWORD clockDelta);

	void SetVolume(double vol);
	
	BYTE GetNR10() { return Sound1->GetNR10(); }
	BYTE GetNR11() { return Sound1->GetNR11(); }
	BYTE GetNR12() { return Sound1->GetNR12(); }
	BYTE GetNR13() { return Sound1->GetNR13(); }
	BYTE GetNR14() { return Sound1->GetNR14(); }

	BYTE GetNR21() { return Sound2->GetNR21(); }
	BYTE GetNR22() { return Sound2->GetNR22(); }
	BYTE GetNR23() { return Sound2->GetNR23(); }
	BYTE GetNR24() { return Sound2->GetNR24(); }

	BYTE GetNR30() { return Sound3->GetNR30(); }
	BYTE GetNR31() { return Sound3->GetNR31(); }
	BYTE GetNR32() { return Sound3->GetNR32(); }
	BYTE GetNR33() { return Sound3->GetNR33(); }
	BYTE GetNR34() { return Sound3->GetNR34(); }
	BYTE GetWaveRAM(BYTE pos) { return Sound3->GetWaveRAM(pos); }

	BYTE GetNR41() { return Sound4->GetNR41(); }
	BYTE GetNR42() { return Sound4->GetNR42(); }
	BYTE GetNR43() { return Sound4->GetNR43(); }
	BYTE GetNR44() { return Sound4->GetNR44(); }

	BYTE GetNR50() { return NR50; }
	BYTE GetNR51() { return NR51; }
	BYTE GetNR52() { return ((AllSoundEnabled << 7) | (Sound4->Status() << 3) | (Sound3->Status() << 2) | (Sound2->Status() << 1) | Sound1->Status()) | 0x70; }

	//With override argument set to true we can write sound registers even if all sound is off
	void NR10Changed(BYTE value, bool override = false) { Sound1->NR10Changed(value, override); }
	void NR11Changed(BYTE value, bool override = false) { Sound1->NR11Changed(value, override); }
	void NR12Changed(BYTE value, bool override = false) { Sound1->NR12Changed(value, override); }
	void NR13Changed(BYTE value, bool override = false) { Sound1->NR13Changed(value, override); }
	void NR14Changed(BYTE value, bool override = false) { Sound1->NR14Changed(value, override); }

	void NR21Changed(BYTE value, bool override = false) { Sound2->NR21Changed(value, override); }
	void NR22Changed(BYTE value, bool override = false) { Sound2->NR22Changed(value, override); }
	void NR23Changed(BYTE value, bool override = false) { Sound2->NR23Changed(value, override); }
	void NR24Changed(BYTE value, bool override = false) { Sound2->NR24Changed(value, override); }

	void NR30Changed(BYTE value, bool override = false) { Sound3->NR30Changed(value, override); }
	void NR31Changed(BYTE value, bool override = false) { Sound3->NR31Changed(value, override); }
	void NR32Changed(BYTE value, bool override = false) { Sound3->NR32Changed(value, override); }
	void NR33Changed(BYTE value, bool override = false) { Sound3->NR33Changed(value, override); }
	void NR34Changed(BYTE value, bool override = false) { Sound3->NR34Changed(value, override); }
	void WaveRAMChanged(BYTE pos, BYTE value) { Sound3->WaveRAMChanged(pos, value); }

	void NR41Changed(BYTE value, bool override = false) { Sound4->NR41Changed(value, override); }
	void NR42Changed(BYTE value, bool override = false) { Sound4->NR42Changed(value, override); }
	void NR43Changed(BYTE value, bool override = false) { Sound4->NR43Changed(value, override); }
	void NR44Changed(BYTE value, bool override = false) { Sound4->NR44Changed(value, override); }

	void NR50Changed(BYTE value, bool override = false);
	void NR51Changed(BYTE value, bool override = false);
	void NR52Changed(BYTE value);

	void Reset();

	void EmulateBIOS();

	bool GetAllSoundEnabled() { return AllSoundEnabled > 0; }

	const short* GetSoundFramebuffer() { return SampleBuffer; }
	bool IsNewFrameReady() { return NewFrameReady; }
	void WaitForNewFrame() { NewFrameReady = false; }

private:

	const bool &CGB;

	//Sound units
	SoundUnit1 *Sound1;
	SoundUnit2 *Sound2;
	SoundUnit3 *Sound3;
	SoundUnit4 *Sound4;
	
	BYTE NR50;	//Channel control / ON-OFF / Volume (R/W)
				//Bit 7 - Vin->SO2 ON/OFF
				//Bit 6-4 - SO2 output level (volume) (# 0-7)
				//Bit 3 - Vin->SO1 ON/OFF
				//Bit 2-0 - SO1 output level (volume) (# 0-7)
				
	BYTE NR51;	//Selection of Sound output terminal (R/W)
				//Bit 7 - Output sound 4 to SO2 terminal
				//Bit 6 - Output sound 3 to SO2 terminal
				//Bit 5 - Output sound 2 to SO2 terminal
				//Bit 4 - Output sound 1 to SO2 terminal
				//Bit 3 - Output sound 4 to SO1 terminal
				//Bit 2 - Output sound 3 to SO1 terminal
				//Bit 1 - Output sound 2 to SO1 terminal
				//Bit 0 - Output sound 1 to SO1 terminal
				//
				//S02 - left channel
				//S01 - right channel

	BYTE AllSoundEnabled;
	//NR52 - Sound on/off (R/W)
	//Bit 7 - All sound on/off
	//	0: stop all sound circuits
	//	1: operate all sound circuits
	//Bit 3 - Sound 4 ON flag
	//Bit 2 - Sound 3 ON flag
	//Bit 1 - Sound 2 ON flag
	//Bit 0 - Sound 1 ON flag
	//
	//We don't use external NR52 register. NR52 value combined from various variables.
	//Status bits located in sound unit classes.

	const int SampleRate;
	const int SampleBufferLength;
	int SamplePeriod;

	int SampleCounter;
	int SampleBufferPos;
	short* SampleBuffer;
	bool NewFrameReady;

	int FrameSequencerClock;
	BYTE FrameSequencerStep;

	double MasterVolume;
};

}

#endif