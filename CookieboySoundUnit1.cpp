#include "CookieboySoundUnit1.h"
#include "CookieboySound.h"

Cookieboy::SoundUnit1::SoundUnit1(const bool &_CGB, Sound &soundController):
CGB(_CGB),
SoundController(soundController),
LengthCounter(0x3F, StatusBit),
Sweep(NR13, NR14, StatusBit, Duty)
{
	Reset();
}

Cookieboy::SoundUnit1::~SoundUnit1()
{
}

void Cookieboy::SoundUnit1::TimerStep(DWORD clockDelta)
{
	Duty.Step(clockDelta);
}

void Cookieboy::SoundUnit1::FrameSequencerStep(BYTE sequencerStep)
{
	Sweep.Step(sequencerStep);
	Envelope.Step(sequencerStep);
	LengthCounter.Step(sequencerStep);
}

short Cookieboy::SoundUnit1::GetWaveLeftOutput()
{
	short leftSwitch = (SoundController.GetNR51() >> 4) & 0x1;
	short masterVolume = (SoundController.GetNR50() >> 4) & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return Duty.GetOutput() * envelopeValue * (masterVolume + 1) * leftSwitch * StatusBit;
}

short Cookieboy::SoundUnit1::GetWaveRightOutput()
{
	short rightSwitch = SoundController.GetNR51() & 0x1;
	short masterVolume = SoundController.GetNR50() & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return Duty.GetOutput() * envelopeValue * (masterVolume + 1) * rightSwitch * StatusBit;
}

void Cookieboy::SoundUnit1::Reset()
{
	NR10Changed(0, true);
	NR11Changed(0, true);
	NR12Changed(0, true);
	NR13Changed(0, true);
	NR14Changed(0, true);

	Duty.Reset();
	Sweep.Reset();
	Envelope.Reset();
	LengthCounter.Reset();

	StatusBit = 0;

	WaveOutput = 0;
}

void Cookieboy::SoundUnit1::EmulateBIOS()
{
	Reset();

	//On DMG after BIOS executes NR52 containts 0xF1 - status bit for sound 1 is set 
	StatusBit = 1;
	NR10Changed(0x80, true);
	NR11Changed(0xBF, true);
	NR12Changed(0xF3, true);
	NR14Changed(0xBF, true);
}

//Sweep function control
void Cookieboy::SoundUnit1::NR10Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	Sweep.NRX0Changed(value);
}

//Wave pattern duty, Sound length
void Cookieboy::SoundUnit1::NR11Changed(BYTE value, bool override)
{
	if (CGB && !SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		//While all sound off only length can be written
		value &= LengthCounter.GetLengthMask();
	}
	
	NR11 = value;

	Duty.NRX1Changed(value);
	LengthCounter.NRX1Changed(value);
}

//Envelope function control
void Cookieboy::SoundUnit1::NR12Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}
	
	Envelope.NRX2Changed(value);

	if (Envelope.DisablesSound())
	{
		StatusBit = 0;
	}
}

//Low frequency bits
void Cookieboy::SoundUnit1::NR13Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR13 = value;

	Duty.NRX3Changed(value);
}

//initial, counter/consecutive mode, High frequency bits
void Cookieboy::SoundUnit1::NR14Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}
	
	NR14 = value;

	//If channel initial set
	if (value >> 7)
	{
		StatusBit = 1;
	}
	//In some cases envelope unit disables sound unit
	if (Envelope.DisablesSound())
	{
		StatusBit = 0;
	}

	Duty.NRX4Changed(value);
	Sweep.NRX4Changed(value);
	Envelope.NRX4Changed(value);
	LengthCounter.NRX4Changed(value);
}

void Cookieboy::SoundUnit1::NR52Changed(BYTE value)
{
	if (!(value >> 7))
	{
		StatusBit = 0;
		
		NR10Changed(0, true);
		if (CGB)
		{
			NR11Changed(0, true);
		}
		else
		{
			NR11Changed(NR11 & LengthCounter.GetLengthMask(), true);
		}
		NR12Changed(0, true);
		NR13Changed(0, true);
		NR14Changed(0, true);
	}
}