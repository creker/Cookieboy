#ifndef COOKIEBOYMBC3_H
#define COOKIEBOYMBC3_H

#include "CookieboyDefs.h"
#include "CookieboyMBC.h"
#include <ctime>

namespace Cookieboy
{

/*
Beside for the ability to access up to 2MB ROM (128 banks), and 32KB RAM (4 banks), the MBC3 also includes a built-in Real Time Clock (RTC).
The RTC requires an external 32.768 kHz Quartz Oscillator, and an external battery (if it should continue to tick when the gameboy is turned off).

0000-3FFF - ROM Bank 00 (Read Only)
			Same as for MBC1.

4000-7FFF - ROM Bank 01-7F (Read Only)
			Same as for MBC1, except that accessing banks 20h, 40h, and 60h is supported now.

A000-BFFF - RAM Bank 00-03, if any (Read/Write)
A000-BFFF - RTC Register 08-0C (Read/Write)
			Depending on the current Bank Number/RTC Register selection (see below), this memory space is used to access an 8KByte external RAM Bank,
			or a single RTC Register.

0000-1FFF - RAM and Timer Enable (Write Only)
			Mostly the same as for MBC1, a value of 0Ah will enable reading and writing to external RAM - and to the RTC Registers!
			A value of 00h will disable either.

2000-3FFF - ROM Bank Number (Write Only)
			Same as for MBC1, except that the whole 7 bits of the RAM Bank Number are written directly to this address.
			As for the MBC1, writing a value of 00h, will select Bank 01h instead. All other values 01-7Fh select the corresponding ROM Banks.

4000-5FFF - RAM Bank Number - or - RTC Register Select (Write Only)
			As for the MBC1s RAM Banking Mode, writing a value in range for 00h-03h maps the corresponding external RAM Bank (if any) into memory at A000-BFFF.
			When writing a value of 08h-0Ch, this will map the corresponding RTC register into memory at A000-BFFF. 
			That register could then be read/written by accessing any address in that area, typically that is done by using address A000.

6000-7FFF - Latch Clock Data (Write Only)
			When writing 00h, and then 01h to this register, the current time becomes latched into the RTC registers. 
			The latched data will not change until it becomes latched again, by repeating the write 00h->01h procedure.
			This is supposed for <reading> from the RTC registers. It is proof to read the latched (frozen) time from the RTC registers, 
			while the clock itself continues to tick in background.

The Clock Counter Registers
  08h  RTC S   Seconds   0-59 (0-3Bh)
  09h  RTC M   Minutes   0-59 (0-3Bh)
  0Ah  RTC H   Hours     0-23 (0-17h)
  0Bh  RTC DL  Lower 8 bits of Day Counter (0-FFh)
  0Ch  RTC DH  Upper 1 bit of Day Counter, Carry Bit, Halt Flag
        Bit 0  Most significant bit of Day Counter (Bit 8)
        Bit 6  Halt (0=Active, 1=Stop Timer)
        Bit 7  Day Counter Carry Bit (1=Counter Overflow)
The Halt Flag is supposed to be set before <writing> to the RTC Registers.

The Day Counter
The total 9 bits of the Day Counter allow to count days in range from 0-511 (0-1FFh). The Day Counter Carry Bit becomes set when this value overflows.
In that case the Carry Bit remains set until the program does reset it.
*/
class MBC3 : public MBC
{
public:
	enum MBC3ModesEnum
	{
		RAMBankMapping = 0,
		RTCRegisterMapping = 1
	};

	enum RTCRegistersEnum
	{
		RTC_S = 0,
		RTC_M = 1,
		RTC_H = 2,
		RTC_DL = 3,
		RTC_DH = 4
	};

	MBC3(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		Mode = RAMBankMapping;
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
		RAMRTCEnabled = false;
		LastLatchWrite = 0xFF;
		BaseTime = std::time(NULL);
		HaltTime = std::time(NULL);
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//RAM/RTC registers enable/disable
		case 0x0000:
		case 0x1000:
			RAMRTCEnabled = (value & 0x0F) == 0x0A;
			break;

		//ROM bank switching
		case 0x2000:
		case 0x3000:
			ROMOffset = value & 0x7F;
			ROMOffset %= ROMSize;

			if (ROMOffset == 0)
			{
				ROMOffset = 1;
			}

			ROMOffset *= ROMBankSize;
			break;

		//RAM bank/RTC register switching
		case 0x4000:
		case 0x5000:
			if ((value & 0xF) <= 0x3)
			{
				Mode = RAMBankMapping;

				RAMOffset = value & 0xF;
				RAMOffset %= RAMSize;
				RAMOffset *= RAMBankSize;
			}
			else if ((value & 0xF) >= 0x8 && (value & 0xF) <= 0xC)
			{
				Mode = RTCRegisterMapping;
				SelectedRTCRegister = (value & 0xF) - 0x8;
			}
			break;

		//RTC data latch
		case 0x6000:
		case 0x7000:
			if (Mode == RTCRegisterMapping)
			{
				if (LastLatchWrite == 0 && value == 1)
				{
					LatchRTCData();
				}
				LastLatchWrite = value;
			}
			break;

		//Switchable RAM bank/RTC register
		case 0xA000:
		case 0xB000:
			if (RAMRTCEnabled)
			{
				if (Mode == RAMBankMapping)
				{
					RAMBanks[RAMOffset + (addr - 0xA000)] = value;
				}
				else if (Mode == RTCRegisterMapping)
				{
					if (SelectedRTCRegister == RTC_DH)
					{
						BYTE oldDH = RTCRegisters[SelectedRTCRegister];
						RTCRegisters[SelectedRTCRegister] = value;
						ReloadBaseTime();

						//Halt flag stops timer clock
						if ((oldDH ^ value) & 0x40)
						{
							if (value & 0x40)
							{
								HaltTime = std::time(NULL);
							}
							else
							{
								BaseTime += std::time(NULL) - HaltTime;
							}
						}
					}
					else
					{
						RTCRegisters[SelectedRTCRegister] = value;
						ReloadBaseTime();
					}
				}
			}
			break;
		}
	}

	virtual BYTE Read(WORD addr)
	{
		switch (addr & 0xF000)
		{
		//ROM bank 0
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
			return ROM[addr];

		//ROM bank 1
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			return ROM[ROMOffset + (addr - 0x4000)];

		//Switchable RAM bank/RTC register
		case 0xA000:
		case 0xB000:
			if (RAMRTCEnabled)
			{
				if (Mode == RAMBankMapping)
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
				else
				{
					return RTCRegisters[SelectedRTCRegister];
				}
			}
		}

		return 0xFF;
	}

	virtual bool SaveRAM(const char *path, DWORD RAMSize)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "wb");
		if (file == NULL)
		{
			return true;
		}

		fwrite(RAMBanks, RAMSize, 1, file);
		fwrite(&BaseTime, sizeof(BaseTime), 1, file);

		fwrite(&HaltTime, sizeof(HaltTime), 1, file);
		fwrite(RTCRegisters, sizeof(BYTE), 5, file);

		fflush(file);
		fclose(file);

		return false;
	}

	virtual bool LoadRAM(const char *path, DWORD RAMSize)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "rb");
		if (file == NULL)
		{
			return true;
		}

		fread(RAMBanks, RAMSize, 1, file);

		fread(&HaltTime, sizeof(HaltTime), 1, file);
		fread(RTCRegisters, sizeof(BYTE), 5, file);

		fflush(file);
		fclose(file);

		return false;
	}

private:
	void LatchRTCData()
	{
		time_t timeDiff = ((RTCRegisters[RTC_DH] >> 6) & 0x1) ? HaltTime - BaseTime : std::time(NULL) - BaseTime;

		if (timeDiff > 0x1FF * 86400)
		{
			while (timeDiff > 0x1FF * 86400)
			{
				timeDiff -= 0x1FF * 86400;
				BaseTime += 0x1FF * 86400;
			}

			RTCRegisters[RTC_DH] |= 0x80;//Day counter overflow
		}

		int dayCounter = (timeDiff / 86400) & 0x1FF;
		RTCRegisters[RTC_DL] = dayCounter & 0xFF;
		RTCRegisters[RTC_DH] |= dayCounter >> 8;
		timeDiff %= 86400;

		RTCRegisters[RTC_H] = (timeDiff / 3600) & 0xFF;
		timeDiff %= 3600;

		RTCRegisters[RTC_M] = (timeDiff / 60) & 0xFF;
		timeDiff %= 60;

		RTCRegisters[RTC_S] = timeDiff & 0xFF;
	}

	void ReloadBaseTime()
	{
		if (RTCRegisters[RTC_DH] & 0x40)
		{
			BaseTime = HaltTime;
		}
		else
		{
			BaseTime = std::time(NULL);
		}

		int dayCounter = (RTCRegisters[RTC_DH] << 8) | RTCRegisters[RTC_DL];
		BaseTime -= dayCounter * 86400;

		BaseTime -= RTCRegisters[RTC_H] * 3600;
		BaseTime -= RTCRegisters[RTC_M] * 60;
		BaseTime -= RTCRegisters[RTC_S];
	}

	BYTE RTCRegisters[5];
	BYTE SelectedRTCRegister;

	MBC3ModesEnum Mode;
	bool RAMRTCEnabled;

	std::time_t BaseTime;
	std::time_t HaltTime;
	BYTE LastLatchWrite;
};

}

#endif