/*
*	DisplayController.h, Copyright Jonathan Mackey 2019
*	Base class for a display controller.
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
#ifndef DisplayController_h
#define DisplayController_h

#include <inttypes.h>
#include <string.h>
#ifndef __MACH__
#include <avr/pgmspace.h>
#else
#define memcpy_P memcpy
#endif

class DataStream;

typedef struct Rect8_t
{
	uint8_t	x;
	uint8_t	y;
	uint8_t	width;
	uint8_t	height;
	
	inline void				Copy(
								const Rect8_t&			inRect)
								{memcpy(this, &inRect, 4);}
	inline void				Copy_P(
								const Rect8_t&			inRect)
								{memcpy_P(this, &inRect, 4);}
} Rect8_t;


class DisplayController
{
public:
							DisplayController(
								uint16_t				inRows,
								uint16_t				inColumns);
	virtual uint8_t			BitsPerPixel(void) const
								{return(16);}
	bool					CanMoveTo(
								uint16_t				inRow,
								uint16_t				inColumn) const;
	virtual void			MoveTo(
								uint16_t				inRow,
								uint16_t				inColumn) = 0;
	bool					MoveBy(
								uint16_t				inRows,
								uint16_t				inColumns);
	virtual void			MoveToRow(
								uint16_t				inRow) = 0;
	virtual void			MoveToColumn(
								uint16_t				inColumn) = 0;
	void					MoveRowBy(
								uint16_t				inMoveBy);
	void					MoveColumnBy(
								uint16_t				inMoveBy);
	/*
	*	WillFit: Returns true if a block inRows x inColumns fit on the display
	*	relative to the current position.
	*/
	bool					WillFit(
								uint16_t				inRows,
								uint16_t				inColumns);
	uint16_t				GetRow(void) const
								{return(mRow);}
	uint16_t				GetColumn(void) const
								{return(mColumn);}
	uint16_t				GetRows(void) const
								{return(mRows);}
	uint16_t				GetColumns(void) const
								{return(mColumns);}
	/*
	*	Turns the display off.
	*/
	virtual void			Sleep(void) = 0;
	/*
	*	Turns the display on.
	*/
	virtual void			WakeUp(void) = 0;
	/*
	*	Fill: Fills the entire display by setting the pixel data to inFillColor.
	*	The current row and column will be set to zero.
	*/
	virtual void			Fill(
								uint16_t				inFillColor = 0);
	/*
	*	FillPixels: Sets a run of inPixelsToClear to inFillColor from the
	*	current position and column clipping.
	*/
	virtual void			FillPixels(
								uint16_t				inPixelsToFill,
								uint16_t				inFillColor) = 0;
	/*
	*	FillBlock: Fills a block defined by inRows x inColumns to inFillColor
	*	from the current position.  On exit the column will be advanced by
	*	inColumns, and the current row will remain unchanged.
	*/
	void					FillBlock(
								uint16_t				inRows,
								uint16_t				inColumns,
								uint16_t				inFillColor);
	
	void					FillRect(
								uint16_t				inX,
								uint16_t				inY,
								uint16_t				inWidth,
								uint16_t				inHeight,
								uint16_t				inFillColor);
	void					FillRect8(
								const Rect8_t*			inRect,
								uint16_t				inFillColor);
								
	/*
	*	DrawFrame: Draws an inThickness pixel frame at (x,y, w, h).  The frame
	*	is inset.
	*	NOTE: DrawFrame doesn't work on 1 bit displays.
	*/
	void					DrawFrame(
								uint16_t				inX,
								uint16_t				inY,
								uint16_t				inWidth,
								uint16_t				inHeight,
								uint16_t				inColor,
								uint8_t					inThickness = 1);
	void					DrawFrame8(
								const Rect8_t*			inRect,
								uint16_t				inColor,
								uint8_t					inThickness = 1);
//	void					DrawFrameP(
//								const Rect8_t*			inRect,
//								uint16_t				inColor,
//								uint8_t					inThickness = 1);
	/*
	*	SetColumnRange: Sets a the column clipping relative to the current
	*	column. (column : column + inRelativeWidth -1)
	*/
	void					SetColumnRange(
								uint16_t				inRelativeWidth);
	/*
	*	SetColumnRange: Sets a the absolute column range clipping to
	*	inStartColumn to inEndColumn.
	*/
	virtual void			SetColumnRange(
								uint16_t				inStartColumn,
								uint16_t				inEndColumn) = 0;
	/*
	*	SetRowRange: Sets a the row clipping relative to the current
	*	row. (row : row + inRelativeHeight -1)
	*/
	void					SetRowRange(
								uint16_t				inRelativeHeight);
								
	/*
	*	SetRowRange: Sets a the absolute row range clipping to
	*	inStartRow to inEndRow.
	*/
	virtual void			SetRowRange(
								uint16_t				inStartRow,
								uint16_t				inEndRow) = 0;

	/*
	*	FillTillEndColumn: will fill inRows till the end column relative to
	*	the current row.  The current row (page) is left unchanged, the current
	*	column is set to zero.
	*/
	void					FillTillEndColumn(
								uint16_t				inRows,
								uint16_t				inFillColor);
	/*
	*	StreamCopyBlock: Copies inRows*inColumns data Pixels from inDataStream
	*	starting at the current row and column.  Will fail if it won't
	*	fit (nothing drawn, just fails.)  If successful the current column
	*	is advanced by inColumns.  The current page is left unchanged.
	*/
	virtual bool			StreamCopyBlock(
								DataStream*				inDataStream,
								uint16_t				inRows,
								uint16_t				inColumns);
	/*
	*	StreamCopy: Blindly copies inPixelsToCopy data bytes from inDataStream
	*	starting at the current row and column.  No checking to see if the data
	*	will fit on the display without clipping, skewing or wrapping.  The
	*	current row and column will be undefined (not updated within the class.)
	*	For monochrome 8 bits per pixel devices, inPixelsToCopy is treated as
	*	in bytes to copy.
	*/
	virtual void			StreamCopy(
								DataStream*				inDataStream,
								uint16_t				inPixelsToCopy) = 0;
								
	virtual void			CopyPixels(
								const void*				inPixels,
								uint16_t				inPixelsToCopy){};
	enum EAddressingMode
	{
		eHorizontal,
		eVertical
	};
	virtual void			SetAddressingMode(
								EAddressingMode			inAddressingMode = eHorizontal) = 0;
protected:
	uint16_t	mRows;
	uint16_t	mColumns;
	uint16_t	mRow;
	uint16_t	mColumn;
	EAddressingMode	mAddressingMode;
};

#endif // DisplayController_h
