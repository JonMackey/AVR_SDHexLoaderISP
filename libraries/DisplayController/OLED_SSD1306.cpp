/*
*	OLED_SSD1306.cpp, Copyright Jonathan Mackey 2019
*	Class to control an I2C OLED SSD1306 display controller.
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
#include <Wire.h>
#include "OLED_SSD1306.h"
#include <DataStream.h>
#include "Arduino.h"


/******************************** OLED_SSD1306 ********************************/
OLED_SSD1306::OLED_SSD1306(
	uint8_t	inI2CAddress,
	uint8_t	inHeight,
	uint8_t	inWidth)
	: DisplayController(inHeight/8, inWidth), mHeight(inHeight),
	  mI2CAddr(inI2CAddress)
{
}

/*********************************** begin ************************************/
void OLED_SSD1306::begin(
	bool	inRotate180)
{
	Init(inRotate180);
}

/************************************ Init ************************************/
void OLED_SSD1306::Init(
	bool	inRotate180)
{

	/*
	*	Currently only handles the two most popular sizes, 128x64 and 128x32
	*/
	static const uint8_t initCmdsP[] PROGMEM = 
	{
		0x00,				//	0 Sending commands, Co = 0, D/C# = 0
		
		eDisplayOffCmd,		//	1 Display off
		
		eAddrModeCmd,		//	2 Memory addressing mode
		0x00,				//	3 horizontal
		
		0x40,				//	4 Display start line

		0x81,				//	5 Contrast control
		0xCF,				//	6 8F for 32, CF for 64
		
		0x8D,				//	7 Charge pump setting
		0x14,				//	8 0x14 = Enable Charge Pump, 0x10 = Disable
		
		0xA0,				//	9 Segment Re-map 0xA1 to rotate screen 180 deg
		
		0xA8,				//	10 Multiplex ratio
		63,					//	11 Height -1
		
		0xC0,				//	12 COM Output Scan Direction  0xC8 to rotate screen 180 Deg

		0xD3,				//	13 Display offset
		0x0,				//	14 none
		
		0xD5,				//	15 Display clock divide ratio/oscillator frequency
		0x80,				//	16 Default ratio 0x80
		
		0xD9,				//	17 Pre-charge period 
		0xF1,				//	18
		
		0xDA,				//	19 COM pins hardware config
		0x12,				//	20 0x12 for 64, 0x2 for 32
		
		0xDB,				//	21 VCOMH deselect level  
		0x40,				//	22
		
		0xA4,				//	23 Entire display ON, resume to RAM content display 
		       
		0xA6,				//	24 Normal display
		             	
		eDisplayOnCmd		//	24 Display On            	
	};
	uint8_t	initCmds[sizeof(initCmdsP)];
	memcpy_P(initCmds, initCmdsP, sizeof(initCmdsP));
	
	if (mHeight == 32)
	{
		initCmds[6] = 0x8F;
		initCmds[11] = 31; // Height-1;
		initCmds[20] = 2;
	}
	if (inRotate180)
	{
		initCmds[9] = 0xA1;
		initCmds[12] = 0xC8;
	}
	WriteBytes(initCmds, sizeof(initCmdsP));
}

/********************************** WriteCmd ***********************************/
void OLED_SSD1306::WriteCmd(
	uint8_t	inCmd)
{
	Wire.beginTransmission(mI2CAddr);
	Wire.write(0);  // Co = 0, D/C# = 0
	Wire.write(inCmd);
	Wire.endTransmission(); 
}

/********************************* WriteBytes **********************************/
// Total bytes to send, inLength, must be lessthan 32 (wire limitation)
// First byte determines command or data (0 for command, 0x40 for data)
void OLED_SSD1306::WriteBytes(
	const uint8_t*	inBytes,
	uint16_t		inLength)
{
	Wire.beginTransmission(mI2CAddr);
	Wire.write(inBytes, inLength);
	Wire.endTransmission();
}

/*********************************** MoveTo ***********************************/
// No bounds checking.  Blind move.
void OLED_SSD1306::MoveTo(
	uint16_t	inRow,	// aka page
	uint16_t	inColumn)
{
	uint8_t	cmds[] = {0, (uint8_t)(0xB0+inRow), (uint8_t)(inColumn & 0xF), (uint8_t)(0x10+(inColumn >> 4))};
	WriteBytes(cmds, sizeof(cmds));
	mRow = inRow;
	mColumn = inColumn;
}\

/********************************* MoveToRow **********************************/
// No bounds checking.  Blind move.
void OLED_SSD1306::MoveToRow(
	uint16_t inRow)	// aka page
{
	WriteCmd(0xB0+inRow);			// Set page address
	mRow = inRow;
}

/******************************** MoveToColumn ********************************/
// No bounds checking.  Blind move.
void OLED_SSD1306::MoveToColumn(
	uint16_t inColumn)
{
	uint8_t	cmds[] = {0, (uint8_t)(inColumn & 0xF), (uint8_t)(0x10+(inColumn >> 4))};
	WriteBytes(cmds, sizeof(cmds));
	mColumn = inColumn;
}

/*********************************** Sleep ************************************/
void OLED_SSD1306::Sleep(void)
{
	WriteCmd(eDisplayOffCmd);
}

/*********************************** WakeUp ***********************************/
void OLED_SSD1306::WakeUp(void)
{
	WriteCmd(eDisplayOnCmd);
}

/********************************* FillPixels *********************************/
void OLED_SSD1306::FillPixels(
	uint16_t	inBytesToFill,
	uint16_t	inFillColor)
{
	uint8_t	fillData = inFillColor ? 0xFF : 0;
	while (inBytesToFill)
	{
		// Wire has a 32 byte transmission limit so clear the data in 31 byte chunks
		uint16_t bytesToWrite = inBytesToFill > 31 ? 31 : inBytesToFill;
		inBytesToFill -= bytesToWrite;
		Wire.beginTransmission(mI2CAddr);
		Wire.write(0x40);  // Sending data, Co = 0, D/C# = 1  (counts as 1 of the 32)
		for (; bytesToWrite; bytesToWrite--)
		{
			Wire.write(fillData);
		}
		Wire.endTransmission();
	}
}

/******************************* SetColumnRange *******************************/
void OLED_SSD1306::SetColumnRange(
	uint16_t	inStartColumn,
	uint16_t	inEndColumn)
{
	uint8_t	columnAddrCmd[] = {0x0, eSetColAddrCmd, (uint8_t)inStartColumn, (uint8_t)inEndColumn};
	WriteBytes(columnAddrCmd, sizeof(columnAddrCmd));
}

/******************************** SetRowRange *********************************/
void OLED_SSD1306::SetRowRange(
	uint16_t	inStartPage,
	uint16_t	inEndPage)
{
	uint8_t	pageAddrCmd[] = {0x0, eSetPageAddrCmd, (uint8_t)inStartPage, (uint8_t)inEndPage};
	WriteBytes(pageAddrCmd, sizeof(pageAddrCmd));
}

/******************************** StreamCopy **********************************/
void OLED_SSD1306::StreamCopy(
	DataStream*	inDataStream,
	uint16_t	inPixelsToCopy)
{
	uint8_t	buffer[32];
	buffer[0] = 0x40; // Sending data, Co = 0, D/C# = 1  (counts as 1 of the 32)
	while (inPixelsToCopy)
	{
		// Wire has a 32 byte transmission limit so copy the data in 31 byte chunks
		uint16_t bytesToWrite = inPixelsToCopy > 31 ? 31 : inPixelsToCopy;
		inPixelsToCopy -= bytesToWrite;
		inDataStream->Read(bytesToWrite, &buffer[1]);
		WriteBytes(buffer, bytesToWrite+1);
	}
}

/***************************** SetAddressingMode ******************************/
void OLED_SSD1306::SetAddressingMode(
	EAddressingMode	inAddressingMode)
{
	if (inAddressingMode != mAddressingMode)
	{
		mAddressingMode = inAddressingMode;
		uint8_t	addrModeCmd[] = {0x0, eAddrModeCmd, (uint8_t)inAddressingMode};
		WriteBytes(addrModeCmd, sizeof(addrModeCmd));
		if (inAddressingMode == eHorizontal)
		{
			SetRowRange(mRow, mRows-1);
		}
	}
}


