#include "CookieboyROMInfo.h"
#include <memory.h>

Cookieboy::ROMInfo::ROMInfo()
{
	for (int i = 0; i < 0x101; i++)
	{
		CartridgeTypes[i] = "Unknown";
	}

	CartridgeTypes[CART_ROM_ONLY] = "ROM ONLY";
	CartridgeTypes[CART_ROM_MBC1] = "ROM+MBC1";
	CartridgeTypes[CART_ROM_MBC1_RAM] = "ROM+MBC1+RAM";
	CartridgeTypes[CART_ROM_MBC1_RAM_BATT] = "ROM+MBC1+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC2] = "ROM+MBC2";
	CartridgeTypes[CART_ROM_MBC2_BATT] = "ROM+MBC2+BATT";
	CartridgeTypes[CART_ROM_RAM] = "ROM+RAM";
	CartridgeTypes[CART_ROM_RAM_BATT] = "ROM+RAM+BATT";
	CartridgeTypes[CART_ROM_MMM01] = "ROM+MMM01";
	CartridgeTypes[CART_ROM_MMM01_SRAM] = "ROM+MMM01+SRAM";
	CartridgeTypes[CART_ROM_MMM01_SRAM_BATT] = "ROM+MMM01+SRAM+BATT";
	CartridgeTypes[CART_ROM_MBC3_TIMER_BATT] = "ROM+MBC3+TIMER+BATT";
	CartridgeTypes[CART_ROM_MBC3_TIMER_RAM_BATT] = "ROM+MBC3+TIMER+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC3] = "ROM+MBC3";
	CartridgeTypes[CART_ROM_MBC3_RAM] = "ROM+MBC3+RAM";
	CartridgeTypes[CART_ROM_MBC3_RAM_BATT] = "ROM+MBC3+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC5] = "ROM+MBC5";
	CartridgeTypes[CART_ROM_MBC5_RAM] = "ROM+MBC5+RAM";
	CartridgeTypes[CART_ROM_MBC5_RAM_BATT] = "ROM+MBC5+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE] = "ROM+MBC5+RUMBLE";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE_SRAM] = "ROM+MBC5+RUMBLE+SRAM";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE_SRAM_BATT] = "ROM+MBC5+RUMBLE+SRAM+BATT";
	CartridgeTypes[CART_POCKET_CAMERA] = "Pocket Camera";
	CartridgeTypes[CART_BANDAI_TAMA5] = "Bandai TAMA5";
	CartridgeTypes[CART_HUDSON_HUC3] = "Hudson HuC-3";
	CartridgeTypes[CART_HUDSON_HUC1] = "Hudson HuC-1";

	for (int i = 0; i < 0x3; i++)
	{
		DestinationCodes[i] = "Unknown";
	}

	DestinationCodes[DESTINATIONCODE_JAPANESE] = "Japanese";
	DestinationCodes[DESTINATIONCODE_NONJAPANESE] = "Non-Japanese";
	
	for (int i = 0; i < 0x9B; i++)
	{
		NewLicenses[i] = "Unknown";
	}

	NewLicenses[NEWLICENSE_NONE] = "None";
	NewLicenses[NEWLICENSE_NINTENDO1] = "Nintendo";
	NewLicenses[NEWLICENSE_NINTENDO2] = "Nintendo";
	NewLicenses[NEWLICENSE_CAPCOM] = "Capcom";
	NewLicenses[NEWLICENSE_ELECTRONIC_ARTS1] = "Electronic Arts";
	NewLicenses[NEWLICENSE_ELECTRONIC_ARTS2] = "Electronic Arts";
	NewLicenses[NEWLICENSE_HUDSONSOFT] = "HudsonSoft";
	NewLicenses[NEWLICENSE_B_AI] = "B-AI";
	NewLicenses[NEWLICENSE_KSS] = "KSS";
	NewLicenses[NEWLICENSE_POW] = "POW";
	NewLicenses[NEWLICENSE_PCM_COMPLETE] = "PCM Complete";
	NewLicenses[NEWLICENSE_SAN_X] = "San-X";
	NewLicenses[NEWLICENSE_KEMCO_JAPAN] = "Kemco Japan";
	NewLicenses[NEWLICENSE_SETA] = "Seta";
	NewLicenses[NEWLICENSE_VIACOM] = "Viacom";
	NewLicenses[NEWLICENSE_BANDIA] = "Bandia";
	NewLicenses[NEWLICENSE_OCEAN_ACCLAIM1] = "ocean/acclaim";
	NewLicenses[NEWLICENSE_OCEAN_ACCLAIM2] = "ocean/acclaim";
	NewLicenses[NEWLICENSE_KONAMI1] = "Konami";
	NewLicenses[NEWLICENSE_KONAMI2] = "Konami";
	NewLicenses[NEWLICENSE_HECTOR] = "Hector";
	NewLicenses[NEWLICENSE_TAITO] = "Taito";
	NewLicenses[NEWLICENSE_HUDSON] = "Hudson";
	NewLicenses[NEWLICENSE_BANPRESTO] = "Banpresto";
	NewLicenses[NEWLICENSE_UBISOFT] = "Ubisoft";
	NewLicenses[NEWLICENSE_ATLUS] = "Atlus";
	NewLicenses[NEWLICENSE_MALIBU] = "Malibu";
	NewLicenses[NEWLICENSE_ANGEL] = "Angel";
	NewLicenses[NEWLICENSE_PULLETPROOF] = "Pullet-Proof";
	NewLicenses[NEWLICENSE_IREM] = "Irem";
	NewLicenses[NEWLICENSE_ABSOLUTE] = "Absolute";
	NewLicenses[NEWLICENSE_ACCLAIM] = "Acclaim";
	NewLicenses[NEWLICENSE_ACTIVISION] = "Activision";
	NewLicenses[NEWLICENSE_AMERICAN_SUMMY] = "American Summy";
	NewLicenses[NEWLICENSE_HITECH_ENTERTAINMENT] = "HiTech Entertainment";
	NewLicenses[NEWLICENSE_LJN] = "LJN";
	NewLicenses[NEWLICENSE_MATCHBOX] = "Matchbox";
	NewLicenses[NEWLICENSE_MATTEL] = "Mattel";
	NewLicenses[NEWLICENSE_MILTON_BRADLEY] = "Milton Bradley";
	NewLicenses[NEWLICENSE_TITUS] = "Titus";
	NewLicenses[NEWLICENSE_VIRGIN] = "Virgin";
	NewLicenses[NEWLICENSE_LUCASARTS] = "Lucas Arts";
	NewLicenses[NEWLICENSE_OCEAN] = "Ocean";
	NewLicenses[NEWLICENSE_INFOGRAMES] = "Infogrames";
	NewLicenses[NEWLICENSE_INTERPLAY] = "Interplay";
	NewLicenses[NEWLICENSE_BRODERBUND] = "Broderbund";
	NewLicenses[NEWLICENSE_SCULPTURED] = "Sculptured";
	NewLicenses[NEWLICENSE_SCI] = "SCI";
	NewLicenses[NEWLICENSE_T_HQ] = "T*HQ";
	NewLicenses[NEWLICENSE_ACCOLADE] = "Accolade";
	NewLicenses[NEWLICENSE_MISAWA] = "Misawa";
	NewLicenses[NEWLICENSE_LOZC] = "Lozc";
	NewLicenses[NEWLICENSE_TOKUMA_SHOTEN_I] = "Tokuma Shoten I";
	NewLicenses[NEWLICENSE_TSUKUDA_ORI] = "Tsukuda Ori";
	NewLicenses[NEWLICENSE_CHUN_SOFT] = "Chun Soft";
	NewLicenses[NEWLICENSE_VIDEO_SYSTEM] = "Video System";
	NewLicenses[NEWLICENSE_VARIE] = "Varie";
	NewLicenses[NEWLICENSE_YONEZAWAS_PAL] = "Yonezawas PAL";
	NewLicenses[NEWLICENSE_KANEKO] = "Kaneko";
	NewLicenses[NEWLICENSE_PACK_IN_SOFT] = "Pack in Soft";

	for (int i = 0; i < 0x101; i++)
	{
		OldLicenses[i] = "Unknown";
	}

	OldLicenses[OLDLICENSE_NONE] = "None";
	OldLicenses[OLDLICENSE_NINTENDO1] = "Nintendo";
	OldLicenses[OLDLICENSE_NINTENDO2] = "Nintendo";
	OldLicenses[OLDLICENSE_JALECO1] = "Jaleco";
	OldLicenses[OLDLICENSE_JALECO2] = "Jaleco";
	OldLicenses[OLDLICENSE_ELECTRONIC_ARTS1] = "Electronic Arts";
	OldLicenses[OLDLICENSE_ELECTRONIC_ARTS2] = "Electronic Arts";
	OldLicenses[OLDLICENSE_YANOMAN] = "Yanoman";
	OldLicenses[OLDLICENSE_PCM_COMPLETE] = "PCM Complete";
	OldLicenses[OLDLICENSE_SETA] = "Seta";
	OldLicenses[OLDLICENSE_BANDAI1] = "Bandai";
	OldLicenses[OLDLICENSE_BANDAI2] = "Bandai";
	OldLicenses[OLDLICENSE_BANDAI3] = "Bandai";
	OldLicenses[OLDLICENSE_HECTOR] = "Hector";
	OldLicenses[OLDLICENSE_ENTERTAINMENT_I] = "Entertainment I";
	OldLicenses[OLDLICENSE_ATLUS1] = "Atlus";
	OldLicenses[OLDLICENSE_ATLUS2] = "Atlus";
	OldLicenses[OLDLICENSE_SPECTRUM_HOLOBY] = "Spectrum Holoby";
	OldLicenses[OLDLICENSE_MALIBU1] = "Malibu";
	OldLicenses[OLDLICENSE_MALIBU2] = "Malibu";
	OldLicenses[OLDLICENSE_ACCLAIM1] = "Acclaim";
	OldLicenses[OLDLICENSE_ACCLAIM2] = "Accalim";
	OldLicenses[OLDLICENSE_GAMETEK] = "Gametek";
	OldLicenses[OLDLICENSE_MATCHBOX] = "MatchBox";
	OldLicenses[OLDLICENSE_ROMSTAR] = "Romstar";
	OldLicenses[OLDLICENSE_TITUS] = "Titus";
	OldLicenses[OLDLICENSE_INFOGRAMES1] = "Infogrames";
	OldLicenses[OLDLICENSE_INFOGRAMES2] = "Infogrames";
	OldLicenses[OLDLICENSE_SCULPTERED_SOFT] = "Sculptered Soft";
	OldLicenses[OLDLICENSE_ACCOLADE] = "Accolade";
	OldLicenses[OLDLICENSE_KEMCO1] = "Kemco";
	OldLicenses[OLDLICENSE_KEMCO2] = "Kemco";
	OldLicenses[OLDLICENSE_TOKUMA_SHOTEN_I1] = "Tokuma Shoten I";
	OldLicenses[OLDLICENSE_TOKUMA_SHOTEN_I2] = "Tokuma Shoten I";
	OldLicenses[OLDLICENSE_APE] = "APE";
	OldLicenses[OLDLICENSE_VIDEOSYSTEM] = "Video System";
	OldLicenses[OLDLICENSE_YONEZAWAS_PAL] = "Yonezawas PAL";
	OldLicenses[OLDLICENSE_NIHON_BUSSAN] = "Nihon Bussan";
	OldLicenses[OLDLICENSE_BANPRESTO1] = "Banpresto";
	OldLicenses[OLDLICENSE_BANPRESTO2] = "Banpresto";
	OldLicenses[OLDLICENSE_BANPRESTO3] = "Banpresto";
	OldLicenses[OLDLICENSE_TAKARA] = "Takara";
	OldLicenses[OLDLICENSE_TOEI_ANIMATION] = "Toei Animation";
	OldLicenses[OLDLICENSE_ENIX] = "Enix";
	OldLicenses[OLDLICENSE_PONY_CANYON] = "Pony Canyon";
	OldLicenses[OLDLICENSE_SONY_IMAGESOFT] = "Sony/ImageSoft";
	OldLicenses[OLDLICENSE_DATA_EAST] = "Data East";
	OldLicenses[OLDLICENSE_UFL] = "UFL";
	OldLicenses[OLDLICENSE_USE] = "Use";
	OldLicenses[OLDLICENSE_ANGEL1] = "Angel";
	OldLicenses[OLDLICENSE_ANGEL2] = "Angel";
	OldLicenses[OLDLICENSE_QUEST] = "Quest";
	OldLicenses[OLDLICENSE_NAXAT_SOFT1] = "Naxat Soft";
	OldLicenses[OLDLICENSE_NAXAT_SOFT2] = "Naxat Soft";
	OldLicenses[OLDLICENSE_TOMY] = "Tomy";
	OldLicenses[OLDLICENSE_HUMAN] = "Human";
	OldLicenses[OLDLICENSE_TOWACHIKI] = "Towachiki";
	OldLicenses[OLDLICENSE_EPOCH] = "Epoch";
	OldLicenses[OLDLICENSE_NATSUME] = "Natsume";
	OldLicenses[OLDLICENSE_EPIC_SONYRECORDS] = "Epic SonyRecords";
	OldLicenses[OLDLICENSE_EXTREME_ENTERTAINMENT] = "Extereme Entertainment";
	OldLicenses[OLDLICENSE_CAPCOM1] = "Capcom";
	OldLicenses[OLDLICENSE_CAPCOM2] = "Capcom";
	OldLicenses[OLDLICENSE_COCONUTS] = "Coconuts";
	OldLicenses[OLDLICENSE_HUDSONSOFT] = "Hudsonsoft";
	OldLicenses[OLDLICENSE_CLARY] = "Clary";
	OldLicenses[OLDLICENSE_SAN_X] = "San X";
	OldLicenses[OLDLICENSE_SEEABOVE] = "Seeabove";
	OldLicenses[OLDLICENSE_GREMLIN] = "Gremlin";
	OldLicenses[OLDLICENSE_IREM] = "Irem";
	OldLicenses[OLDLICENSE_US_GOLD] = "US Gold";
	OldLicenses[OLDLICENSE_ACTIVISION] = "Activision";
	OldLicenses[OLDLICENSE_PARKPLACE] = "ParkPlace";
	OldLicenses[OLDLICENSE_MILTON_BRADLEY] = "Milton Bradley";
	OldLicenses[OLDLICENSE_VIRGIN1] = "Virgin";
	OldLicenses[OLDLICENSE_VIRGIN2] = "Virgin";
	OldLicenses[OLDLICENSE_VIRGIN3] = "Virgin";
	OldLicenses[OLDLICENSE_ELITE_SYSTEMS1] = "Elite Systems";
	OldLicenses[OLDLICENSE_ELITE_SYSTEMS2] = "Elite Systems";
	OldLicenses[OLDLICENSE_INTERPLAY] = "Interplay";
	OldLicenses[OLDLICENSE_THE_SALES_CURVE] = "The Sales Curve";
	OldLicenses[OLDLICENSE_TRIFFIX_ENTERTAINMENT] = "Triffix Entertainment";
	OldLicenses[OLDLICENSE_MISAWA_ENTERTAINMENT] = "Misawa Entertainment";
	OldLicenses[OLDLICENSE_BULLET_PROOFSOFTWARE] = "Bullet-Proof Software";
	OldLicenses[OLDLICENSE_IMAX] = "IMAX";
	OldLicenses[OLDLICENSE_TSUBURAVA] = "Tsuburava";
	OldLicenses[OLDLICENSE_KANEKO] = "Kaneko";
	OldLicenses[OLDLICENSE_TECMO] = "Tecmo";
	OldLicenses[OLDLICENSE_NOVA] = "Nova";
	OldLicenses[OLDLICENSE_KONAMI1] = "Konami";
	OldLicenses[OLDLICENSE_KONAMI2] = "Konami";
	OldLicenses[OLDLICENSE_TECHNOS_JAPAN] = "Technos JAPAN";
	OldLicenses[OLDLICENSE_TOHO] = "Toho";
	OldLicenses[OLDLICENSE_ASCII_OR_NEXOFT] = "ASCII Or Nexoft";
	OldLicenses[OLDLICENSE_HAL] = "HAL";
	OldLicenses[OLDLICENSE_CULTURE_BRAIN_O] = "Culture Brain O";
	OldLicenses[OLDLICENSE_SAMMY] = "Sammy";
	OldLicenses[OLDLICENSE_SQUARESOFT] = "SquareSoft";
	OldLicenses[OLDLICENSE_TONKINHOUSE] = "TonkinHouse";
	OldLicenses[OLDLICENSE_ULTRA] = "Ultra";
	OldLicenses[OLDLICENSE_MELDAC] = "Meldac";
	OldLicenses[OLDLICENSE_TAITO1] = "Taito";
	OldLicenses[OLDLICENSE_TAITO2] = "Taito";
	OldLicenses[OLDLICENSE_SIGMA_ENTERPRISES] = "Sigma Enterprises";
	OldLicenses[OLDLICENSE_COPYA_SYSTEMS] = "Copya Systems";
	OldLicenses[OLDLICENSE_LJN1] = "LJN";
	OldLicenses[OLDLICENSE_LJN2] = "LJN";
	OldLicenses[OLDLICENSE_LJN3] = "LJN";
	OldLicenses[OLDLICENSE_ALTRON] = "Altron";
	OldLicenses[OLDLICENSE_UUTAKA] = "Uutaka";
	OldLicenses[OLDLICENSE_ATHENA] = "Athena";
	OldLicenses[OLDLICENSE_KING_RECORDS] = "King Records";
	OldLicenses[OLDLICENSE_IGS] = "IGS";
	OldLicenses[OLDLICENSE_HOT_B] = "Hot B";
	OldLicenses[OLDLICENSE_ITC_ENTERTAINMENT] = "ITC Entertainment";
	OldLicenses[OLDLICENSE_KOTOBUKI_SYSTEMS] = "Kotobuki Systems";
	OldLicenses[OLDLICENSE_UBISOFT] = "Ubisoft";
	OldLicenses[OLDLICENSE_ABSOLUTE] = "Absolute";
	OldLicenses[OLDLICENSE_AMERICAN_SAMMY] = "American Sammy";
	OldLicenses[OLDLICENSE_MINDSCAPE] = "Mindscape";
	OldLicenses[OLDLICENSE_TRADEWEST] = "Tradewest";
	OldLicenses[OLDLICENSE_OCEAN] = "Ocean";
	OldLicenses[OLDLICENSE_ELECTRO_BRAIN] = "Electro-Brain";
	OldLicenses[OLDLICENSE_BRODERBUND1] = "Broderbund";
	OldLicenses[OLDLICENSE_BRODERBUND2] = "Broderbund";
	OldLicenses[OLDLICENSE_THQ] = "THQ";
	OldLicenses[OLDLICENSE_MICROPROSE] = "Microprose";
	OldLicenses[OLDLICENSE_LOZC] = "Lozc";
	OldLicenses[OLDLICENSE_VICTOKAI] = "Victokai";
	OldLicenses[OLDLICENSE_CHUNSOFT] = "Chunsoft";
	OldLicenses[OLDLICENSE_VARIE1] = "Varie";
	OldLicenses[OLDLICENSE_VARIE2] = "Varie";
	OldLicenses[OLDLICENSE_ARC] = "Arc";
	OldLicenses[OLDLICENSE_IMAGINEER] = "Imagineer";
	OldLicenses[OLDLICENSE_HORI_ELECTRIC] = "Hori Electric";
	OldLicenses[OLDLICENSE_KAWADA] = "Kawada";
	OldLicenses[OLDLICENSE_NAMCO] = "Namco";
	OldLicenses[OLDLICENSE_SNK] = "SNK";
	OldLicenses[OLDLICENSE_SUNSOFT] = "Sunsoft";
	OldLicenses[OLDLICENSE_KOEI] = "Koei";
	OldLicenses[OLDLICENSE_VAP] = "Vap";
	OldLicenses[OLDLICENSE_PONY_CANYON_OR] = "Pony Canyon OR";
	OldLicenses[OLDLICENSE_SOFEL] = "Sofel";
	OldLicenses[OLDLICENSE_ASK_KODANSHA] = "Ask Kodansha";
	OldLicenses[OLDLICENSE_NCS] = "NCS";
	OldLicenses[OLDLICENSE_ASMIK] = "Asmik";
	OldLicenses[OLDLICENSE_AWAVE] = "Awave";

	memset(ROMSizes, 0, sizeof(int) * 0xFF);
	ROMSizes[ROMSIZE_2BANK] = 2;
	ROMSizes[ROMSIZE_4BANK] = 4;
	ROMSizes[ROMSIZE_8BANK] = 8;
	ROMSizes[ROMSIZE_16BANK] = 16;
	ROMSizes[ROMSIZE_32BANK] = 32;
	ROMSizes[ROMSIZE_64BANK] = 64;
	ROMSizes[ROMSIZE_128BANK] = 128;
	ROMSizes[ROMSIZE_72BANK] = 72;
	ROMSizes[ROMSIZE_80BANK] = 80;
	ROMSizes[ROMSIZE_96BANK] = 96;

	memset(RAMSizes, 0, sizeof(int) * 0xFF);
	RAMSizes[RAMSIZE_NONE] = 0;
	RAMSizes[RAMSIZE_HALFBANK] = 1;
	RAMSizes[RAMSIZE_1BANK] = 1;
	RAMSizes[RAMSIZE_4BANK] = 4;
	RAMSizes[RAMSIZE_16BANK] = 16;
}

void Cookieboy::ROMInfo::ReadROMInfo(const BYTE* ROMBytes)
{
	memcpy(gameTitle, ROMBytes + 0x134, 16);
	
	colorGB = ROMBytes[0x143] == 0x80;
	superGB = ROMBytes[0x146] == 0x03;
	ROMSize = ROMSizes[ROMBytes[0x148]];
	RAMSize = RAMSizes[ROMBytes[0x149]];

	newLicense = NewLicensesEnum((ROMBytes[0x144] & 0xF0) | (ROMBytes[0x145] & 0x0F));
	if (newLicense > NEWLICENSE_UNKNOWN)
	{
		newLicense = NEWLICENSE_UNKNOWN;
	}

	cartType = (CartridgeTypesEnum)ROMBytes[0x147];
	if (cartType > CART_UNKNOWN)
	{
		cartType = CART_UNKNOWN;
	}

	destinationCode = (DestinationCodesEnum)ROMBytes[0x14A];
	if (destinationCode > DESTINATIONCODE_UNKNOWN)
	{
		destinationCode = DESTINATIONCODE_UNKNOWN;
	}

	oldLicense = (OldLicensesEnum)ROMBytes[0x14B];
	if (oldLicense > OLDLICENSE_UNKNOWN)
	{
		oldLicense = OLDLICENSE_UNKNOWN;
	}

	switch(cartType)
	{
		case CART_ROM_ONLY:
			MMCType = MMC_ROMONLY;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;

		case CART_ROM_MBC1:
		case CART_ROM_MBC1_RAM:
			MMCType = MMC_MBC1;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC1_RAM_BATT:
			MMCType = MMC_MBC1;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;

		case CART_ROM_MBC2:
			MMCType = MMC_MBC2;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC2_BATT:
			MMCType = MMC_MBC2;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;

		case CART_ROM_RAM:
			MMCType = MMC_ROMONLY;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_RAM_BATT:
			MMCType = MMC_ROMONLY;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;

		case CART_ROM_MMM01:
		case CART_ROM_MMM01_SRAM:
			MMCType = MMC_MMM01;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MMM01_SRAM_BATT:
			MMCType = MMC_MMM01;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		
		case CART_ROM_MBC3:
		case CART_ROM_MBC3_RAM:
			MMCType = MMC_MBC3;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC3_RAM_BATT:
			MMCType = MMC_MBC3;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC3_TIMER_BATT:
		case CART_ROM_MBC3_TIMER_RAM_BATT:
			MMCType = MMC_MBC3;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 1;
			break;

		case CART_ROM_MBC5:
		case CART_ROM_MBC5_RAM:
			MMCType = MMC_MBC5;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC5_RAM_BATT:
			MMCType = MMC_MBC5;
			BatterySupport = 1;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC5_RUMBLE:
		case CART_ROM_MBC5_RUMBLE_SRAM:
			MMCType = MMC_MBC5;
			BatterySupport = 0;
			RumbleSupport = 1;
			RTCSupport = 0;
			break;
		case CART_ROM_MBC5_RUMBLE_SRAM_BATT:
			MMCType = MMC_MBC5;
			BatterySupport = 1;
			RumbleSupport = 1;
			RTCSupport = 0;
			break;

		case CART_HUDSON_HUC3:
		case CART_HUDSON_HUC1:
			MMCType = MMC_MBC1;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
			break;

		case CART_POCKET_CAMERA:
		case CART_BANDAI_TAMA5:
			break;

		default:
			MMCType = MMC_UNKNOWN;
			BatterySupport = 0;
			RumbleSupport = 0;
			RTCSupport = 0;
	}

	if (MMCType == MMC_MBC2)
	{
		RAMSize = 512;
	}
}