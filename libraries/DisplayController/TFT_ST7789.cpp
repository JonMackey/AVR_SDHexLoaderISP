/*
*	TFT_ST7789.cpp, Copyright Jonathan Mackey 2019
*	Class to control an SPI TFT ST7789 display controller.
*
*	GNU license:
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*	Please maintain this license information along with authorship and copyright
*	notices in any redistribution of this code.
*
*/
#include <SPI.h>
#include "TFT_ST7789.h"
#include <DataStream.h>
#include "Arduino.h"


/******************************** TFT_ST7789 ********************************/
TFT_ST7789::TFT_ST7789(
	uint8_t		inDCPin,
	int8_t		inResetPin,	
	int8_t		inCSPin,
	int8_t		inBacklightPin,
	uint16_t	inHeight,
	uint16_t	inWidth,
	bool		inCentered,
	bool		inIsBGR)
	: TFT_ST77XX(inDCPin, inResetPin, inCSPin, inBacklightPin, inHeight, inWidth, inCentered, inIsBGR)
{
}

/************************************ Init ************************************/
void TFT_ST7789::Init(void)
{
	static const uint8_t initCmds[] PROGMEM
	{	// Copied from Adafruit_ST7789's generic_st7789
		// Edited - removed delays (no documented reason for them other than
		//			reset and wake/sleep.)
		eCOLMODCmd, 1,	// Interface Pixel Format
		0x55,			//    16-bit color
		// Edited - removed commands performed elsewhere (MADCTL, CASET, RASET)
		eINVONCmd, 0,	// Display Inversion On
		eNORONCmd, 0,	// Normal Display Mode On
		eDISPONCmd, 0,	// Display on
		0
	};
	BeginTransaction();
	// Init does a hardware or software reset depending on whether the reset
	// pin is defined followed by a Wake command.
	TFT_ST77XX::Init();
	WriteCmds(initCmds);
	EndTransaction();
}
