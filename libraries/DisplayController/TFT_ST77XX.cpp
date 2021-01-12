/*
*	TFT_ST77XX.cpp, Copyright Jonathan Mackey 2019
*	Base class of the SPI TFT ST77XX display controller family.
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
#include "TFT_ST77XX.h"
#include <DataStream.h>
#include "Arduino.h"


/********************************* TFT_ST77XX *********************************/
/*
*	inCSPin and inResetPin are optional.  Pass -1 if it doesn't exist.
*	inResetPin is highly recommended because software reset doesn't always work.
*	Note that without as CS pin you can only have one SPI device.
*/
TFT_ST77XX::TFT_ST77XX(
	uint8_t		inDCPin,
	int8_t		inResetPin,	
	int8_t		inCSPin,
	int8_t		inBacklightPin,
	uint16_t	inHeight,
	uint16_t	inWidth,
	bool		inCentered,
	bool		inIsBGR)
	: DisplayController(inHeight, inWidth),
	// According to the docs for the 35 and 89 controllers, the min write
	// cycle is 66ns or approximately 15Mhz
	  mSPISettings(15000000, MSBFIRST, SPI_MODE3),
	  mCSPin(inCSPin), mDCPin(inDCPin), mResetPin(inResetPin),
	  mBacklightPin(inBacklightPin), mRowOffset(0), mColOffset(0),
	  mCentered(inCentered), mIsBGR(inIsBGR)
{
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
void TFT_ST77XX::begin(
	uint8_t	inRotation,
	bool	inResetLevel)
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
		mResetLevel = inResetLevel;
		pinMode(mResetPin, OUTPUT);
		digitalWrite(mResetPin, !mResetLevel);
	}
	Init();
	SetRotation(inRotation);
}

/************************************ Init ************************************/
void TFT_ST77XX::Init(void)
{
	if (mResetPin >= 0)
	{
		delay(1);
		digitalWrite(mResetPin, mResetLevel);
		delay(1);
		digitalWrite(mResetPin, !mResetLevel);
	} else
	{
		WriteCmd(eSWRESETCmd);
	}
	// Per docs: After reset, delay 120ms before sending the next command.
	// (The controller IC is in the process of writing the defaults.)
	delay(120);
	WriteWakeUpCmds();	// By default the controller is asleep after reset.
}

/********************************** WriteCmd **********************************/
inline void TFT_ST77XX::WriteCmd(
	uint8_t	inCmd) const
{
	*mDCPortReg &= ~mDCBitMask;	// Command mode (LOW)
	SPI.transfer(inCmd);
	*mDCPortReg |= mDCBitMask;	// Data mode (HIGH)
}

/********************************** WriteCmd **********************************/
void TFT_ST77XX::WriteCmd(
	uint8_t			inCmd,
	const uint8_t*	inCmdData,
	uint8_t			inCmdDataLen,
	bool			inPGM) const
{
	WriteCmd(inCmd);
	WriteData(inCmdData, inCmdDataLen, inPGM);
}

/********************************* WriteCmds **********************************/
/*
*	A null terminated array of commands of the format:
*	{Cmd, cmdDataLen, [cmdData]}, ... {0}
*	inCmds is an address in PROGMEM
*/
void TFT_ST77XX::WriteCmds(
	const uint8_t*	inCmds) const
{
	uint8_t	cmd, cmdDataLen;
	while (true)
	{
		cmd = pgm_read_byte(inCmds++);
		if (cmd)
		{
			WriteCmd(cmd);
			cmdDataLen = pgm_read_byte(inCmds++);
			if (cmdDataLen)
			{
				do 
				{
					SPI.transfer(pgm_read_byte(inCmds++));
				} while (--cmdDataLen);
			}
			continue;
		}
		break;
	}
}

/********************************* WriteData **********************************/
void TFT_ST77XX::WriteData(
	const uint8_t*	inData,
	uint16_t		inDataLen,
	bool			inPGM) const
{
	if (inDataLen)
	{
		if (inPGM)
		{
			do 
			{
				SPI.transfer(pgm_read_byte(inData++));
			} while (--inDataLen);
		} else
		{
			do
			{
				SPI.transfer(*(inData++));
			} while (--inDataLen);
		}
	}
}

/******************************** WriteData16 *********************************/
/*
*	inData is an address in SRAM
*/
void TFT_ST77XX::WriteData16(
	const uint16_t*	inData,
	uint16_t		inDataLen) const
{
	if (inDataLen)
	{
		const uint8_t*	data = (const uint8_t*)inData;
		const uint8_t*	endData = data + (inDataLen*2);
		do
		{
			uint8_t	lsb = *(data++);
			SPI.transfer(*(data++));
			SPI.transfer(lsb);
		} while (data < endData);
	}
}

/******************************** SetRotation *********************************/
void TFT_ST77XX::SetRotation(
	uint8_t	inRotation)
{
/*
	MADCTL
		MY	MX	MV	
[0]	0	0	0	0	
[1]	90	0	1	1	
[2]	180	1	1	0	
[3]	270	1	0	1	

	Bit mask
MY  = 0x80
MX  = 0x40
MV  = 0x20
ML  = 0x10 (when set it's bottom to top)
RGB = 0x08 (when set it's BGR)
MH  = 0x04 (when set it's right to left)

For both the 89 and 35R the RGB order is BGR.

*/
	inRotation &= 3;
	/*
	*	It looks like there's a manufacturing mistake in the 160x80 displays
	*	that I have (maybe all?) where RGB is BGR based on the controller
	*	RGB/BGR order bit is the opposite of what you expect.  So either there's
	*	a bug in the ST7735S controller or the display manufacturer screwed up.
	*
	*	This is the reason for the mSwapRB flag.  
	*
	*	The other thing about the 160x80 display is that rather than pick 0,0
	*	to be the origin, they centered it in ST7735S's 162x132 memory window,
	*	so all xy values need to be offset regardless of rotation.  For the
	*	240x240 display they did not center it, so you only need to offset when
	*	rotating by 180 and 270.
	*
	*	This is the reason for the mCentered flag.
	*/
	static const uint8_t cmdData[] PROGMEM = {0x08, 0x68, 0xC8, 0xA8};
	uint8_t	madctlParam = pgm_read_byte(&cmdData[inRotation]);
	if (mIsBGR)
	{
		madctlParam &= ~8;
	}
	BeginTransaction();
	WriteCmd(eMADCTLCmd);
	SPI.transfer(madctlParam);
	EndTransaction();
	{
		uint8_t	vDelta = VerticalRes() - mRows;
		uint8_t	hDelta = HorizontalRes() - mColumns;
		if (mCentered)
		{
			mRowOffset = vDelta /= 2;
			mColOffset = hDelta /= 2;
		} else
		{
			if (inRotation & 2)
			{
				mRowOffset = vDelta;
				mColOffset = hDelta;
			} else
			{
				mRowOffset = 0;
				mColOffset = 0;
			}
		}
	}
	if (inRotation & 1)
	{
		uint16_t	temp = mRows;
		mRows = mColumns;
		mColumns = temp;
		temp = mRowOffset;
		mRowOffset = mColOffset;
		mColOffset = temp;
	}
}

/*********************************** Sleep ************************************/
void TFT_ST77XX::Sleep(void)
{
	BeginTransaction();
	WriteSleepCmds();
	EndTransaction();
}

/******************************* WriteSleepCmds *******************************/
void TFT_ST77XX::WriteSleepCmds(void)
{
	WriteCmd(eSLPINCmd);
	// Per docs: When going to Sleep, delay 120ms before sending the next command.
	// delay(120);  << This assumes there will be no commands after calling
	// this routine.
	
	if (mBacklightPin >= 0)
	{
		digitalWrite(mBacklightPin, LOW);	// Off
	}
}

/*********************************** WakeUp ***********************************/
void TFT_ST77XX::WakeUp(void)
{
	BeginTransaction();
	WriteWakeUpCmds();
	EndTransaction();
}

/****************************** WriteWakeUpCmds *******************************/
void TFT_ST77XX::WriteWakeUpCmds(void)
{
	WriteCmd(eSLPOUTCmd);
	// Per docs: When waking from Sleep, delay 120ms before sending the next command.
	delay(120);
	if (mBacklightPin >= 0)
	{
		digitalWrite(mBacklightPin, HIGH);	// On
	}

}

/********************************* FillPixels *********************************/
void TFT_ST77XX::FillPixels(
	uint16_t	inPixelsToFill,
	uint16_t	inFillColor)
{
	uint8_t	msb = inFillColor >> 8;
	uint8_t	lsb = inFillColor;
	BeginTransaction();
	for (; inPixelsToFill; inPixelsToFill--)
	{
		SPI.transfer(msb);
		SPI.transfer(lsb);
	}
	EndTransaction();
}

/*********************************** MoveTo ***********************************/
// No bounds checking.  Blind move.
void TFT_ST77XX::MoveTo(
	uint16_t	inRow,
	uint16_t	inColumn)
{
	MoveToRow(inRow);
	mColumn = inColumn;
}

/********************************* MoveToRow **********************************/
// No bounds checking.  Blind move.
void TFT_ST77XX::MoveToRow(
	uint16_t inRow)
{
	uint16_t	rows[] = {inRow + mRowOffset, mRows + mRowOffset -1};

	BeginTransaction();
	WriteCmd(eRASETCmd);
	WriteData16(rows, 2);
	EndTransaction();
	mRow = inRow;
}

/******************************** MoveToColumn ********************************/
// No bounds checking.  Blind move.
// Doesn't actually make any changes to the controller.  Used by  other
// functions to set the start column relative to mColumn.
// (e.g. the relative version of SetColumnRange as defined in DisplayController)
void TFT_ST77XX::MoveToColumn(
	uint16_t inColumn)
{
	mColumn = inColumn;
}

/******************************* SetColumnRange *******************************/
void TFT_ST77XX::SetColumnRange(
	uint16_t	inStartColumn,
	uint16_t	inEndColumn)
{
	uint16_t	columns[] = {inStartColumn + mColOffset, inEndColumn + mColOffset};

	BeginTransaction();
	WriteCmd(eCASETCmd);
	WriteData16(columns, 2);
	WriteCmd(eRAMWRCmd); // Resets controller memory ptr to inStartColumn and
						 // the start of current the row frame 
	EndTransaction();
}

/******************************* SetRowRange *******************************/
void TFT_ST77XX::SetRowRange(
	uint16_t	inStartRow,
	uint16_t	inEndRow)
{
	uint16_t	rows[] = {inStartRow + mRowOffset, inEndRow + mRowOffset};

	BeginTransaction();
	WriteCmd(eRASETCmd);
	WriteData16(rows, 2);
	// Does not send a start RAM write command.
	// SetRowRange should be called before SetColumnRange.
	EndTransaction();
}

/******************************** StreamCopy **********************************/
void TFT_ST77XX::StreamCopy(
	DataStream*	inDataStream,	// A 16 bit data stream
	uint16_t	inPixelsToCopy)
{
	BeginTransaction();
	uint16_t	buffer[32];
	while (inPixelsToCopy)
	{
		uint16_t pixelsToWrite = inPixelsToCopy > 32 ? 32 : inPixelsToCopy;
		inPixelsToCopy -= pixelsToWrite;
		inDataStream->Read(pixelsToWrite, buffer);
		WriteData16(buffer, pixelsToWrite);
	}
	EndTransaction();
}

/******************************** CopyPixels **********************************/
void TFT_ST77XX::CopyPixels(
	const void*		inPixels,
	uint16_t		inPixelsToCopy)
{
	BeginTransaction();
	WriteData16((const uint16_t*)inPixels, inPixelsToCopy);
	EndTransaction();
}


