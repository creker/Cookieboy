#ifndef COOKIEBOYROMINFO_H
#define COOKIEBOYROMINFO_H

#include "CookieboyDefs.h"

namespace Cookieboy
{

/*
An internal information area is located at 0100-014F in each cartridge. It contains the following values:
	0100-0103	This is the begin code execution point in a cart. Usually there is a NOP and 
				a JP instruction here but not always.
	0104-0133	Scrolling Nintendo graphic:
				CE ED 66 66 CC 0D 00 0B 03 73 00 83 00 0C 00 0D
				00 08 11 1F 88 89 00 0E DC CC 6E E6 DD DD D9 99
				BB BB 67 63 6E 0E EC CC DD DC 99 9F BB B9 33 3E
				( PROGRAM WON'T RUN IF CHANGED!!!)
	0134-0142	Title of the game in UPPER CASE ASCII. If it is less than 16 characters then 
				the remaining bytes are filled with 00's.
	0143		$80 = Color GB, $00 or other = not Color GB
	0144		Ascii hex digit, high nibble of licensee code (new).
	0145		Ascii hex digit, low nibble of licensee code (new).
	0146		GB/SGB Indicator (00 = GameBoy, 03 = Super GameBoy functions)
				(Super GameBoy functions won't work	if <> $03.)
	0147		Cartridge type:
				0-ROM ONLY				12-ROM+MBC3+RAM
				1-ROM+MBC1				13-ROM+MBC3+RAM+BATT
				2-ROM+MBC1+RAM			19-ROM+MBC5
				3-ROM+MBC1+RAM+BATT		1A-ROM+MBC5+RAM
				5-ROM+MBC2				1B-ROM+MBC5+RAM+BATT
				6-ROM+MBC2+BATTERY		1C-ROM+MBC5+RUMBLE
				8-ROM+RAM				1D-ROM+MBC5+RUMBLE+SRAM
				9-ROM+RAM+BATTERY		1E-ROM+MBC5+RUMBLE+SRAM+BATT
				B-ROM+MMM01				1F-Pocket Camera
				C-ROM+MMM01+SRAM		FD-Bandai TAMA5
				D-ROM+MMM01+SRAM+BATT	FE - Hudson HuC-3
				F-ROM+MBC3+TIMER+BATT	FF - Hudson HuC-1
				10-ROM+MBC3+TIMER+RAM+BATT
				11-ROM+MBC3
	0148		ROM size:
				0 - 256Kbit = 32KByte = 2 banks
				1 - 512Kbit = 64KByte = 4 banks
				2 - 1Mbit = 128KByte = 8 banks
				3 - 2Mbit = 256KByte = 16 banks
				4 - 4Mbit = 512KByte = 32 banks
				5 - 8Mbit = 1MByte = 64 banks
				6 - 16Mbit = 2MByte = 128 banks
				$52 - 9Mbit = 1.1MByte = 72 banks
				$53 - 10Mbit = 1.2MByte = 80 banks
				$54 - 12Mbit = 1.5MByte = 96 banks
	0149		RAM size:
				0 - None
				1 - 16kBit = 2kB = 1 bank
				2 - 64kBit = 8kB = 1 bank
				3 - 256kBit = 32kB = 4 banks
				4 - 1MBit =128kB =16 banks
	014A		Destination code:
				0 - Japanese
				1 - Non-Japanese
	014B		Licensee code (old):
				33 - Check 0144/0145 for Licensee code.
				79 - Accolade
				A4 - Konami
				(Super GameBoy function won't work if <> $33.)
	014C		Mask ROM Version number (Usually $00)
	014D		Complement check
				(PROGRAM WON'T RUN ON GB IF NOT CORRECT!!!)
				(It will run on Super GB, however, if incorrect.)
	014E-014F	Checksum (higher byte first) produced by adding all bytes of a cartridge except for two 
				checksum bytes and taking two lower bytes of the result. (GameBoy ignores this value.)
*/
struct ROMInfo
{
public:
	enum CartridgeTypesEnum
	{
		CART_ROM_ONLY =						0x00,
		CART_ROM_MBC1 =						0x01,
		CART_ROM_MBC1_RAM =					0x02,
		CART_ROM_MBC1_RAM_BATT =			0x03,
		CART_ROM_MBC2 =						0x05,
		CART_ROM_MBC2_BATT =				0x06,
		CART_ROM_RAM =						0x08,
		CART_ROM_RAM_BATT =					0x09,
		CART_ROM_MMM01 =					0x0B,
		CART_ROM_MMM01_SRAM =				0x0C,
		CART_ROM_MMM01_SRAM_BATT =			0x0D,
		CART_ROM_MBC3_TIMER_BATT =			0x0F,
		CART_ROM_MBC3_TIMER_RAM_BATT =		0x10,
		CART_ROM_MBC3 =						0x11,
		CART_ROM_MBC3_RAM =					0x12,
		CART_ROM_MBC3_RAM_BATT =			0x13,
		CART_ROM_MBC5 =						0x19,
		CART_ROM_MBC5_RAM =					0x1A,
		CART_ROM_MBC5_RAM_BATT =			0x1B,
		CART_ROM_MBC5_RUMBLE =				0x1C,
		CART_ROM_MBC5_RUMBLE_SRAM =			0x1D,
		CART_ROM_MBC5_RUMBLE_SRAM_BATT =	0x1E,
		CART_POCKET_CAMERA =				0x1F,
		CART_BANDAI_TAMA5 =					0xFD,
		CART_HUDSON_HUC3 =					0xFE,
		CART_HUDSON_HUC1 =					0xFF,
		CART_UNKNOWN =						0x100
	};

	enum MMCTypesEnum
	{
		MMC_ROMONLY = 0x00,
		MMC_MBC1 = 0x01,
		MMC_MBC2 = 0x02,
		MMC_MBC3 = 0x03,
		MMC_MBC5 = 0x04,
		MMC_MMM01 = 0x05,
		MMC_UNKNOWN = 0x06
	};

	enum NewLicensesEnum
	{
		NEWLICENSE_NONE =					0x00,
		NEWLICENSE_NINTENDO1 =				0x01,
		NEWLICENSE_CAPCOM =					0x08,
		NEWLICENSE_ELECTRONIC_ARTS1 =		0x13,
		NEWLICENSE_HUDSONSOFT =				0x18,
		NEWLICENSE_B_AI =					0x19,
		NEWLICENSE_KSS =					0x20,
		NEWLICENSE_POW =					0x22,
		NEWLICENSE_PCM_COMPLETE =			0x24,
		NEWLICENSE_SAN_X =					0x25,
		NEWLICENSE_KEMCO_JAPAN =			0x28,
		NEWLICENSE_SETA =					0x29,
		NEWLICENSE_VIACOM =					0x30,
		NEWLICENSE_NINTENDO2 =				0x31,
		NEWLICENSE_BANDIA =					0x32,
		NEWLICENSE_OCEAN_ACCLAIM1 =			0x33,
		NEWLICENSE_KONAMI1 =				0x34,
		NEWLICENSE_HECTOR =					0x35,
		NEWLICENSE_TAITO =					0x37,
		NEWLICENSE_HUDSON =					0x38,
		NEWLICENSE_BANPRESTO =				0x39,
		NEWLICENSE_UBISOFT =				0x41,
		NEWLICENSE_ATLUS =					0x42,
		NEWLICENSE_MALIBU =					0x44,
		NEWLICENSE_ANGEL =					0x46,
		NEWLICENSE_PULLETPROOF =			0x47,
		NEWLICENSE_IREM =					0x49,
		NEWLICENSE_ABSOLUTE =				0x50,
		NEWLICENSE_ACCLAIM =				0x51,
		NEWLICENSE_ACTIVISION =				0x52,
		NEWLICENSE_AMERICAN_SUMMY =			0x53,
		NEWLICENSE_KONAMI2 =				0x54,
		NEWLICENSE_HITECH_ENTERTAINMENT =	0x55,
		NEWLICENSE_LJN =					0x56,
		NEWLICENSE_MATCHBOX =				0x57,
		NEWLICENSE_MATTEL =					0x58,
		NEWLICENSE_MILTON_BRADLEY =			0x59,
		NEWLICENSE_TITUS =					0x60,
		NEWLICENSE_VIRGIN =					0x61,
		NEWLICENSE_LUCASARTS =				0x64,
		NEWLICENSE_OCEAN =					0x67,
		NEWLICENSE_ELECTRONIC_ARTS2 =		0x69,
		NEWLICENSE_INFOGRAMES =				0x70,
		NEWLICENSE_INTERPLAY =				0x71,
		NEWLICENSE_BRODERBUND =				0x72,
		NEWLICENSE_SCULPTURED =				0x73,
		NEWLICENSE_SCI =					0x75,
		NEWLICENSE_T_HQ =					0x78,
		NEWLICENSE_ACCOLADE =				0x79,
		NEWLICENSE_MISAWA =					0x80,
		NEWLICENSE_LOZC =					0x83,
		NEWLICENSE_TOKUMA_SHOTEN_I =		0x86,
		NEWLICENSE_TSUKUDA_ORI =			0x87,
		NEWLICENSE_CHUN_SOFT =				0x91,
		NEWLICENSE_VIDEO_SYSTEM =			0x92,
		NEWLICENSE_OCEAN_ACCLAIM2 =			0x93,
		NEWLICENSE_VARIE =					0x95,
		NEWLICENSE_YONEZAWAS_PAL =			0x96,
		NEWLICENSE_KANEKO =					0x97,
		NEWLICENSE_PACK_IN_SOFT =			0x99,
		NEWLICENSE_UNKNOWN =				0x9A
	};

	enum OldLicensesEnum
	{	
		OLDLICENSE_NONE =					0x00,
		OLDLICENSE_NINTENDO1 =				0x01,
		OLDLICENSE_CAPCOM1 =				0x08,
		OLDLICENSE_HOT_B =					0x09,
		OLDLICENSE_JALECO1 =				0x0A,
		OLDLICENSE_COCONUTS =				0x0B,
		OLDLICENSE_ELITE_SYSTEMS2 =			0x0C,
		OLDLICENSE_ELECTRONIC_ARTS1 =		0x13,
		OLDLICENSE_HUDSONSOFT =				0x18,
		OLDLICENSE_ITC_ENTERTAINMENT =		0x19,
		OLDLICENSE_YANOMAN =				0x1A,
		OLDLICENSE_CLARY =					0x1D,
		OLDLICENSE_VIRGIN2 =				0x1F,
		OLDLICENSE_PCM_COMPLETE =			0x24,
		OLDLICENSE_SAN_X =					0x25,
		OLDLICENSE_KOTOBUKI_SYSTEMS =		0x28,
		OLDLICENSE_SETA =					0x29,
		OLDLICENSE_INFOGRAMES2 =			0x30,
		OLDLICENSE_NINTENDO2 =				0x31,
		OLDLICENSE_BANDAI1 =				0x32,
		OLDLICENSE_SEEABOVE =				0x33,
		OLDLICENSE_KONAMI2 =				0x34,
		OLDLICENSE_HECTOR =					0x35,
		OLDLICENSE_CAPCOM2 =				0x38,
		OLDLICENSE_BANPRESTO2 =				0x39,
		OLDLICENSE_ENTERTAINMENT_I =		0x3C,
		OLDLICENSE_GREMLIN =				0x3E,
		OLDLICENSE_UBISOFT =				0x41,
		OLDLICENSE_ATLUS1 =					0x42,
		OLDLICENSE_MALIBU2 =				0x44,
		OLDLICENSE_ANGEL2 =					0x46,
		OLDLICENSE_SPECTRUM_HOLOBY =		0x47,
		OLDLICENSE_IREM =					0x49,
		OLDLICENSE_VIRGIN3 =				0x4A,
		OLDLICENSE_MALIBU1 =				0x4D,
		OLDLICENSE_US_GOLD =				0x4F,
		OLDLICENSE_ABSOLUTE =				0x50,
		OLDLICENSE_ACCLAIM1 =				0x51,
		OLDLICENSE_ACTIVISION =				0x52,
		OLDLICENSE_AMERICAN_SAMMY =			0x53,
		OLDLICENSE_GAMETEK =				0x54,
		OLDLICENSE_PARKPLACE =				0x55,
		OLDLICENSE_LJN3 =					0x56,
		OLDLICENSE_MATCHBOX =				0x57,
		OLDLICENSE_MILTON_BRADLEY =			0x59,
		OLDLICENSE_MINDSCAPE =				0x5A,
		OLDLICENSE_ROMSTAR =				0x5B,
		OLDLICENSE_NAXAT_SOFT2 =			0x5C,
		OLDLICENSE_TRADEWEST =				0x5D,
		OLDLICENSE_TITUS =					0x60,
		OLDLICENSE_VIRGIN1 =				0x61,
		OLDLICENSE_OCEAN =					0x67,
		OLDLICENSE_ELECTRONIC_ARTS2 =		0x69,
		OLDLICENSE_ELITE_SYSTEMS1 =			0x6E,
		OLDLICENSE_ELECTRO_BRAIN =			0x6F,
		OLDLICENSE_INFOGRAMES1 =			0x70,
		OLDLICENSE_INTERPLAY =				0x71,
		OLDLICENSE_BRODERBUND1 =			0x72,
		OLDLICENSE_SCULPTERED_SOFT =		0x73,
		OLDLICENSE_THE_SALES_CURVE =		0x75,
		OLDLICENSE_THQ =					0x78,
		OLDLICENSE_ACCOLADE =				0x79,
		OLDLICENSE_TRIFFIX_ENTERTAINMENT =	0x7A,
		OLDLICENSE_MICROPROSE =				0x7C,
		OLDLICENSE_KEMCO1 =					0x7F,
		OLDLICENSE_MISAWA_ENTERTAINMENT =	0x80,
		OLDLICENSE_LOZC =					0x83,
		OLDLICENSE_TOKUMA_SHOTEN_I1 =		0x86,
		OLDLICENSE_BULLET_PROOFSOFTWARE =	0x8B,
		OLDLICENSE_VICTOKAI =				0x8C,
		OLDLICENSE_APE =					0x8E,
		OLDLICENSE_IMAX =					0x8F,
		OLDLICENSE_CHUNSOFT =				0x91,
		OLDLICENSE_VIDEOSYSTEM =			0x92,
		OLDLICENSE_TSUBURAVA =				0x93,
		OLDLICENSE_VARIE1 =					0x95,
		OLDLICENSE_YONEZAWAS_PAL =			0x96,
		OLDLICENSE_KANEKO =					0x97,
		OLDLICENSE_ARC =					0x99,
		OLDLICENSE_NIHON_BUSSAN =			0x9A,
		OLDLICENSE_TECMO =					0x9B,
		OLDLICENSE_IMAGINEER =				0x9C,
		OLDLICENSE_BANPRESTO1 =				0x9D,
		OLDLICENSE_NOVA =					0x9F,
		OLDLICENSE_HORI_ELECTRIC =			0xA1,
		OLDLICENSE_BANDAI2 =				0xA2,
		OLDLICENSE_KONAMI1 =				0xA4,
		OLDLICENSE_KAWADA =					0xA6,
		OLDLICENSE_TAKARA =					0xA7,
		OLDLICENSE_TECHNOS_JAPAN =			0xA9,
		OLDLICENSE_BRODERBUND2 =			0xAA,
		OLDLICENSE_TOEI_ANIMATION =			0xAC,
		OLDLICENSE_TOHO =					0xAD,
		OLDLICENSE_NAMCO =					0xAF,
		OLDLICENSE_ACCLAIM2 =				0xB0,
		OLDLICENSE_ASCII_OR_NEXOFT =		0xB1,
		OLDLICENSE_BANDAI3 =				0xB2,
		OLDLICENSE_ENIX =					0xB4,
		OLDLICENSE_HAL =					0xB6,
		OLDLICENSE_SNK =					0xB7,
		OLDLICENSE_PONY_CANYON =			0xB9,
		OLDLICENSE_CULTURE_BRAIN_O =		0xBA,
		OLDLICENSE_SUNSOFT =				0xBB,
		OLDLICENSE_SONY_IMAGESOFT =			0xBD,
		OLDLICENSE_SAMMY =					0xBF,
		OLDLICENSE_TAITO2 =					0xC0,
		OLDLICENSE_KEMCO2 =					0xC2,
		OLDLICENSE_SQUARESOFT =				0xC3,
		OLDLICENSE_TOKUMA_SHOTEN_I2 =		0xC4,
		OLDLICENSE_DATA_EAST =				0xC5,
		OLDLICENSE_TONKINHOUSE =			0xC6,
		OLDLICENSE_KOEI =					0xC8,
		OLDLICENSE_UFL =					0xC9,
		OLDLICENSE_ULTRA =					0xCA,
		OLDLICENSE_VAP =					0xCB,
		OLDLICENSE_USE =					0xCC,
		OLDLICENSE_MELDAC =					0xCD,
		OLDLICENSE_PONY_CANYON_OR =			0xCE,
		OLDLICENSE_ANGEL1 =					0xCF,
		OLDLICENSE_TAITO1 =					0xD0,
		OLDLICENSE_SOFEL =					0xD1,
		OLDLICENSE_QUEST =					0xD2,
		OLDLICENSE_SIGMA_ENTERPRISES =		0xD3,
		OLDLICENSE_ASK_KODANSHA =			0xD4,
		OLDLICENSE_NAXAT_SOFT1 =			0xD6,
		OLDLICENSE_COPYA_SYSTEMS =			0xD7,
		OLDLICENSE_BANPRESTO3 =				0xD9,
		OLDLICENSE_TOMY =					0xDA,
		OLDLICENSE_LJN1 =					0xDB,
		OLDLICENSE_NCS =					0xDD,
		OLDLICENSE_HUMAN =					0xDE,
		OLDLICENSE_ALTRON =					0xDF,
		OLDLICENSE_JALECO2 =				0xE0,
		OLDLICENSE_TOWACHIKI =				0xE1,
		OLDLICENSE_UUTAKA =					0xE2,
		OLDLICENSE_VARIE2 =					0xE3,
		OLDLICENSE_EPOCH =					0xE5,
		OLDLICENSE_ATHENA =					0xE7,
		OLDLICENSE_ASMIK =					0xE8,
		OLDLICENSE_NATSUME =				0xE9,
		OLDLICENSE_KING_RECORDS =			0xEA,
		OLDLICENSE_ATLUS2 =					0xEB,
		OLDLICENSE_EPIC_SONYRECORDS =		0xEC,
		OLDLICENSE_IGS =					0xEE,
		OLDLICENSE_AWAVE =					0xF0,
		OLDLICENSE_EXTREME_ENTERTAINMENT =	0xF3,
		OLDLICENSE_LJN2 =					0xFF,
		OLDLICENSE_UNKNOWN =				0x100
	};

	enum ROMSizesEnum
	{
		ROMSIZE_2BANK = 0x0,
		ROMSIZE_4BANK = 0x1,
		ROMSIZE_8BANK = 0x2,
		ROMSIZE_16BANK = 0x3,
		ROMSIZE_32BANK = 0x4,
		ROMSIZE_64BANK = 0x5,
		ROMSIZE_128BANK = 0x6,
		ROMSIZE_72BANK = 0x52,
		ROMSIZE_80BANK = 0x53,
		ROMSIZE_96BANK = 0x54
	};

	enum RAMSizesEnum
	{
		RAMSIZE_NONE = 0x0,
		RAMSIZE_HALFBANK = 0x1,
		RAMSIZE_1BANK = 0x2,
		RAMSIZE_4BANK = 0x3,
		RAMSIZE_16BANK = 0x4
	};

	enum DestinationCodesEnum
	{
		DESTINATIONCODE_JAPANESE = 0x0,
		DESTINATIONCODE_NONJAPANESE = 0x1,
		DESTINATIONCODE_UNKNOWN = 0x2
	};

	char ROMFile[255];
	char gameTitle[17];
	bool colorGB;
	bool superGB;
	CartridgeTypesEnum cartType;
	MMCTypesEnum MMCType;
	int BatterySupport;
	int RumbleSupport;
	int RTCSupport;
	NewLicensesEnum newLicense;
	OldLicensesEnum oldLicense;
	int ROMSize;
	int RAMSize;
	DestinationCodesEnum destinationCode;

	ROMInfo();
	void ReadROMInfo(const BYTE* ROMBytes);

	const char* CartTypeToString(CartridgeTypesEnum type) const { return CartridgeTypes[type]; }
	const char* NewLicenseToString(NewLicensesEnum lic) const { return NewLicenses[lic]; }
	const char* OldLicenseToString(OldLicensesEnum lic) const { return OldLicenses[lic]; }
	const char* DestinationCodeToString(DestinationCodesEnum dest) const { return DestinationCodes[dest]; }

private:
	const char* CartridgeTypes[0x101];	//Cartridge type # -> Cartridge type string table
	const char* NewLicenses[0x9B];		//New license # -> New wicense string table
	const char* OldLicenses[0x101];		//Old license # -> Old license string table
	const char* DestinationCodes[3];	//Destination # -> Destination string table
	int ROMSizes[0xFF];					//ROM size # -> ROM size value table
	int RAMSizes[0xFF];					//RAM size # -> RAM size value table
};

}

#endif