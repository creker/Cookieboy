#include "CookieboyGPU.h"
#include <algorithm>

#include "CookieboyInterrupts.h"
#include "CookieboyMemory.h"

#define LCD_ON() ((LCDC & 0x80) != 0)
#define GET_LCD_MODE() ((GameboyLCDModes)(STAT & 0x3))
#define SET_LCD_MODE(value) {STAT &= 0xFC; STAT |= (value);}
#define GET_TILE_PIXEL(VRAMAddr, x, y) ((((VRAM[(VRAMAddr) + (y) * 2 + 1] >> (7 - (x))) & 0x1) << 1) | ((VRAM[(VRAMAddr) + (y) * 2] >> (7 - (x))) & 0x1))

Cookieboy::GPU::GPU(const bool &_CGB, Interrupts& INT, DMGPalettes palette) :
INT(INT),
CGB(_CGB)
{
	Reset();

	if (palette == RGBPALETTE_REAL)
	{
		//Green palette
		GB2RGBPalette[0] = 0xFFE1F7D1;
		GB2RGBPalette[1] = 0xFF87C372;
		GB2RGBPalette[2] = 0xFF337053;
		GB2RGBPalette[3] = 0xFF092021;
	}
	else
	{
		GB2RGBPalette[0] = 0xFFFFFFFF;
		GB2RGBPalette[1] = 0xFFAAAAAA;
		GB2RGBPalette[2] = 0xFF555555;
		GB2RGBPalette[3] = 0xFF000000;
	}

	SpriteClocks[0] = 0;
	SpriteClocks[1] = 8;
	SpriteClocks[2] = 20;
	SpriteClocks[3] = 32;
	SpriteClocks[4] = 44;
	SpriteClocks[5] = 52;
	SpriteClocks[6] = 64;
	SpriteClocks[7] = 76;
	SpriteClocks[8] = 88;
	SpriteClocks[9] = 96;
	SpriteClocks[10] = 108;

	SpriteQueue = std::vector<DWORD>();
	SpriteQueue.reserve(40);

	for (int i = 0; i < 32768; i++)
	{
		BYTE r = (i & 0x1F) << 3;
		BYTE g = ((i >> 5) & 0x1F) << 3;
		BYTE b = ((i >> 10) & 0x1F) << 3;
		GBC2RGBPalette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
	}
}

void Cookieboy::GPU::Step(DWORD clockDelta, Memory& MMU)
{
	ClockCounter += clockDelta;
	
	while (ClockCounter >= ClocksToNextState)
	{
		ClockCounter -= ClocksToNextState;

		if (!LCD_ON())
		{
			LY = 0;
			ClocksToNextState = 70224;
			for (int i = 0; i < 144; i++)
			{
				memset(Framebuffer[i], 0xFF, sizeof(Framebuffer[0][0]) * 160);
				memset(Backbuffer[i], 0x0, sizeof(Backbuffer[0][0]) * 160);
			}
			NewFrameReady = true;
			continue;
		}
		
		switch (LCDMode)
		{
		case LCDMODE_LYXX_OAM:
			SET_LCD_MODE(GBLCDMODE_OAM);
			if ((STAT & 0x20) && !LCDCInterrupted)//OAM interrupt selected
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);
				LCDCInterrupted = true;
			}

			//LYC=LY checked in the beginning of scanline
			CheckLYC(INT);
			
			ScrollXClocks = (SCX & 0x4) ? 4 : 0;
			LCDMode = LCDMODE_LYXX_OAMRAM;
			ClocksToNextState = 80;
			break;

		case LCDMODE_LYXX_OAMRAM:
			PrepareSpriteQueue();
			SET_LCD_MODE(GBLCDMODE_OAMRAM);

			LCDMode = LCDMODE_LYXX_HBLANK;
			ClocksToNextState = 172 + ScrollXClocks + SpriteClocks[SpriteQueue.size()];
			break;

		case LCDMODE_LYXX_HBLANK:
			RenderScanline();

			SET_LCD_MODE(GBLCDMODE_HBLANK);
			if ((STAT & 0x08) && !LCDCInterrupted)//H-blank interrupt selected
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);
				LCDCInterrupted = true;
			}

			//HDMA block copy
			if (CGB && hdmaActive)
			{
				HDMACopyBlock(HDMASource, VRAMBankOffset + HDMADestination, MMU);
				HDMAControl--;
				HDMADestination += 0x10;
				HDMASource += 0x10;

				if ((HDMAControl & 0x7F) == 0x7F)
				{
					HDMAControl = 0xFF;
					hdmaActive = false;
				}
			}

			LCDMode = LCDMODE_LYXX_HBLANK_INC;
			ClocksToNextState = 200 - ScrollXClocks - SpriteClocks[SpriteQueue.size()];
			break;

		case LCDMODE_LYXX_HBLANK_INC:
			LY++;
			
			//Reset LYC bit
			STAT &= 0xFB;
			
			LCDCInterrupted = false;

			if (LY == 144)
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}
			else
			{
				LCDMode = LCDMODE_LYXX_OAM;
			}
			ClocksToNextState = 4;
			break;

		/* Offscreen LCD modes */
		case LCDMODE_LY9X_VBLANK:
			//V-blank interrupt
			if (LY == 144)
			{
				SET_LCD_MODE(GBLCDMODE_VBLANK);
				INT.Request(Interrupts::INTERRUPT_VBLANK);

				if (STAT & 0x10)
				{
					INT.Request(Interrupts::INTERRUPT_LCDC);
				}
			}

			//Checking LYC=LY in the begginng of scanline
			CheckLYC(INT);

			LCDMode = LCDMODE_LY9X_VBLANK_INC;
			ClocksToNextState = 452;
			break;

		case LCDMODE_LY9X_VBLANK_INC:
			LY++;

			//Reset LYC bit
			STAT &= 0xFB;

			if (LY == 153)
			{
				LCDMode = LCDMODE_LY00_VBLANK;
			}
			else
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}

			LCDCInterrupted = false;
			
			ClocksToNextState = 4;
			break;

		case LCDMODE_LY00_VBLANK:
			//Checking LYC=LY in the begginng of scanline
			//Here LY = 153
			CheckLYC(INT);
			
			LY = 0;

			LCDMode = LCDMODE_LY00_HBLANK;
			ClocksToNextState = 452;
			break;

		case LCDMODE_LY00_HBLANK:
			SET_LCD_MODE(GBLCDMODE_HBLANK);

			LCDCInterrupted = false;

			if (DelayedWY > -1)
			{
				WY = DelayedWY;
				DelayedWY = -1;
			}

			NewFrameReady = true;
			WindowLine = 0;

			LCDMode = LCDMODE_LYXX_OAM;
			ClocksToNextState = 4;
			break;
		}
	}
}

void Cookieboy::GPU::Reset()
{
	memset(VRAM, 0x0, VRAMBankSize * 2);
	memset(OAM, 0x0, 0xFF);
	for (int i = 0; i < 144; i++)
	{
		memset(Framebuffer[i], 0xFF, sizeof(Framebuffer[0][0]) * 160);
		memset(Backbuffer[i], 0, sizeof(Backbuffer[0][0]) * 160);
	}

	// Initially all background colors are initialized as white
	memset(BGPD, 0xFF, 64);

	LCDC = 0x0;
	STAT = 0x0;
	SCY = 0x0;
	SCX = 0x0;
	BGP = 0x0;
	OBP0 = 0x0;
	OBP1 = 0x0;
	WY = 0x0;
	WX = 0x0;
	SpriteQueue.clear();
	NewFrameReady = false;
	ClockCounter = 0;
	ClocksToNextState = 0x0;
	ScrollXClocks = 0x0;
	LCDCInterrupted = false;
	LY = 0x0;
	LYC = 0x0;
	LCDMode = LCDMODE_LYXX_OAM;
	DelayedWY = -1;
	WindowLine = 0;
	VRAMBankOffset = 0;
	hdmaActive = false;
	HDMASource = 0;
	HDMADestination = 0;
	HDMAControl = 0;
	VBK = 0;
	BGPI = 0;
	OBPI = 0;
}

void Cookieboy::GPU::EmulateBIOS()
{
	LCDC = 0x91;
	SCY = 0x0;
	SCX = 0x0;
	LYC = 0x0;
	BGP = 0xFC;
	OBP0 = 0xFF;
	OBP1 = 0xFF;
	WY = 0x0;
	WX = 0x0;
}

void Cookieboy::GPU::WriteVRAM(WORD addr, BYTE value)
{
	if (GET_LCD_MODE() != GBLCDMODE_OAMRAM || !LCD_ON())
	{
		VRAM[VRAMBankOffset + addr] = value;
	}
}

void Cookieboy::GPU::WriteOAM(BYTE addr, BYTE value)
{
	if (GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK || !LCD_ON())
	{
		OAM[addr] = value;
	}
}

BYTE Cookieboy::GPU::ReadVRAM(WORD addr)
{
	if (GET_LCD_MODE() != GBLCDMODE_OAMRAM || !LCD_ON())
	{
		return VRAM[VRAMBankOffset + addr];
	}
	else
	{
		return 0xFF;
	}
}

BYTE Cookieboy::GPU::ReadOAM(BYTE addr) 
{
	if (GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK || !LCD_ON())
	{
		return OAM[addr];
	}
	else
	{
		return 0xFF;
	}
}

void Cookieboy::GPU::DMAChanged(BYTE value, Memory& MMU)
{
	//Transferring data to OAM
	for (int i = 0; i < 0xA0; i++)
	{
		OAM[i] = MMU.Read((value << 8) | i);
	}
}

void Cookieboy::GPU::LCDCChanged(BYTE value)
{
	//If LCD is being turned off
	if (!(value & 0x80))
	{
		STAT &= ~0x3;
		LY = 0;
	}
	//If LCD is being turned on
	else if ((value & 0x80) && !LCD_ON())
	{
		SpriteQueue.clear();
		NewFrameReady = false;
		ClockCounter = 0;
		ScrollXClocks = 0;
		LCDCInterrupted = false;

		LY = 0;
		CheckLYC(INT);

		//When LCD turned on H-blank is active instead of OAM for 80 cycles
		SET_LCD_MODE(GBLCDMODE_HBLANK);
		LCDMode = LCDMODE_LYXX_OAMRAM;
		ClocksToNextState = 80;
	}

	if (!(LCDC & 0x20) && (value & 0x20))
	{
		WindowLine = 144;
	}

	LCDC = value;
}

void Cookieboy::GPU::STATChanged(BYTE value)
{
	//Coincidence flag and mode flag are read-only
	STAT = (STAT & 0x7) | (value & 0x78); 

	//STAT bug
	if (LCD_ON() && 
		(GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK))
	{
		INT.Request(Interrupts::INTERRUPT_LCDC); 
	}
}

void Cookieboy::GPU::LYCChanged(BYTE value) 
{ 
	if (LYC != value)
	{
		LYC = value;
		if (LCDMode != LCDMODE_LYXX_HBLANK_INC && LCDMode != LCDMODE_LY9X_VBLANK_INC)
		{
			CheckLYC(INT);
		}
	}
	else
	{
		LYC = value;
	}
}

void Cookieboy::GPU::VBKChanged(BYTE value)
{ 
	if (CGB)
	{
		VBK = value; 
		VRAMBankOffset = VRAMBankSize * (VBK & 0x1);
	}
	else
	{
		VRAMBankOffset = 0;
	}
}

void Cookieboy::GPU::HDMA1Changed(BYTE value) 
{ 
	if (!CGB) 
	{ 
		return;
	}

	HDMASource = (value << 8) | (HDMASource & 0xFF);
}

void Cookieboy::GPU::HDMA2Changed(BYTE value) 
{ 
	if (!CGB)
	{
		return;
	}
	
	HDMASource = (HDMASource & 0xFF00) | (value & 0xF0);
}

void Cookieboy::GPU::HDMA3Changed(BYTE value)
{
	if (!CGB)
	{
		return;
	}

	HDMADestination = ((value & 0x1F) << 8) | (HDMADestination & 0xF0);
}

void Cookieboy::GPU::HDMA4Changed(BYTE value)
{
	if (!CGB)
	{
		return;
	}

	HDMADestination = (HDMADestination & 0xFF00) | (value & 0xF0);
}

void Cookieboy::GPU::HDMA5Changed(BYTE value, Memory& MMU)
{
	if (!CGB)
	{
		return;
	}

	HDMAControl = value & 0x7F;

	if (hdmaActive)
	{
		if (!(value & 0x80))
		{
			hdmaActive = false;
			HDMAControl |= 0x80;
		}
	}
	else
	{
		if (value & 0x80)//H-Blank DMA
		{
			hdmaActive = true;
		}
		else//General purpose DMA
		{
			hdmaActive = false;

			WORD sourceAddr = HDMASource;
			WORD destAddr = VRAMBankOffset + HDMADestination;
			for (; (HDMAControl & 0x7F) != 0x7F; HDMAControl--, destAddr += 0x10, sourceAddr += 0x10)
			{
				if (HDMACopyBlock(sourceAddr, destAddr, MMU))
				{
					break;
				}
			}

			HDMASource = sourceAddr;
			HDMADestination = destAddr - VRAMBankOffset;
			HDMAControl = 0xFF;
		}
	}
}

void Cookieboy::GPU::BGPDChanged(BYTE value) 
{
	if (!CGB)
	{
		return;
	}

	BYTE index = BGPI & 0x3F;

	BGPD[index] = value;

	if (BGPI & 0x80)
	{
		index++;
		BGPI = (BGPI & 0xC0) | (index & 0x3F);
	}
}

void Cookieboy::GPU::OBPDChanged(BYTE value) 
{
	if (!CGB)
	{
		return;
	}

	BYTE index = OBPI & 0x3F;

	OBPD[index] = value;

	if (OBPI & 0x80)
	{
		index++;
		OBPI = (OBPI & 0xC0) | (index & 0x3F);
	}
}

void Cookieboy::GPU::RenderScanline()
{
	#pragma region background
	if ((LCDC & 0x01) || CGB)//Background display
	{
		WORD tileMapAddr = (LCDC & 0x8) ? 0x1C00 : 0x1800;
		WORD tileDataAddr = (LCDC & 0x10) ? 0x0 : 0x800;

		WORD tileMapX = SCX >> 3;					//converting absolute coorditates to tile coordinates i.e. integer division by 8
		WORD tileMapY = ((SCY + LY) >> 3) & 0x1F;	//then clamping to 0-31 range i.e. wrapping background
		BYTE tileMapTileX = SCX & 0x7;				//
		BYTE tileMapTileY = (SCY + LY) & 0x7;		//

		int tileIdx;
		for (WORD x = 0; x < 160; x++)
		{
			WORD tileAddr = tileMapAddr + tileMapX + tileMapY * 32;
			if (LCDC & 0x10)
			{
				tileIdx = VRAM[tileAddr];
			}
			else
			{
				tileIdx = (signed char)VRAM[tileAddr] + 128;
			}

			if (CGB)
			{
				BYTE tileAttributes = VRAM[VRAMBankSize + tileAddr];

				BYTE flippedTileX = tileMapTileX;
				BYTE flippedTileY = tileMapTileY;
				if (tileAttributes & 0x20)//Horizontal flip
				{
					flippedTileX = 7 - flippedTileX;
				}
				if (tileAttributes & 0x40)//Vertical flip
				{
					flippedTileY = 7 - flippedTileY;
				}

				BYTE colorIdx = GET_TILE_PIXEL(VRAMBankSize * ((tileAttributes >> 3) & 0x1) + tileDataAddr + tileIdx * 16, flippedTileX, flippedTileY);
				WORD color;
				memcpy(&color, BGPD + (tileAttributes & 0x7) * 8 + colorIdx * 2, 2);

				Backbuffer[LY][x] = (tileAttributes & 0x80) | colorIdx;//Storing pallet index with BG-to-OAM priority - sprites will need both
				Framebuffer[LY][x] = GBC2RGBPalette[color & 0x7FFF];
			}
			else
			{
				BYTE palIdx = GET_TILE_PIXEL(tileDataAddr + tileIdx * 16, tileMapTileX, tileMapTileY);

				Backbuffer[LY][x] = (BGP >> (palIdx * 2)) & 0x3;
				Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];
			}
			 
			tileMapX = (tileMapX + ((tileMapTileX + 1) >> 3)) & 0x1F;
			tileMapTileX = (tileMapTileX + 1) & 0x7;
		}
	}
	else
	{
		//Clearing current scanline as background covers whole screen
		memset(Framebuffer[LY], 0xFF, sizeof(Framebuffer[0][0]) * 160);
		memset(Backbuffer[LY], 0, sizeof(Backbuffer[0][0]) * 160);
	}
	#pragma endregion
	#pragma region window
	if (LCDC & 0x20)//Window display
	{
		if (WX <= 166 && WY <= 143 && WindowLine <= 143 && LY >= WY)//Checking window visibility on the whole screen and in the scanline
		{
			WORD tileMapAddr = (LCDC & 0x40) ? 0x1C00 : 0x1800;
			WORD tileDataAddr = (LCDC & 0x10) ? 0x0 : 0x800;

			int windowX = (signed)WX - 7;
			WORD tileMapX = 0;
			WORD tileMapY = (WindowLine >> 3) & 0x1F;
			BYTE tileMapTileX = 0;
			BYTE tileMapTileY = WindowLine & 0x7;

			//Skipping if window lies outside the screen
			if (windowX < 0)
			{
				int offset = -windowX;
				tileMapX = (tileMapX + ((tileMapTileX + offset) >> 3)) & 0x1F;
				tileMapTileX = (tileMapTileX + offset) & 0x7;
				windowX = 0;
			}

			int tileIdx;
			for (int x = windowX; x < 160; x++)
			{
				WORD tileIdxAddr = tileMapAddr + tileMapX + tileMapY * 32;
				if (LCDC & 0x10)
				{
					tileIdx = VRAM[tileIdxAddr];
				}
				else
				{
					tileIdx = (signed char)VRAM[tileIdxAddr] + 128;
				}
				
				if (CGB)
				{
					BYTE tileAttributes = VRAM[VRAMBankSize + tileIdxAddr];

					BYTE flippedTileX = tileMapTileX;
					BYTE flippedTileY = tileMapTileY;
					if (tileAttributes & 0x20)//Horizontal flip
					{
						flippedTileX = 7 - flippedTileX;
					}
					if (tileAttributes & 0x40)//Vertical flip
					{
						flippedTileY = 7 - flippedTileY;
					}

					BYTE colorIdx = GET_TILE_PIXEL(VRAMBankSize * ((tileAttributes >> 3) & 0x1) + tileDataAddr + tileIdx * 16, flippedTileX, flippedTileY);
					WORD color;
					memcpy(&color, BGPD + (tileAttributes & 0x7) * 8 + colorIdx * 2, 2);
				
					Backbuffer[LY][x] = (tileAttributes & 0x80) | colorIdx;//Storing pallet index with BG-to-OAM priority - sprites will need both
					Framebuffer[LY][x] = GBC2RGBPalette[color & 0x7FFF];
				}
				else
				{
					BYTE palIdx = GET_TILE_PIXEL(tileDataAddr + tileIdx * 16, tileMapTileX, tileMapTileY);

					Backbuffer[LY][x] = (BGP >> (palIdx * 2)) & 0x3;
					Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];
				}
				
				tileMapX = (tileMapX + ((tileMapTileX + 1) >> 3)) & 0x1F;
				tileMapTileX = (tileMapTileX + 1) & 0x7;
			}

			WindowLine++;
		}
	}
	#pragma endregion
	#pragma region sprites
	if (LCDC & 0x02)//Object (sprite) display
	{
		BYTE spriteHeight;
		BYTE tileIdxMask;
		if (LCDC & 0x04)
		{
			spriteHeight = 16;
			tileIdxMask = 0xFE;
		}
		else
		{
			spriteHeight = 8;
			tileIdxMask = 0xFF;
		}

		BYTE dmgPalettes[2] = {OBP0, OBP1};

		for (int i = SpriteQueue.size() - 1; i >= 0; i--)
		{
			BYTE spriteAddr = SpriteQueue[i] & 0xFF;

			//Sprites that are hidden by X coordinate still affect sprite queue
			if (OAM[spriteAddr + 1] == 0 || OAM[spriteAddr + 1] >= 168)
			{
				continue;
			}

			BYTE dmgPalette = dmgPalettes[(OAM[spriteAddr + 3] >> 4) & 0x1];
			BYTE *cgbPalette = &OBPD[(OAM[spriteAddr + 3] & 0x7) * 8];
			WORD cgbTileMapOffset = VRAMBankOffset * ((OAM[spriteAddr + 3] >> 3) & 0x1);

			int spriteX = OAM[spriteAddr + 1] - 8;
			int spritePixelX = 0;
			int spritePixelY = LY - (OAM[spriteAddr] - 16);
			int dx = 1;

			if (spriteX < 0)
			{
				spritePixelX = (BYTE)(spriteX * -1);
				spriteX = 0;
			}

			if (OAM[spriteAddr + 3] & 0x20)//X flip
			{
				spritePixelX = 7 - spritePixelX;
				dx = -1;
			}
			if (OAM[spriteAddr + 3] & 0x40)//Y flip
			{
				spritePixelY = spriteHeight - 1 - spritePixelY;
			}

			//If sprite priority is
			if ((OAM[spriteAddr + 3] & 0x80) && (!CGB || (LCDC & 0x1)))
			{
				//sprites are hidden only behind non-zero colors of the background and window
				BYTE colorIdx;
				for (int x = spriteX; x < spriteX + 8 && x < 160; x++, spritePixelX += dx)
				{
					if (CGB)
					{
						colorIdx = GET_TILE_PIXEL(cgbTileMapOffset + (OAM[spriteAddr + 2] & tileIdxMask) * 16, spritePixelX, spritePixelY);
					}
					else
					{
						colorIdx = GET_TILE_PIXEL((OAM[spriteAddr + 2] & tileIdxMask) * 16, spritePixelX, spritePixelY);
					}

					if (colorIdx == 0 ||				//sprite color 0 is transparent
						(Backbuffer[LY][x] & 0x7) > 0)	//Sprite hidden behind colors 1-3
					{
						continue;
					}
					//If CGB game and priority flag of current background tile is set - background and window on top of sprites
					//Master priority on LCDC can override this but here it's set and 
					if (CGB && (Backbuffer[LY][x] & 0x80))
					{
						continue;
					}

					if (CGB)
					{
						WORD color;
						memcpy(&color, cgbPalette + colorIdx * 2, 2);

						Backbuffer[LY][x] = colorIdx;
						Framebuffer[LY][x] = GBC2RGBPalette[color & 0x7FFF];
					}
					else
					{
						Backbuffer[LY][x] = (dmgPalette >> (colorIdx * 2)) & 0x3;
						Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];
					}
				}
			}
			else
			{
				//sprites are on top of the background and window
				BYTE colorIdx;
				for (int x = spriteX; x < spriteX + 8 && x < 160; x++, spritePixelX += dx)
				{
					if (CGB)
					{
						colorIdx = GET_TILE_PIXEL(cgbTileMapOffset + (OAM[spriteAddr + 2] & tileIdxMask) * 16, spritePixelX, spritePixelY);
					}
					else
					{
						colorIdx = GET_TILE_PIXEL((OAM[spriteAddr + 2] & tileIdxMask) * 16, spritePixelX, spritePixelY);
					}
					
					if (colorIdx == 0)//sprite color 0 is transparent
					{
						continue;
					}

					if (CGB)
					{
						WORD color;
						memcpy(&color, cgbPalette + colorIdx * 2, 2);

						Backbuffer[LY][x] = colorIdx;
						Framebuffer[LY][x] = GBC2RGBPalette[color & 0x7FFF];
					}
					else
					{
						Backbuffer[LY][x] = (dmgPalette >> (colorIdx * 2)) & 0x3;
						Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];
					}
				}
			}
		}
	}
	#pragma endregion
}

void Cookieboy::GPU::CheckLYC(Interrupts &INT)
{
	if (LYC == LY)
	{
		STAT |= 0x4;
		if ((STAT & 0x40) && !LCDCInterrupted)//LYC=LY coincidence interrupt selected
		{
			INT.Request(Interrupts::INTERRUPT_LCDC);
			LCDCInterrupted = true;
		}
	}
	else
	{
		STAT &= 0xFB;
	}
}

void Cookieboy::GPU::PrepareSpriteQueue()
{
	SpriteQueue.clear();

	if (LCD_ON() && (LCDC & 0x2))
	{
		int spriteHeight = (LCDC & 0x4) ? 16 : 8;

		//Building sprite queue
		for (BYTE i = 0; i < 160; i += 4)
		{
			if ((OAM[i] - 16) <= LY && LY < (OAM[i] - 16 + spriteHeight))//Sprite on a current scanline
			{
				SpriteQueue.push_back((OAM[i + 1] << 8) | i);//Building ID that corresponds to the sprite priority

				//Max 10 sprites per scanline
				if (SpriteQueue.size() == 10)
				{
					break;
				}
			}
		}

		//In CGB mode sprites always ordered according to OAM ordering
		//In DMG mode sprites ordered accroging to X coorinate and OAM ordering (for sprites with equal X coordinate)
		if (!CGB && SpriteQueue.size())
		{
			std::sort(SpriteQueue.begin(), SpriteQueue.end(), std::less<DWORD>());
		}
	}
}

bool Cookieboy::GPU::HDMACopyBlock(WORD source, WORD dest, Memory &MMU)
{
	if (dest >= VRAMBankOffset + VRAMBankSize || source == 0xFFFF)
	{
		return true;
	}

	VRAM[dest + 0x0] = MMU.Read(source + 0x0);
	VRAM[dest + 0x1] = MMU.Read(source + 0x1);
	VRAM[dest + 0x2] = MMU.Read(source + 0x2);
	VRAM[dest + 0x3] = MMU.Read(source + 0x3);
	VRAM[dest + 0x4] = MMU.Read(source + 0x4);
	VRAM[dest + 0x5] = MMU.Read(source + 0x5);
	VRAM[dest + 0x6] = MMU.Read(source + 0x6);
	VRAM[dest + 0x7] = MMU.Read(source + 0x7);
	VRAM[dest + 0x8] = MMU.Read(source + 0x8);
	VRAM[dest + 0x9] = MMU.Read(source + 0x9);
	VRAM[dest + 0xA] = MMU.Read(source + 0xA);
	VRAM[dest + 0xB] = MMU.Read(source + 0xB);
	VRAM[dest + 0xC] = MMU.Read(source + 0xC);
	VRAM[dest + 0xD] = MMU.Read(source + 0xD);
	VRAM[dest + 0xE] = MMU.Read(source + 0xE);
	VRAM[dest + 0xF] = MMU.Read(source + 0xF);

	return false;
}

DWORD Cookieboy::GPU::GBCColorToARGB(WORD color)
{
	return 0xFF000000 | color;
}