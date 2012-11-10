#include "CookieboySound.h"
#include <SDL.h>
#include <algorithm>

Cookieboy::Sound::Sound(const bool &_CGB, int sampleRate, int sampleBufferLength):
CGB(_CGB),
SampleRate(sampleRate),
SampleBufferLength(sampleBufferLength)
{
	SampleBuffer = new short[SampleBufferLength];
	SamplePeriod = 4194304 / SampleRate;
	memset(SampleBuffer, 0, sizeof(short) * SampleBufferLength);

	MasterVolume = 1;

	Sound1 = new SoundUnit1(CGB, *this);
	Sound2 = new SoundUnit2(CGB, *this);
	Sound3 = new SoundUnit3(CGB, *this);
	Sound4 = new SoundUnit4(CGB, *this);

	Reset();
}

Cookieboy::Sound::~Sound()
{
	delete Sound1;
	delete Sound2;
	delete Sound3;
	delete Sound4;

	delete[] SampleBuffer;
}

void Cookieboy::Sound::Step(DWORD clockDelta)
{
	Sound1->TimerStep(clockDelta);
	Sound2->TimerStep(clockDelta);
	Sound3->TimerStep(clockDelta);
	Sound4->TimerStep(clockDelta);

	FrameSequencerClock += clockDelta;
	
	if (FrameSequencerClock >= 4194304 / 512)
	{
		FrameSequencerClock -= 4194304 / 512;
		
		FrameSequencerStep++;
		FrameSequencerStep &= 0x7;

		if (AllSoundEnabled)
		{
			Sound1->FrameSequencerStep(FrameSequencerStep);
			Sound2->FrameSequencerStep(FrameSequencerStep);
			Sound3->FrameSequencerStep(FrameSequencerStep);
			Sound4->FrameSequencerStep(FrameSequencerStep);
		}
	}

	SampleCounter += clockDelta;
	if (SampleCounter >= SamplePeriod)
	{
		SampleCounter %= SamplePeriod;

		//Mixing is done by adding voltages from every channel
		SampleBuffer[SampleBufferPos] = Sound1->GetWaveLeftOutput() + Sound2->GetWaveLeftOutput() + Sound3->GetWaveLeftOutput() + Sound4->GetWaveLeftOutput();
		SampleBuffer[SampleBufferPos + 1] = Sound1->GetWaveRightOutput() + Sound2->GetWaveRightOutput() + Sound3->GetWaveRightOutput() + Sound4->GetWaveRightOutput();

		//Amplifying sound
		//Max amplitude for 16-bit audio is 32767. Max channel volume is 15. Max master volume is 7 + 1
		//So gain is 32767 / (15 * 8 * 4) ~ 64
		SampleBuffer[SampleBufferPos] *= 64;
		SampleBuffer[SampleBufferPos + 1] *= 64;

		//DMG doesn't have this one. This is global volume so we don't need to use OS volume settings to change emulator volume
		SampleBuffer[SampleBufferPos] = short(SampleBuffer[SampleBufferPos] * MasterVolume);
		SampleBuffer[SampleBufferPos + 1] = short(SampleBuffer[SampleBufferPos + 1] * MasterVolume);

		SampleBufferPos += 2;

		//"Resampling" DMG samples to actual sound samples
		if (SampleBufferPos >= SampleBufferLength)
		{
			SampleBufferPos = 0;
			NewFrameReady = true;
		}
	}
}

void Cookieboy::Sound::SetVolume(double vol)
{
	MasterVolume = vol;
}

void Cookieboy::Sound::Reset()
{
	Sound1->Reset();
	Sound2->Reset();
	Sound3->Reset();
	Sound4->Reset();

	NR50Changed(0, true);
	NR51Changed(0, true);
	AllSoundEnabled = 0;
	FrameSequencerClock = 0;
	FrameSequencerStep = 1;
	SampleCounter = 0;
	SampleBufferPos = 0;
	NewFrameReady = false;
}

void Cookieboy::Sound::EmulateBIOS()
{
	Sound1->EmulateBIOS();
	Sound2->EmulateBIOS();
	Sound3->EmulateBIOS();
	Sound4->EmulateBIOS();

	NR50Changed(0x77, true);
	NR51Changed(0xF3, true);
	NR52Changed(0xF1);

	AllSoundEnabled = 1;
}

void Cookieboy::Sound::NR50Changed(BYTE value, bool override)
{
	if (!AllSoundEnabled && !override)
	{
		return;
	}

	NR50 = value;
}

void Cookieboy::Sound::NR51Changed(BYTE value, bool override)
{
	if (!AllSoundEnabled && !override)
	{
		return;
	}

	NR51 = value;
}

void Cookieboy::Sound::NR52Changed(BYTE value)
{
	//If sound being turned in
	if (value & 0x80)
	{
		if (!AllSoundEnabled)
		{
			//On power on frame sequencer starts at 1
			FrameSequencerStep = 1;
			FrameSequencerClock = 0;
			
			Sound1->NR52Changed(value);
			Sound2->NR52Changed(value);
			Sound3->NR52Changed(value);
			Sound4->NR52Changed(value);

			//Very important to let know sound units that frame sequencer was reset.
			//Test "08-len ctr during power" requires this to pass
			Sound1->FrameSequencerStep(FrameSequencerStep);
			Sound2->FrameSequencerStep(FrameSequencerStep);
			Sound3->FrameSequencerStep(FrameSequencerStep);
			Sound4->FrameSequencerStep(FrameSequencerStep);
		}
	}
	else
	{
		NR50Changed(0, true);
		NR51Changed(0, true);
		
		Sound1->NR52Changed(value);
		Sound2->NR52Changed(value);
		Sound3->NR52Changed(value);
		Sound4->NR52Changed(value);
	}

	//Only all sound on/off can be changed
	AllSoundEnabled = value >> 7;
}
