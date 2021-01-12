/*
*	TFT_ST7735S.cpp, Copyright Jonathan Mackey 2019
*	Class to control an SPI TFT ST7735S display controller.
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
#include "TFT_ST7735S.h"
#include <DataStream.h>
#include "Arduino.h"


/******************************** TFT_ST7735S ********************************/
TFT_ST7735S::TFT_ST7735S(
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
void TFT_ST7735S::Init(void)
{
	// These settings were blindly copied from Adafruit_ST7735.cpp
	static const uint8_t initCmds[] PROGMEM
	{	// Copied from Adafruit_ST7735's Rcmd1
		// Edited - removed delays (no documented reason for them other than
		//			reset and wake/sleep.)
		eFRMCTR1Cmd, 3,	// Frame Rate Control
		0x01, 0x2C, 0x2D,
		eFRMCTR2Cmd, 3,	// Frame Rate Control
		0x01, 0x2C, 0x2D,
		eFRMCTR3Cmd, 6,	// Frame Rate Control
		0x01, 0x2C, 0x2D,
		0x01, 0x2C, 0x2D,
		eINVCTRCmd, 1,	// Display Inversion Control
    	0x07,			//	 No inversion
		ePWCTR1Cmd, 3,	// Power Control 1
    	0xA2, 0x02, 0x84,
		ePWCTR2Cmd, 1,	// Power Control 2
    	0xC5,
		ePWCTR3Cmd, 2,	// Power Control 3 (in Normal mode/ Full colors)
    	0x0A, 0x00,
		ePWCTR4Cmd, 2,	// Power Control 4 (in Idle mode/ 8-colors)
    	0x8A, 0x2A,
		ePWCTR5Cmd, 2,	// Power Control 5 (in Partial mode/ full-colors)
    	0x8A, 0xEE,
		eVMCTR1Cmd, 1,	// VCOM Control 1
    	0x0E,
		eINVOFFCmd, 0,	// Display Inversion Off
		eCOLMODCmd, 1,	// Interface Pixel Format
		0x05,			//	16-bit color
		// end Rcmd1
		// Edited - removed commands performed elsewhere (MADCTL, CASET, RASET)
		// Copied from Adafruit_ST7735's Rcmd3
		eGMCTRP1Cmd, 16,	// Gamma (‘+’polarity) Correction Characteristics Setting
		0x02, 0x1c, 0x07, 0x12,	//	(Not entirely necessary, but provides
		0x37, 0x32, 0x29, 0x2d,	//	accurate colors)
		0x29, 0x25, 0x2B, 0x39,
		0x00, 0x01, 0x03, 0x10,
		eGMCTRP1Cmd, 16,	// Gamma (‘-’polarity) Correction Characteristics Setting
		0x03, 0x1d, 0x07, 0x06,	//	(Not entirely necessary, but provides
		0x2E, 0x2C, 0x29, 0x2D,	//	accurate colors)
		0x2E, 0x2E, 0x37, 0x3F,
		0x00, 0x00, 0x02, 0x10,
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
