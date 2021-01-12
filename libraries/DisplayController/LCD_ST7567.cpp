/*
*	LCD_ST7567.h, Copyright Jonathan Mackey 2020
*	Class to control an SPI LCD ST7567 display controller.
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
#include "LCD_ST7567.h"
#include <DataStream.h>
#include "Arduino.h"


/********************************* LCD_ST7567 *********************************/
/*
*	inCSPin and inResetPin are optional.  Pass -1 if it doesn't exist.
*	inResetPin is highly recommended because software reset doesn't always work.
*	Note that without as CS pin you can only have one SPI device.
*/
LCD_ST7567::LCD_ST7567(
	uint8_t		inDCPin,
	int8_t		inResetPin,	
	int8_t		inCSPin,
	int8_t		inBacklightPin,
	uint16_t	inHeight,
	uint16_t	inWidth)
	: DisplayController(inHeight/8, inWidth),
	// According to the docs, the min write cycle is 50ns or approximately 20MHz
	  mSPISettings(15000000, MSBFIRST, SPI_MODE3),
	  mCSPin(inCSPin), mDCPin(inDCPin), mResetPin(inResetPin),
	  mBacklightPin(inBacklightPin), mStartColumn(0), mEndColumn(0),
	  mStartRow(0), mColOffset(0)
{
	mEndRow = mRows-1;
	// Setting the CS pin mode and state was moved from begin to avoid
	// interference with other SPI devices on the bus.
	if (mCSPin >= 0)
	{
		mChipSelBitMask = digitalPinToBitMask(mCSPin);
		mChipSelPortReg = portOutputRegister(digitalPinToPort(mCSPin));
		digitalWrite(mCSPin, HIGH);
		pinMode(mCSPin, OUTPUT);
	}
	mDCBitMask = digitalPinToBitMask(mDCPin);
	mDCPortReg = portOutputRegister(digitalPinToPort(mDCPin));
}

/*********************************** begin ************************************/
void LCD_ST7567::begin(
	uint8_t	inRotation)
{
	if (mBacklightPin >= 0)
	{
		pinMode(mBacklightPin, OUTPUT);
		digitalWrite(mBacklightPin, LOW);	// Off
	}
	digitalWrite(mDCPin, HIGH);
	pinMode(mDCPin, OUTPUT);

	if (mResetPin >= 0)
	{
		pinMode(mResetPin, OUTPUT);
		digitalWrite(mResetPin, HIGH);
	}
	Init();
	SetRotation(inRotation);
}

/************************************ Init ************************************/
void LCD_ST7567::Init(void)
{
	BeginTransaction();
	if (mResetPin >= 0)
	{
		delay(1);	// doc says delay 1ms for power to stabilize
		digitalWrite(mResetPin, LOW);
		delayMicroseconds(6);
		digitalWrite(mResetPin, HIGH);
	} else
	{
		WriteCmd(eSWResetCmd);
	}
	// Per docs: After reset, delay > 5us before sending the next command.
	// (The controller IC is in the process of writing the defaults.)
	delayMicroseconds(6);
	WriteCmd(eSet1_7BiasRatioCmd);// Bias ration 1/7 (1/9 after HW reset)
	WriteCmd(eRegulationRatioCmd + eRatio6_0);// Regulation ratio 6.0
	WriteCmd(eSetContrastCmd, 0x1F);// Set Contrast to 0x1F (in conjunction with regulation ratio)
	WriteCmd(ePowerControlOnCmd);// Turn on regulators	
	WriteWakeUpCmds();	// By default the controller is asleep after reset.
	EndTransaction();
}

/********************************** WriteCmd **********************************/
inline void LCD_ST7567::WriteCmd(
	uint8_t	inCmd) const
{
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(inCmd);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
}

/********************************** WriteCmd **********************************/
void LCD_ST7567::WriteCmd(
	uint8_t			inCmd,
	uint8_t			inCmdData) const
{
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(inCmd);
	SPI.transfer(inCmdData);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
}

/********************************* IncCoords **********************************/
/*
*	Implements constraining to a fixed window based on the current addressing
*	mode.
*/
void LCD_ST7567::IncCoords(void)
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
		}
		/*
		*	The ST7567 doesn't support vertical addressing.  This means that the
		*	column is auto incremented after each write.  For vertical we don't
		*	want this behavior.  The column and page are set after each write.
		*/
		WriteSetColAddrCmd(mDataColumn);
		WriteSetPageStartCmd(mDataRow);
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
			WriteSetColAddrCmd(mDataColumn);
			WriteSetPageStartCmd(mDataRow);
		}
	}
}

/********************************* WriteData **********************************/
void LCD_ST7567::WriteData(
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

/******************************** SetRotation *********************************/
void LCD_ST7567::SetRotation(
	uint8_t	inRotation)
{
/*
		MY	MX
[0]	0	1	0
[1]	90	Not supported, treated as 0
[2]	180	0	1
[3]	270	Not supported, treated as 180
*/
	inRotation &= 3;

	BeginTransaction();

	if (inRotation & 2)
	{
		// Need to offset all columns by the delta of the max width supported by
		// the contoller and the actual width of the display.
		mColOffset = HorizontalRes() - mColumns;
		WriteCmd(eComDirNormalCmd);		// MY = 0
		WriteCmd(eSegDirInvertedCmd);	// MX = 1
	} else
	{
		// A row offset not needed here.  Seems to map OK, although it may be
		// needed on a display with an actual height less than 64.
		// If needed use bottleneck WriteSetPageStartCmd().
		WriteCmd(eComDirInvertedCmd);	// MY = 1
		WriteCmd(eSegDirNormalCmd);		// MX = 0
		mColOffset = 0;
	}
	EndTransaction();
}

/*********************************** Sleep ************************************/
void LCD_ST7567::Sleep(void)
{
	BeginTransaction();
	WriteSleepCmds();
	EndTransaction();
}

/******************************* WriteSleepCmds *******************************/
void LCD_ST7567::WriteSleepCmds(void)
{
	WriteCmd(eDisplayOffCmd);
	WriteCmd(eAllPixelsOnCmd);
	
	if (mBacklightPin >= 0)
	{
		digitalWrite(mBacklightPin, LOW);	// Off
	}
}

/*********************************** WakeUp ***********************************/
void LCD_ST7567::WakeUp(void)
{
	BeginTransaction();
	WriteWakeUpCmds();
	EndTransaction();
}

/****************************** WriteWakeUpCmds *******************************/
void LCD_ST7567::WriteWakeUpCmds(void)
{
	WriteCmd(eAllPixelsNormalCmd);
	WriteCmd(eDisplayOnCmd);
	if (mBacklightPin >= 0)
	{
		digitalWrite(mBacklightPin, HIGH);	// On
	}

}

/********************************* FillPixels *********************************/
void LCD_ST7567::FillPixels(
	uint16_t	inPixelsToFill,
	uint16_t	inFillColor)
{
	mDataRow = mRow;
	mDataColumn = mColumn;
	uint8_t	fillData = inFillColor;
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
void LCD_ST7567::MoveTo(
	uint16_t	inRow,
	uint16_t	inColumn)
{
	mRow = inRow;
	mColumn = inColumn;
	BeginTransaction();
	WriteSetColAddrCmd(inColumn);
	WriteSetPageStartCmd(inRow);
	EndTransaction();
}

/********************************* MoveToRow **********************************/
// No bounds checking.  Blind move.
void LCD_ST7567::MoveToRow(
	uint16_t inRow)
{
	mRow = inRow;
	BeginTransaction();
	WriteSetPageStartCmd(inRow);
	EndTransaction();
}

/******************************** MoveToColumn ********************************/
// No bounds checking.  Blind move.
void LCD_ST7567::MoveToColumn(
	uint16_t inColumn)
{
	mColumn = inColumn;
	BeginTransaction();
	WriteSetColAddrCmd(inColumn);
	EndTransaction();
}

/***************************** WriteSetColAddrCmd *****************************/
// Bottleneck to handle column offset.
void LCD_ST7567::WriteSetColAddrCmd(
	uint8_t	inColumn) const
{
	inColumn += mColOffset;
	WriteCmd(eSetColAddrCmd | (inColumn >> 4), inColumn & 0x0F);
}

/**************************** WriteSetPageStartCmd ****************************/
// Bottleneck to handle possible row/page offset.
void LCD_ST7567::WriteSetPageStartCmd(
	uint8_t	inPage) const
{
	WriteCmd(eSetPageStartCmd | inPage);
}

/******************************* SetColumnRange *******************************/
void LCD_ST7567::SetColumnRange(
	uint16_t	inStartColumn,
	uint16_t	inEndColumn)
{
	// & FF keeps it sane, but doesn't make it valid if it's out of range
	mStartColumn = inStartColumn & 0xFF;
	mEndColumn = inEndColumn & 0xFF;
	BeginTransaction();
	WriteSetColAddrCmd(mColumn);
	// To mimic the ST77xx controllers, reset the controller's row
	WriteSetPageStartCmd(mRow);
	EndTransaction();
}

/******************************* SetRowRange *******************************/
void LCD_ST7567::SetRowRange(
	uint16_t	inStartRow,
	uint16_t	inEndRow)
{
	// & 0xF keeps it sane, but doesn't make it valid if it's out of range
	mStartRow = inStartRow & 0xF;
	mEndRow = inEndRow & 0xF;
	BeginTransaction();
	WriteSetPageStartCmd(mRow);
	EndTransaction();
}

/******************************** StreamCopy **********************************/
void LCD_ST7567::StreamCopy(
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
void LCD_ST7567::SetAddressingMode(
	EAddressingMode	inAddressingMode)
{
	if (inAddressingMode != mAddressingMode)
	{
		mAddressingMode = inAddressingMode;
		if (inAddressingMode == eHorizontal)
		{
			SetRowRange(mRow, mRows-1);
		}
	}
}

/********************************** Invert ************************************/
void LCD_ST7567::Invert(
	bool	inInvert)
{
	BeginTransaction();
	WriteCmd(inInvert ? eInvertDispOnCmd : eInvertDispOffCmd);
	EndTransaction();
}

