#include "CookieboySoundUnit2.h"
#include "CookieboySound.h"

Cookieboy::SoundUnit2::SoundUnit2(Sound &soundController):
SoundController(soundController),
LengthCounter(0x3F, StatusBit)
{
	Reset();
}

Cookieboy::SoundUnit2::~SoundUnit2()
{
}

void Cookieboy::SoundUnit2::TimerStep(DWORD clockDelta)
{
	Duty.Step(clockDelta);
}

void Cookieboy::SoundUnit2::FrameSequencerStep(BYTE sequencerStep)
{
	Envelope.Step(sequencerStep);
	LengthCounter.Step(sequencerStep);
}

short Cookieboy::SoundUnit2::GetWaveLeftOutput()
{
	short leftSwitch = (SoundController.GetNR51() >> 5) & 0x1;
	short masterVolume = (SoundController.GetNR50() >> 4) & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return Duty.GetOutput() * envelopeValue * (masterVolume + 1) * leftSwitch * StatusBit;
}

short Cookieboy::SoundUnit2::GetWaveRightOutput()
{
	short rightSwitch = (SoundController.GetNR51() >> 1) & 0x1;
	short masterVolume = SoundController.GetNR50() & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return Duty.GetOutput() * envelopeValue * (masterVolume + 1) * rightSwitch * StatusBit;
}

void Cookieboy::SoundUnit2::Reset()
{
	NR21Changed(0, true);
	NR22Changed(0, true);
	NR23Changed(0, true);
	NR24Changed(0, true);

	StatusBit = 0;
}

void Cookieboy::SoundUnit2::EmulateBIOS()
{
	Reset();

	NR21Changed(0x80, true);
	NR22Changed(0xF3, true);
}

//Wave pattern duty, Sound length
void Cookieboy::SoundUnit2::NR21Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		//While all sound off only length can be written
		value &= LengthCounter.GetLengthMask();
	}

	NR21 = value;

	Duty.NRX1Changed(value);
	LengthCounter.NRX1Changed(value);
}

//Envelope function control
void Cookieboy::SoundUnit2::NR22Changed(BYTE value, bool override)
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
void Cookieboy::SoundUnit2::NR23Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR23 = value;

	Duty.NRX3Changed(value);
}

//initial, counter/consecutive mode, High frequency bits
void Cookieboy::SoundUnit2::NR24Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR24 = value;

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
	Envelope.NRX4Changed(value);
	LengthCounter.NRX4Changed(value);
}

void Cookieboy::SoundUnit2::NR52Changed(BYTE value)
{
	if (!(value >> 7))
	{
		StatusBit = 0;

		NR21Changed(NR21 & LengthCounter.GetLengthMask(), true);
		NR22Changed(0, true);
		NR23Changed(0, true);
		NR24Changed(0, true);
	}
}