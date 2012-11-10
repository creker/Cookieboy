#include "CookieboySoundUnit4.h"
#include "CookieboySound.h"

Cookieboy::SoundUnit4::SoundUnit4(const bool &_CGB, Sound &soundController):
CGB(_CGB),
SoundController(soundController),
LengthCounter(0x3F, StatusBit, 0xFF)
{
	Reset();
}

Cookieboy::SoundUnit4::~SoundUnit4()
{
}

void Cookieboy::SoundUnit4::TimerStep(DWORD clockDelta)
{
	lfsr.Step(clockDelta);
}

void Cookieboy::SoundUnit4::FrameSequencerStep(BYTE sequencerStep)
{
	Envelope.Step(sequencerStep);
	LengthCounter.Step(sequencerStep);
}

short Cookieboy::SoundUnit4::GetWaveLeftOutput()
{
	short leftSwitch = (SoundController.GetNR51() >> 7) & 0x1;
	short masterVolume = (SoundController.GetNR50() >> 4) & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return lfsr.GetOutput() * envelopeValue * (masterVolume + 1) * leftSwitch * StatusBit;
}

short Cookieboy::SoundUnit4::GetWaveRightOutput()
{
	short rightSwitch = (SoundController.GetNR51() >> 3) & 0x1;
	short masterVolume = SoundController.GetNR50() & 0x7;
	short envelopeValue = Envelope.GetEnvelopeValue();

	return lfsr.GetOutput() * envelopeValue * (masterVolume + 1) * rightSwitch * StatusBit;
}

void Cookieboy::SoundUnit4::Reset()
{
	NR41Changed(0, true);
	NR42Changed(0, true);
	NR43Changed(0, true);
	NR44Changed(0, true);

	lfsr.Reset();
	Envelope.Reset();
	LengthCounter.Reset();

	StatusBit = 0;
}

void Cookieboy::SoundUnit4::EmulateBIOS()
{
	Reset();

	NR41Changed(0xFF, true);
}

//Sound length
void Cookieboy::SoundUnit4::NR41Changed(BYTE value, bool override)
{
	if (CGB && !SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	//While all sound off only length can be written
	LengthCounter.NRX1Changed(value);
}

//Envelope function control
void Cookieboy::SoundUnit4::NR42Changed(BYTE value, bool override)
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

//LFSR control
void Cookieboy::SoundUnit4::NR43Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	lfsr.NR43Changed(value);
}

//initial, counter/consecutive mode
void Cookieboy::SoundUnit4::NR44Changed(BYTE value, bool override)
{
	if (!SoundController.GetAllSoundEnabled() && !override)
	{
		return;
	}

	NR44 = value;

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

	lfsr.NR44Changed(value);
	Envelope.NRX4Changed(value);
	LengthCounter.NRX4Changed(value);
}

void Cookieboy::SoundUnit4::NR52Changed(BYTE value)
{
	if (!(value >> 7))
	{
		StatusBit = 0;

		if (CGB)
		{
			NR41Changed(0, true);
		}
		NR42Changed(0, true);
		NR43Changed(0, true);
		NR44Changed(0, true);
	}
}