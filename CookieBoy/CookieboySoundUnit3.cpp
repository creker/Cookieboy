#include "CookieboySoundUnit3.h"
#include "CookieboySound.h"
#include <memory.h>

Cookieboy::SoundUnit3::SoundUnit3(Sound &soundController):
SoundController(soundController),
LengthCounter(0xFF, StatusBit)
{
	Reset();
}

Cookieboy::SoundUnit3::~SoundUnit3()
{
}

void Cookieboy::SoundUnit3::TimerStep(DWORD clockDelta)
{
	ClockCounter += clockDelta;

	if (ClockCounter >= Period)
	{
		int passedPeriods = ClockCounter / Period;
		ClockCounter %= Period;
		
		SampleIndex = (SampleIndex + passedPeriods) & 0x1F;
		SampleBuffer = WaveRAM[SampleIndex >> 1];

		Output = (SampleBuffer >> (4 * ((~SampleIndex) & 0x1))) & 0xF;
		switch ((NR32 >> 5) & 0x3)
		{
		case 0x0://mute
			Output = 0;
			break;

		case 0x1://1:1
			break;

		case 0x2://1:2
			Output >>= 1;
			break;

		case 0x3://1:4
			Output >>= 2;
			break;
		}
	}
}

void Cookieboy::SoundUnit3::FrameSequencerStep(BYTE sequencerStep)
{
	LengthCounter.Step(sequencerStep);
}

short Cookieboy::SoundUnit3::GetWaveLeftOutput()
{
	short leftSwitch = (SoundController.GetNR51() >> 6) & 0x1;
	short masterVolume = (SoundController.GetNR50() >> 4) & 0x7;

	return Output * (masterVolume + 1) * leftSwitch * StatusBit;
}

short Cookieboy::SoundUnit3::GetWaveRightOutput()
{
	short rightSwitch = (SoundController.GetNR51() >> 2) & 0x1;
	short masterVolume = SoundController.GetNR50() & 0x7;

	return Output * (masterVolume + 1) * rightSwitch * StatusBit;
}

void Cookieboy::SoundUnit3::Reset()
{
	NR30Changed(0, true);
	NR31Changed(0, true);
	NR32Changed(0, true);
	NR33Changed(0, true);
	NR34Changed(0, true);
	memset(WaveRAM, 0, 0x10);

	LengthCounter.Reset();

	StatusBit = 0;

	SampleIndex = 0;
	SampleBuffer = 0;
	ClockCounter = 0;
}

void Cookieboy::SoundUnit3::EmulateBIOS()
{
	Reset();
}

//While sound 1 is on Wave pattern RAM can be read or written only when sound 3 reads samples from it.
BYTE Cookieboy::SoundUnit3::GetWaveRAM(BYTE pos)
{ 
	return WaveRAM[pos];
	/*if (StatusBit)
	{
		if (canAccessWaveRAM)
		{
			return WaveRAM[SampleIndex >> 1];
		}
		else
		{
			return 0xFF;
		}
	}
	else
	{
		return WaveRAM[pos];
	}*/
}

void Cookieboy::SoundUnit3::WaveRAMChanged(BYTE pos, BYTE value)
{ 
	WaveRAM[pos] = value;
	/*if (StatusBit)
	{
		if (canAccessWaveRAM)
		{
			WaveRAM[SampleIndex >> 1] = value;
		}
	}
	else
	{
		WaveRAM[pos] = value;
	}*/
}

//Sound on/off
void Cookieboy::SoundUnit3::NR30Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR30 = value;

	if (!(value >> 7))
	{
		StatusBit = 0;
	}
}

//Sound length
void Cookieboy::SoundUnit3::NR31Changed(BYTE value, bool override)
{
	//While all sound off only length can be written
	LengthCounter.NRX1Changed(value);
}

//Output level
void Cookieboy::SoundUnit3::NR32Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR32 = value;
}

//Frequency low bits
void Cookieboy::SoundUnit3::NR33Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}
	
	NR33 = value;

	CalculatePeriod();
}

//initial, counter/consecutive mode, High frequency bits
void Cookieboy::SoundUnit3::NR34Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}
	
	NR34 = value;

	CalculatePeriod();

	//If channel initial set
	if (value & NR30 & 0x80)
	{
		/*if (canAccessWaveRAM)
		{
			if ((SampleIndex >> 1) < 4)
			{
				WaveRAM[0] = WaveRAM[SampleIndex >> 1];
			}
			else
			{
				int alignedIndex = (SampleIndex >> 1) & 0xFC;
				WaveRAM[0] = WaveRAM[alignedIndex];
				WaveRAM[1] = WaveRAM[alignedIndex + 1];
				WaveRAM[2] = WaveRAM[alignedIndex + 2];
				WaveRAM[3] = WaveRAM[alignedIndex + 3];
			}
		}*/
		
		//Switching to first sample
		//Sample buffer is not updated until next timer clock
		SampleIndex = 0;

		//If sound 3 On
		StatusBit = 1;
	}

	LengthCounter.NRX4Changed(value);
}

void Cookieboy::SoundUnit3::NR52Changed(BYTE value)
{
	if (!(value >> 7))
	{
		StatusBit = 0;

		NR30Changed(0, true);
		//Length register is not changed
		NR32Changed(0, true);
		NR33Changed(0, true);
		NR34Changed(0, true);
	}
}