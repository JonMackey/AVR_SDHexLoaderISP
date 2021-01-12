/*
*	LCD_PCD8544.h, Copyright Jonathan Mackey 2019
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
#ifndef LCD_PCD8544_h
#define LCD_PCD8544_h
#include <SPI.h>
#include "DisplayController.h"

class DataStream;

class LCD_PCD8544 : public DisplayController
{
public:
							LCD_PCD8544(
								uint8_t					inDCPin,
								int8_t					inResetPin,	
								int8_t					inCSPin);

	virtual uint8_t			BitsPerPixel(void) const
								{return(1);}
	void					begin(
								uint8_t					inContrast = 63,
								uint8_t					inBias = 3);
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
protected:
	enum ECmds
	{
		// Read commands are not included because the MISO pin isn't generally available.
		eFunctionSetCmd		= 0x20,	// Function set
			eExtendedInst	= 0x01,	// Instruction 1 = extended, 0 = basic
			eVAddressingMode = 0x02,// Addressing mode 2 = vertical, 0 = horizontal
			ePowerDownChip	= 0x04, // 0 = chip is active, 4 = power down/sleep
									// Note: must clear screen before entering sleep
		// Basic instructions (H = 0)
		eDisplayContCmd		= 0x08,	// Display control
			eDisplayBlank	= 0,
			eNormalMode		= 0x04,	// Normal mode
			eAllSegOn		= 0x01,	// All segments on
			eInvert			= 0x05,	// Invert mode
		eSetYAddrCmd		= 0x40,	// Set y address, lower 3 bits = bank 0 to 5
		eSetXAddrCmd		= 0x80,	// Set x address, lower 7 bits = x
		// Extended instructions (H = 1)
		eBiasSystemCmd		= 0x10,	// Bias system
			eBiasMask		= 0x07,	// Bits BS0 to 2 (0 to 7)
		eTempContCmd		= 0x04,	// Temperature control
			eTempCoef0		= 0,	// Coefficient 0
			eTempCoef1		= 1,	// Coefficient 1
			eTempCoef2		= 2,	// Coefficient 2
			eTempCoef3		= 3,	// Coefficient 3
		eSetVOPCmd			= 0x80,	// VOP or contrast
			eVOPMask		= 0x3F	// Bits Vop0 to 6 (0 to 63)
	};
	int8_t		mCSPin;
	uint8_t		mDCPin;
	uint8_t		mResetPin;
	uint8_t		mChipSelBitMask;
	uint8_t		mDCBitMask;
	uint8_t		mStartColumn;	// For windowing (column range)
	uint8_t		mEndColumn;		// For windowing (column range)
	uint8_t		mStartRow;		// For windowing (row range)
	uint8_t		mEndRow;		// For windowing (row range)
	uint8_t		mDataRow;		// For windowing (IncCoords & FillPixels)
	uint8_t		mDataColumn;	// For windowing (IncCoords & FillPixels)
	volatile uint8_t*	mChipSelPortReg;
	volatile uint8_t*	mDCPortReg;
	SPISettings	mSPISettings;


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

	inline void				WriteCmd(
								uint8_t					inCmd) const;
	void					WriteData(
								const uint8_t*			inData,
								uint16_t				inDataLen);
	void					IncCoords(void);
								
};
#endif // LCD_PCD8544_h
