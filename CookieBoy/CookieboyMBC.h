#ifndef COOKIEBOYMBC_H
#define COOKIEBOYMBC_H

#include "CookieboyDefs.h"
#include <stdio.h>

namespace Cookieboy
{

const int ROMBankSize = 0x4000;
const int RAMBankSize = 0x2000;

class MBC
{
public:
	virtual void Write(WORD addr, BYTE value) = 0;
	virtual BYTE Read(WORD addr) = 0;
	
	virtual bool SaveRAM(const char *path, DWORD RAMSize)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "wb");
		if (file == NULL)
		{
			return true;
		}

		fwrite(RAMBanks, RAMSize, 1, file);

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

		fflush(file);
		fclose(file);

		return false;
	}

protected:
	MBC(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : ROM(ROM), ROMSize(ROMSize), RAMBanks(RAMBanks), RAMSize(RAMSize) {}

	BYTE *ROM;
	BYTE *RAMBanks;

	DWORD ROMOffset;
	DWORD RAMOffset;

	DWORD ROMSize;
	DWORD RAMSize;
};

}

#endif