#ifndef COOKIEBOYMBC_MMM01_H
#define COOKIEBOYMBC_MMM01_H

#include "CookieboyDefs.h"
#include "CookieboyMBC.h"

namespace Cookieboy
{

class MBC_MMM01 : public MBC
{
public:
	enum MMM01ModesEnum
	{
		MMM01MODE_ROMONLY = 0,
		MMM01MODE_BANKING = 1
	};

	MBC_MMM01(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
		RAMEnabled = false;
		Mode = MMM01MODE_ROMONLY;
		ROMBase = 0x0;
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//Modes switching
		case 0x0000:
		case 0x1000:
			if (Mode == MMM01MODE_ROMONLY)
			{
				Mode = MMM01MODE_BANKING;
			}
			else
			{
				RAMEnabled = (value & 0x0F) == 0x0A;
			}
			break;

		//ROM bank switching
		case 0x2000:
		case 0x3000:
			if (Mode == MMM01MODE_ROMONLY)
			{
				ROMBase = value & 0x3F;
				ROMBase %= ROMSize - 2;
				ROMBase *= ROMBankSize;
			}
			else
			{
				if (value + ROMBase / ROMBankSize > ROMSize - 3)
				{
					value = (ROMSize - 3 - ROMBase / ROMBankSize) & 0xFF;
				}

				ROMOffset = value * ROMBankSize;
			}
			break;

		//RAM bank switching in banking mode
		case 0x4000:
		case 0x5000:
			if (Mode == MMM01MODE_BANKING)
			{
				value %= RAMSize;
				RAMOffset = value * RAMBankSize;
			}
			break;
		//Switchable RAM bank
		case 0xA000:
		case 0xB000:
			if (RAMEnabled)
			{
				RAMBanks[RAMOffset + (addr - 0xA000)] = value;
			}
			break;
		}
	}

	virtual BYTE Read(WORD addr)
	{
		if (Mode == MMM01MODE_ROMONLY)
		{
			switch (addr & 0xF000)
			{
			//ROM bank 0
			case 0x0000:
			case 0x1000:
			case 0x2000:
			case 0x3000:
			//ROM bank 1
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return ROM[addr];

			//Switchable RAM bank
			case 0xA000:
			case 0xB000:
				if (RAMEnabled)
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
			}
		}
		else
		{
			switch (addr & 0xF000)
			{
			//ROM bank 0
			case 0x0000:
			case 0x1000:
			case 0x2000:
			case 0x3000:
				return ROM[ROMBankSize * 2 + ROMBase + addr];

			//ROM bank 1
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return ROM[ROMBankSize * 2 + ROMBase + ROMOffset + (addr - 0x4000)];

			//Switchable RAM bank
			case 0xA000:
			case 0xB000:
				if (RAMEnabled)
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
			}
		}

		return 0xFF;
	}

private:
	bool RAMEnabled;
	MMM01ModesEnum Mode;	
	DWORD ROMBase;
};

}

#endif