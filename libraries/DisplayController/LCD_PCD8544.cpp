/*
*	LCD_PCD8544.cpp, Copyright Jonathan Mackey 2019
*	Class to control the LCD PCD8544 display controller used by the Nokia 5110.
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
#include "LCD_PCD8544.h"
#include <DataStream.h>
#include "Arduino.h"

/********************************* LCD_PCD8544 *********************************/
/*
*	inCSPin and inResetPin are optional.  Pass -1 if it doesn't exist.
*	Note that without as CS pin you can only have one SPI device.
*/
LCD_PCD8544::LCD_PCD8544(
	uint8_t		inDCPin,
	int8_t		inResetPin,	
	int8_t		inCSPin)
	: DisplayController(48/8, 84),
	// According to the docs the maximum clock frequency is 4MHz
	  mSPISettings(4000000, MSBFIRST, SPI_MODE0),
	  mCSPin(inCSPin), mDCPin(inDCPin), mResetPin(inResetPin),
	  mStartColumn(0), mEndColumn(83), mStartRow(0), mEndRow(5)

{
	if (mCSPin >= 0)
	{
		mChipSelBitMask = digitalPinToBitMask(mCSPin);
		mChipSelPortReg = portOutputRegister(digitalPinToPort(mCSPin));
	}
	mDCBitMask = digitalPinToBitMask(mDCPin);
	mDCPortReg = portOutputRegister(digitalPinToPort(mDCPin));
}

/*********************************** begin ************************************/
void LCD_PCD8544::begin(
	uint8_t	inContrast,
	uint8_t	inBias)
{
	if (mCSPin >= 0 &&
		mCSPin != SS)	// If it's SS, SPI.begin() takes care of initializing it.
	{
		digitalWrite(mCSPin, HIGH);
		pinMode(mCSPin, OUTPUT);
	}
	SPI.begin();
	digitalWrite(mDCPin, HIGH);
	pinMode(mDCPin, OUTPUT);

	if (mResetPin >= 0)
	{
		pinMode(mResetPin, OUTPUT);
		digitalWrite(mResetPin, HIGH);
		delay(1);
		digitalWrite(mResetPin, LOW);
		delay(1);
		digitalWrite(mResetPin, HIGH);
	}
	
	BeginTransaction();
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(eFunctionSetCmd | eExtendedInst);	// Go to extended inst mode
	SPI.transfer(eBiasSystemCmd | (inBias & eBiasMask));
	SPI.transfer(eSetVOPCmd | (inContrast & eVOPMask));
	SPI.transfer(eTempContCmd);
	SPI.transfer(eFunctionSetCmd);	// Go to basic inst mode
	SPI.transfer(eDisplayContCmd | eNormalMode);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
	EndTransaction();
}

/********************************** WriteCmd **********************************/
/*
*	Low level, does not begin/end a transaction.
*/
inline void LCD_PCD8544::WriteCmd(
	uint8_t	inCmd) const
{
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(inCmd);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
}

/********************************* IncCoords **********************************/
/*
*	Implements constraining to a fixed window based on the current addressing
*	mode.
*/
void LCD_PCD8544::IncCoords(void)
{
	if (mAddressingMode)	// If vertical
	{
		mDataRow++;
		if (mDataRow > mEndRow)
		{
			mDataRow = mStartRow;
			mDataColumn++;
			if (mDataColumn > mEndColumn)
			{
				mDataColumn = mStartColumn;
			}
			WriteCmd(eSetXAddrCmd | mDataColumn);
			WriteCmd(eSetYAddrCmd | mDataRow);
		}
	} else
	{
		mDataColumn++;
		if (mDataColumn > mEndColumn)
		{
			mDataColumn = mStartColumn;
			mDataRow++;
			if (mDataRow > mEndRow)
			{
				mDataRow = mStartRow;
			}
			WriteCmd(eSetXAddrCmd | mDataColumn);
			WriteCmd(eSetYAddrCmd | mDataRow);
		}
	}
}

/********************************* WriteData **********************************/
void LCD_PCD8544::WriteData(
	const uint8_t*	inData,
	uint16_t		inDataLen)
{
	// mDataRow and mDataColumn should be setup prior to calling this routine.
	for (; inDataLen; inDataLen--)
	{
		SPI.transfer(*(inData++));
		IncCoords();
	}
}

/*********************************** Sleep ************************************/
void LCD_PCD8544::Sleep(void)
{
	Fill(0);	// Per doc, clear display before sleep.
	BeginTransaction();
	WriteCmd(eFunctionSetCmd | ePowerDownChip);
	EndTransaction();
}

/*********************************** WakeUp ***********************************/
void LCD_PCD8544::WakeUp(void)
{
	BeginTransaction();
	WriteCmd(eFunctionSetCmd | (mAddressingMode ? eVAddressingMode : 0));
	EndTransaction();
}

/********************************* FillPixels *********************************/
void LCD_PCD8544::FillPixels(
	uint16_t	inPixelsToFill,
	uint16_t	inFillColor)
{
	mDataRow = mRow;
	mDataColumn = mColumn;
	uint8_t	fillData = inFillColor ? 0xFF : 0;
	BeginTransaction();

	for (; inPixelsToFill; inPixelsToFill--)
	{
		SPI.transfer(fillData);
		IncCoords();
	}
	EndTransaction();
}

/*********************************** MoveTo ***********************************/
// No bounds checking.  Blind move.
void LCD_PCD8544::MoveTo(
	uint16_t	inRow,
	uint16_t	inColumn)
{
	mRow = inRow;
	mColumn = inColumn;
	BeginTransaction();
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(eSetYAddrCmd | inRow);
	SPI.transfer(eSetXAddrCmd | inColumn);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
	EndTransaction();
}

/********************************* MoveToRow **********************************/
// No bounds checking.  Blind move.
void LCD_PCD8544::MoveToRow(
	uint16_t inRow)
{
	mRow = inRow;
	BeginTransaction();
	WriteCmd(eSetYAddrCmd | inRow);
	EndTransaction();
}

/******************************** MoveToColumn ********************************/
// No bounds checking.  Blind move.
void LCD_PCD8544::MoveToColumn(
	uint16_t inColumn)
{
	mColumn = inColumn;
	BeginTransaction();
	WriteCmd(eSetXAddrCmd | inColumn);
	EndTransaction();
}

/******************************* SetColumnRange *******************************/
void LCD_PCD8544::SetColumnRange(
	uint16_t	inStartColumn,
	uint16_t	inEndColumn)
{
	// & 7F keeps it sane, but doesn't make it valid if it's out of range
	mStartColumn = inStartColumn & 0x7F;
	mEndColumn = inEndColumn & 0x7F;
	BeginTransaction();
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(eSetXAddrCmd | mColumn);
	// To mimic the ST77xx controllers, reset the controller's row
	SPI.transfer(eSetYAddrCmd | mRow);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
	EndTransaction();
}

/******************************* SetRowRange *******************************/
void LCD_PCD8544::SetRowRange(
	uint16_t	inStartRow,
	uint16_t	inEndRow)
{
	// & 7 keeps it sane, but doesn't make it valid if it's out of range
	mStartRow = inStartRow & 7;
	mEndRow = inEndRow & 7;
	BeginTransaction();
	WriteCmd(eSetYAddrCmd | mRow);
	EndTransaction();
}

/******************************** StreamCopy **********************************/
void LCD_PCD8544::StreamCopy(
	DataStream*	inDataStream,
	uint16_t	inPixelsToCopy)
{
	BeginTransaction();
	uint8_t	buffer[32];
	mDataRow = mRow;
	mDataColumn = mColumn;
	while (inPixelsToCopy)
	{
		uint16_t pixelsToWrite = inPixelsToCopy > 32 ? 32 : inPixelsToCopy;
		inPixelsToCopy -= pixelsToWrite;
		inDataStream->Read(pixelsToWrite, buffer);
		WriteData(buffer, pixelsToWrite);
	}
	EndTransaction();
}

/***************************** SetAddressingMode ******************************/
void LCD_PCD8544::SetAddressingMode(
	EAddressingMode	inAddressingMode)
{
	if (inAddressingMode != mAddressingMode)
	{
		mAddressingMode = inAddressingMode;
		BeginTransaction();
		WriteCmd(eFunctionSetCmd | (inAddressingMode ? eVAddressingMode : 0));
		EndTransaction();
		if (inAddressingMode == eHorizontal)
		{
			SetRowRange(mRow, mRows-1);
		}
	}
}
