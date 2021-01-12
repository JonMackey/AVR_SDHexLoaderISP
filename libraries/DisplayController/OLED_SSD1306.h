/*
*	OLED_SSD1306.h, Copyright Jonathan Mackey 2019
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
#ifndef OLED_SSD1306_h
#define OLED_SSD1306_h

#include "DisplayController.h"

class DataStream;

class OLED_SSD1306 : public DisplayController
{
public:
							OLED_SSD1306(
								uint8_t					inI2CAddress,
								uint8_t					inHeight = 64,
								uint8_t					inWidth = 128);
	virtual uint8_t			BitsPerPixel(void) const
								{return(1);}
	void					begin(
								bool					inRotate180 = false);
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
	*	FillPixels: Sets a run of inBytesToFill to inFillColor from the
	*	current position and column clipping.  This device is monochrome, 8
	*	pixels per byte, so inFillColor is used as 0 or 0xFF for any non-zero
	*	value.
	*/
	virtual void			FillPixels(
								uint16_t				inBytesToFill,
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
	*	inStartPage to inEndPage.
	*/
	virtual void			SetRowRange(
								uint16_t				inStartPage,
								uint16_t				inEndPage);

	/*
	*	StreamCopy: Blindly copies inBytesToCopy data bytes from inDataStream
	*	starting at the current row and column.  No checking to see if the data
	*	will fit on the display without clipping, skewing or wrapping.  The
	*	current row and column will be undefined (not updated within the class.)
	*/
	virtual void			StreamCopy(
								DataStream*				inDataStream,
								uint16_t				inBytesToCopy);
								
	virtual void			SetAddressingMode(
								EAddressingMode			inAddressingMode);
protected:
	enum ECmds
	{
		eAddrModeCmd		= 0x20,	// Set Memory Addressing Mode
		eSetColAddrCmd		= 0x21,	// Column Address Set (column range)
		eSetPageAddrCmd		= 0x22,	// Page Address Set (page range)
		eDisplayOffCmd		= 0xAE,	// Set Display OFF
		eDisplayOnCmd		= 0xAF	// Set Display ON
	};
	uint8_t	mHeight;
	uint8_t	mI2CAddr;

	void					Init(
								bool					inRotate180);
	void					WriteCmd(
								uint8_t					inCmd);
	void					WriteBytes(
								const uint8_t*			inBytes,
								uint16_t				inLength);
};

#endif // OLED_SSD1306_h
