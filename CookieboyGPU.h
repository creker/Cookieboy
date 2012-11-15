#ifndef GAMEBOYGPU_H
#define GAMEBOYGPU_H

#include "CookieboyDefs.h"
#include <vector>

namespace Cookieboy
{

class Memory;
class Interrupts;

const int VRAMBankSize = 0x2000;

class GPU
{
public:
	/*
	LCD controller modes in the STAT register
	*/
	enum GameboyLCDModes
	{
		GBLCDMODE_OAM = 2,		//OAM is being used ($FE00-$FE9F). CPU cannot access the OAM during this period

		GBLCDMODE_OAMRAM = 3,	//Both the OAM and VRAM are being used. The CPU cannot access either during this period
								//This is where all rendering is done. Lasts for (172 + (SCX % 8) + Sprites) cycles
								//Sprites causing this mode to take longer

		GBLCDMODE_HBLANK = 0,	//H-Blank period. CPU can access the VRAM and OAM
								//Lasts for (204 - (SCX % 8) - Sprites) cycles
								//Here LCD controller compencates mode 3 delays so a scanline would take exactly 456 cycles

		GBLCDMODE_VBLANK = 1	//V-Blank period. CPU can access the VRAM and OAM
	};

	enum InternalLCDModes
	{
		LCDMODE_LY00_HBLANK,
		LCDMODE_LYXX_HBLANK,
		LCDMODE_LYXX_HBLANK_INC,
		LCDMODE_LY00_VBLANK,
		LCDMODE_LY9X_VBLANK,
		LCDMODE_LY9X_VBLANK_INC,
		LCDMODE_LYXX_OAM,
		LCDMODE_LYXX_OAMRAM
	};

	enum DMGPalettes
	{
		RGBPALETTE_BLACKWHITE = 0,
		RGBPALETTE_REAL = 1
	};

	GPU(const bool &_CGB, const bool &_CGBDoubleSpeed, Interrupts& INT, DMGPalettes palette = RGBPALETTE_REAL);

	void Step(DWORD clockDelta, Memory& MMU);

	void Reset();
	void EmulateBIOS();

	const DWORD** GetFramebuffer() { return (const DWORD**)Framebuffer; }
	bool IsNewFrameReady() { return NewFrameReady; }
	void WaitForNewFrame() { NewFrameReady = false; }

	void WriteVRAM(WORD addr, BYTE value);
	void WriteOAM(BYTE addr, BYTE value);
	BYTE ReadVRAM(WORD addr);
	BYTE ReadOAM(BYTE addr);

	void DMAStep(DWORD clockDelta, Memory& MMU);
	void DMAChanged(BYTE value, Memory& MMU);
	void LCDCChanged(BYTE value);
	void STATChanged(BYTE value);
	void SCYChanged(BYTE value) { SCY = value; }
	void SCXChanged(BYTE value) { SCX = value; }
	void LYChanged(BYTE value) { LY = 0; }
	void LYCChanged(BYTE value);
	void BGPChanged(BYTE value) { BGP = value; }
	void OBP0Changed(BYTE value) { OBP0 = value; }
	void OBP1Changed(BYTE value) { OBP1 = value; }
	void WYChanged(BYTE value) { WY = value; }
	void WXChanged(BYTE value) { WX = value; }
	void VBKChanged(BYTE value);
	void HDMA1Changed(BYTE value);
	void HDMA2Changed(BYTE value);
	void HDMA3Changed(BYTE value);
	void HDMA4Changed(BYTE value);
	void HDMA5Changed(BYTE value, Memory& MMU);
	void BGPIChanged(BYTE value) { if (CGB) { BGPI = value; } }
	void BGPDChanged(BYTE value);
	void OBPIChanged(BYTE value) { if (CGB) { OBPI = value; } }
	void OBPDChanged(BYTE value);

	BYTE GetLCDC() { return LCDC; }
	BYTE GetSTAT() { return STAT | 0x80; }
	BYTE GetSCY() { return SCY; }
	BYTE GetSCX() { return SCX; }
	BYTE GetLY() { return LY; }
	BYTE GetLYC() { return LYC; }
	BYTE GetBGP() { return BGP; }
	BYTE GetOBP0() { return OBP0; }
	BYTE GetOBP1() { return OBP1; }
	BYTE GetWY() { return WY; }
	BYTE GetWX() { return WX; }
	BYTE GetVBK() { return VBK | 0xFE; }
	BYTE GetHDMA1() { return HDMASource >> 8; }
	BYTE GetHDMA2() { return HDMASource & 0xFF; }
	BYTE GetHDMA3() { return HDMADestination >> 8; }
	BYTE GetHDMA4() { return HDMADestination & 0xFF; }
	BYTE GetHDMA5() { return HDMAControl; }
	BYTE GetBGPI() { return BGPI | 0x40; }
	BYTE GetBGPD() { return BGPD[BGPI & 0x3F]; }
	BYTE GetOBPI() { return OBPI | 0x40; }
	BYTE GetOBPD() { return OBPD[OBPI & 0x3F]; }

	//Debug
	void ToggleBackground() { BackgroundGlobalToggle = !BackgroundGlobalToggle; }
	void ToggleWindow() { WindowsGlobalToggle = !WindowsGlobalToggle; }
	void ToggleSprites() { SpritesGlobalToggle = !SpritesGlobalToggle; }

private:

	//GPU Microcode
	void RenderScanline();
	void CheckLYC(Interrupts &INT);
	void PrepareSpriteQueue();
	bool HDMACopyBlock(WORD source, WORD dest, Memory &MMU);
	DWORD GBCColorToARGB(WORD color);

	const bool &CGB;
	const bool &CGBDoubleSpeed;

	Interrupts& INT;

	//GPU memory
	BYTE VRAM[VRAMBankSize * 2];
	BYTE OAM[0xA0 + 0x5F];
	WORD VRAMBankOffset;

	//GPU I/O ports
	BYTE LCDC;	//LCD Control (R/W)
				//Bit 7 - LCD Control Operation
				//	0: Stop completely (no picture on screen)
				//	1: operation
				//Bit 6 - Window Tile Map Display Select
				//	0: $9800-$9BFF
				//	1: $9C00-$9FFF
				//Bit 5 - Window Display
				//	0: off
				//	1: on
				//Bit 4 - BG & Window Tile Data Select
				//	0: $8800-$97FF
				//	1: $8000-$8FFF <- Same area as OBJ
				//Bit 3 - BG Tile Map Display Select
				//	0: $9800-$9BFF
				//	1: $9C00-$9FFF
				//Bit 2 - OBJ (Sprite) Size
				//	0: 8*8
				//	1: 8*16 (width*height)
				//Bit 1 - OBJ (Sprite) Display
				//	0: off
				//	1: on
				//Bit 0 - BG & Window Display
				//	0: off
				//	1: on

	BYTE STAT;	//LCDC Status (R/W)
				//Bits 6-3 - Interrupt Selection By LCDC Status
				//Bit 6 - LYC=LY Coincidence (Selectable)
				//Bit 5 - Mode 10
				//Bit 4 - Mode 01
				//Bit 3 - Mode 00
				//	0: Non Selection
				//	1: Selection
				//Bit 2 - Coincidence Flag
				//	0: LYC not equal to LCDC LY
				//	1: LYC = LCDC LY
				//Bit 1-0 - Mode Flag
				//	00: During H-Blank
				//	01: During V-Blank
				//	10: During Searching OAM-RAM
				//	11: During Transfering Data to LCD Driver

	BYTE SCY;	//Scroll Y (R/W)
				//8 Bit value $00-$FF to scroll BG Y screen position

	BYTE SCX;	//Scroll X (R/W)
				//8 Bit value $00-$FF to scroll BG X screen position

	BYTE LY;	//LCDC Y-Coordinate (R)
				//The LY indicates the vertical line to which the present data is transferred to the LCD
				//Driver. The LY can take on any value between 0 through 153. The values between
				//144 and 153 indicate the V-Blank period. Writing will reset the counter

	BYTE LYC;	//LY Compare (R/W)
				//The LYC compares itself with the LY. If the values are the same it causes the STAT to set the coincident flag

	BYTE BGP;	//BG & Window Palette Data (R/W)
				//Bit 7-6 - Data for Dot Data 11 (Normally darkest color)
				//Bit 5-4 - Data for Dot Data 10
				//Bit 3-2 - Data for Dot Data 01
				//Bit 1-0 - Data for Dot Data 00 (Normally lightest color)

	BYTE OBP0;	//Object Palette 0 Data (R/W)
				//This selects the colors for sprite palette 0. It works exactly as BGP except each value of 0 is transparent

	BYTE OBP1;	//Object Palette 1 Data (R/W)
				//This selects the colors for sprite palette 1. It works exactly as OBP0

	BYTE WY;	//Window Y Position (R/W)
				//0 <= WY <= 143
				//WY must be greater than or equal to 0 and must be less than or equal to 143 for window to be visible

	BYTE WX;	//Window X Position (R/W)
				//0 <= WX <= 166
				//WX must be greater than or equal to 0 and must be less than or equal to 166 for window to be visible

	BYTE VBK;	//Current Video Memory (VRAM) Bank
				//Bit 0 - VRAM Bank (0-1)

	BYTE BGPI;	//Background Palette Index
				//Bit 0-5   Index (00-3F)
				//Bit 7     Auto Increment  (0=Disabled, 1=Increment after Writing)

	BYTE BGPD[8 * 8];	//Background Palette Data
						//Bit 0-4   Red Intensity   (00-1F)
						//Bit 5-9   Green Intensity (00-1F)
						//Bit 10-14 Blue Intensity  (00-1F)


	BYTE OBPI;	//Sprite Palette Index
				//Bit 0-5   Index (00-3F)
				//Bit 7     Auto Increment  (0=Disabled, 1=Increment after Writing)

	BYTE OBPD[8 * 8];	//Sprite Palette Data
						//Bit 0-4   Red Intensity   (00-1F)
						//Bit 5-9   Green Intensity (00-1F)
						//Bit 10-14 Blue Intensity  (00-1F)

	DWORD GB2RGBPalette[4];			//Gameboy palette -> ARGB color
	DWORD GBC2RGBPalette[32768];	//GBG color -> ARGB color
	DWORD SpriteClocks[11];			//Sprites rendering affects LCD timings
	bool NewFrameReady;				//Indicates that new frame rendered
	DWORD Framebuffer[144][160];	//Holds actual pixel colors of the last frame
	BYTE Backbuffer[144][160];		//Holds pixel colors in palette values
	std::vector<DWORD> SpriteQueue;	//Contains sprites to be rendered in the right order

	//LCD modes loop
	DWORD ClockCounter;
	DWORD ClocksToNextState;		//Delay before switching LCD mode
	DWORD ScrollXClocks;			//Delay for the SCX Gameboy bug
	bool LCDCInterrupted;			//Makes sure that only one LCDC INT gets requested per scanline
	InternalLCDModes LCDMode;

	BYTE WindowLine;
	int DelayedWY;

	//OAM DMA
	bool OAMDMAStarted;
	WORD OAMDMAProgress;
	WORD OAMDMASource;
	DWORD OAMDMAClockCounter;

	//VRAM DMA
	bool HDMAActive;
	WORD HDMASource;
	WORD HDMADestination;
	BYTE HDMAControl;//New DMA Length/Mode/Start

	//Debug
	bool BackgroundGlobalToggle;
	bool WindowsGlobalToggle;
	bool SpritesGlobalToggle;
};

}

#endif