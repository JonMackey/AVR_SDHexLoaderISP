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
#ifndef LCD_ST7567_h
#define LCD_ST7567_h
#include <SPI.h>
#include "DisplayController.h"

class DataStream;

class LCD_ST7567 : public DisplayController
{
public:
							LCD_ST7567(
								uint8_t					inDCPin,
								int8_t					inResetPin,	
								int8_t					inCSPin,
								int8_t					inBacklightPin = -1,
								uint16_t				inHeight = 64,
								uint16_t				inWidth = 128);

	virtual uint8_t			BitsPerPixel(void) const
								{return(1);}
							// Rotation, one of 0, 1, 2, or 3.
							//			MY	MX	
							// 0	0	1	0	
							// 1	90	Unsuported	
							// 2	180	0	1	
							// 3	270	Unsuported	
	void					begin(
								uint8_t					inRotation = 0);
	virtual void			MoveTo(
								uint16_t				inRow,
								uint16_t				inColumn);
	virtual void			MoveToRow(
								uint16_t				inRow);
	virtual void			MoveToColumn(
								uint16_t				inColumn);
	/*
	*	Turns the display off.
	*/
	virtual void			Sleep(void);
	/*
	*	Turns the display on.
	*/
	virtual void			WakeUp(void);

	/*
	*	FillPixels: Sets a run of inPixelsToFill to inFillColor from the
	*	current position and column clipping.
	*/
	virtual void			FillPixels(
								uint16_t				inPixelsToFill,
								uint16_t				inFillColor);
								
	/*
	/*
	*	SetColumnRange: Sets a the absolute column clipping to inStartColumn to
	*	inEndColumn.
	*/
	virtual void			SetColumnRange(
								uint16_t				inStartColumn,
								uint16_t				inEndColumn);
								
	/*
	*	SetRowRange: Sets a the absolute row range clipping to
	*	inStartRow to inEndRow.
	*/
	virtual void			SetRowRange(
								uint16_t				inStartRow,
								uint16_t				inEndRow);

	/*
	*	StreamCopy: Blindly copies inPixelsToCopy 16 bit pixels from inDataStream
	*	starting at the current row and column.  No checking to see if the data
	*	will fit on the display without clipping, skewing or wrapping.  The
	*	current row and column will be undefined (not updated within the class.)
	*/
	virtual void			StreamCopy(
								DataStream*				inDataStream,
								uint16_t				inPixelsToCopy);
								
	virtual void			SetAddressingMode(
								EAddressingMode			inAddressingMode);
								
	void					Invert(
								bool					inInvert);
protected:
	enum ECmds
	{
		eDisplayOffCmd		= 0xAE,	// Display Off
		eDisplayOnCmd		= 0xAF,	// Display On
		eSetStartLineCmd	= 0x40,	// Start line, + 5 bits that define the line (0 to 63)
		eSetPageStartCmd	= 0xB0,	// Start page, + 4 bits that define the page (0 to 8)
		eSetColAddrCmd		= 0x10,	// Set Column, + 4 MSB, followed by the 4 LSB in a separate byte.
		eSegDirNormalCmd	= 0xA0,	// Normal direction.  X auto increments to 128
		eSegDirInvertedCmd	= 0xA1,	// Inverse direction.  X auto decrements to 0
		eInvertDispOnCmd	= 0xA7,	// Invert Display
		eInvertDispOffCmd	= 0xA6,	// Normal Dislpay (not inverted)
		eAllPixelsOnCmd		= 0xA5,	// Set all pixels on
		eAllPixelsNormalCmd	= 0xA4,	// Normal Dislpay (not forced on)
		eSet1_7BiasRatioCmd	= 0xA3,	// Set Bias ratio 1/7
		eSet1_9BiasRatioCmd	= 0xA2,	// Set Bias ratio 1/9
		eSWResetCmd			= 0xE2,	// Software Reset
		eComDirNormalCmd	= 0xC0,	// Normal direction.  Page auto increments to 8
		eComDirInvertedCmd	= 0xC8,	// Inverse direction.  Page auto decrements to 0
		ePowerControlOnCmd	= 0x2F,	// Controls power regulation.  Off after reset (should be on for operation)
		ePowerControlOffCmd	= 0x28,	// Controls power regulation.  Off after reset (should be on for operation)
		eRegulationRatioCmd	= 0x20,	// Regulation ratio.  + 3 bits (0 to 7) that correspond to the ratio 3.0 to 6.5 respectively.
				eRatio3_0	= 0,
				eRatio3_5,
				eRatio4_0,
				eRatio4_5,
				eRatio5_0,
				eRatio5_5,
				eRatio6_0,
				eRatio6_5,
		eSetContrastCmd		= 0x81,	// Set contrast.  Followed by contrast byte, 0 to 63 (0x3F)
			
	};
	int8_t		mCSPin;
	uint8_t		mDCPin;
	uint8_t		mResetPin;
	int8_t		mBacklightPin;
	uint8_t		mChipSelBitMask;
	uint8_t		mDCBitMask;
	uint8_t		mColOffset;
	uint8_t		mStartColumn;	// For windowing (column range)
	uint8_t		mEndColumn;		// For windowing (column range)
	uint8_t		mStartRow;		// For windowing (row range)
	uint8_t		mEndRow;		// For windowing (row range)
	uint8_t		mDataRow;		// For windowing (IncCoords & FillPixels)
	uint8_t		mDataColumn;	// For windowing (IncCoords & FillPixels)
	volatile uint8_t*	mChipSelPortReg;
	volatile uint8_t*	mDCPortReg;
	SPISettings	mSPISettings;


	virtual void			Init(void);
	void					WriteSleepCmds(void);
	void					WriteWakeUpCmds(void);
	void					SetRotation(
								uint8_t					inRotation);
	inline void				BeginTransaction(void)
							{
								SPI.beginTransaction(mSPISettings);
								if (mCSPin >= 0)
								{
									*mChipSelPortReg &= ~mChipSelBitMask;
								}
							}

	inline void				EndTransaction(void)
							{
								if (mCSPin >= 0)
								{
									*mChipSelPortReg |= mChipSelBitMask;
								}
								SPI.endTransaction();
							}

	void					WriteSetPageStartCmd(
								uint8_t					inPage) const;
	void					WriteSetColAddrCmd(
								uint8_t					inColumn) const;
	inline void				WriteCmd(
								uint8_t					inCmd) const;
	void					WriteCmd(
								uint8_t					inCmd,
								uint8_t					inCmdData) const;
	void					WriteData(
								const uint8_t*			inData,
								uint16_t				inDataLen);
	void					IncCoords(void);
	virtual uint16_t		VerticalRes(void) const
								{return(65);}
	virtual uint16_t		HorizontalRes(void) const
								{return(132);}
};
#endif // LCD_ST7567_h
