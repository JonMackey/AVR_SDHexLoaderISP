/*
*	DisplayController.cpp, Copyright Jonathan Mackey 2019
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
#include "DisplayController.h"
#include "DataStream.h"
#ifndef __MACH__
#include <Arduino.h>
#endif


/******************************** DisplayController ********************************/
DisplayController::DisplayController(
	uint16_t	inRows,
	uint16_t	inColumns)
	: mRows(inRows), mColumns(inColumns), mRow(0), mColumn(0),
	  mAddressingMode(eHorizontal)
{
}

/********************************* CanMoveTo **********************************/
bool DisplayController::CanMoveTo(
	uint16_t	inRow,
	uint16_t	inColumn) const
{
	return(inRow < mRows && inColumn < mColumns);
}

/*********************************** MoveBy ***********************************/
/*
*	Returns true if the relative move was within the display bounds.
*/
bool DisplayController::MoveBy(
	uint16_t	inRows,
	uint16_t	inColumns)
{
	inRows += mRow;
	inColumns += mColumn;
	bool success = CanMoveTo(inRows, inColumns);
	if (success)
	{
		MoveTo(inRows, inColumns);
	}
	return(success);
}

/******************************** MoveColumnBy ********************************/
// Resets to zero on wrap.  Does not affect the row (page)
void DisplayController::MoveColumnBy(
	uint16_t	inMoveBy)
{
	uint16_t	newColumn = mColumn + inMoveBy;
	if (newColumn >= mColumns)
	{
		newColumn = 0;
	}
	MoveToColumn(newColumn);
}

/********************************* MoveRowBy **********************************/
// Resets to zero on wrap.  Does not affect the column
void DisplayController::MoveRowBy(
	uint16_t	inMoveBy)
{
	uint16_t	newRow = mRow + inMoveBy;
	if (newRow >= mRows)
	{
		newRow = 0;
	}
	MoveToRow(newRow);
}

/********************************** WillFit ***********************************/
bool DisplayController::WillFit(
	uint16_t	inRows,
	uint16_t	inColumns)
{
	return(	(mRow + inRows) <= mRows &&
			(mColumn + inColumns) <= mColumns);
}

/************************************ Fill************************************/
void DisplayController::Fill(
	uint16_t	inFillColor)
{
	/*
	*	Note that the order of the calls matter.  MoveTo should be called before
	*	SetColumnRange because on the TFT displays SetColumnRange sends the
	*	command to setup writing mode.  If MoveTo were to be called after, 
	*	writing mode command is terminated before the fill starts.
	*	The order doesn't matter on the OLED display.
	*/
	MoveTo(0, 0);	// After clear the page and column will wrap to zero (no need to call MoveTo again at end.)
	SetColumnRange(0, mColumns-1);	// Reset the range in case it's been clipped.
	FillPixels(mRows*mColumns, inFillColor);
}

/********************************* FillBlock **********************************/
void DisplayController::FillBlock(
	uint16_t	inRows,
	uint16_t	inColumns,
	uint16_t	inFillColor)
{
	if ((inColumns+mColumn) >= mColumns)
	{
		inColumns = mColumns - mColumn;
	}
	if ((inRows+mRow) >= mRows)
	{
		inRows = mRows - mRow;
	}
	if (inColumns && inRows)
	{
		SetColumnRange(inColumns);
		// The column index will wrap back to the starting point.
		// The page won't so it needs to be reset.
		FillPixels(inRows * inColumns, inFillColor);
		SetColumnRange(0, mColumns-1);	// Remove the column range clipping
		MoveToRow(mRow);	// Leave the page unchanged
		MoveColumnBy(inColumns); // Advance by inColumns (or wrap to zero if at or past end)
	}
}

/********************************** FillRect **********************************/
void DisplayController::FillRect(
	uint16_t	inX,
	uint16_t	inY,
	uint16_t	inWidth,
	uint16_t	inHeight,
	uint16_t	inFillColor)
{
	MoveTo(inY, inX);
	FillBlock(inHeight, inWidth, inFillColor);
}

/********************************* FillRect8 **********************************/
void DisplayController::FillRect8(
	const Rect8_t*	inRect,
	uint16_t		inFillColor)
{
	FillRect(inRect->x, inRect->y, inRect->width, inRect->height, inFillColor);
}

/********************************* DrawFrame **********************************/
void DisplayController::DrawFrame(
	uint16_t	inX,
	uint16_t	inY,
	uint16_t	inWidth,
	uint16_t	inHeight,
	uint16_t	inColor,
	uint8_t		inThickness)
{
	MoveTo(inY, inX);
	SetColumnRange(inWidth);
	FillPixels(inWidth * inThickness, inColor);
	MoveToRow(inY+inHeight-inThickness);
	SetColumnRange(inWidth);
	FillPixels(inWidth * inThickness, inColor);
	MoveToRow(inY+inThickness);
	SetColumnRange(inThickness);
	FillPixels((inHeight-(inThickness*2)) * inThickness, inColor);
	MoveToColumn(inX + inWidth - inThickness);
	SetColumnRange(inThickness);
	FillPixels((inHeight-(inThickness*2)) * inThickness, inColor);
}

/********************************* DrawFrame8 *********************************/
void DisplayController::DrawFrame8(
	const Rect8_t*	inRect,
	uint16_t		inColor,
	uint8_t			inThickness)
{
	DrawFrame(inRect->x, inRect->y, inRect->width, inRect->height, inColor, inThickness);
}

/********************************* DrawFrameP *********************************/
/*void DisplayController::DrawFrameP(
	const Rect8_t*	inRect,
	uint16_t		inColor,
	uint8_t			inThickness)
{
	Rect8_t	rect8;
	memcpy_P(&rect8, inRect, sizeof(Rect8_t));
	DrawFrame(&rect8, inColor, inThickness);
}*/

/******************************* SetColumnRange *******************************/
void DisplayController::SetColumnRange(
	uint16_t	inRelativeWidth)
{
	SetColumnRange(mColumn, mColumn + inRelativeWidth-1);
}

/******************************* SetRowRange *******************************/
void DisplayController::SetRowRange(
	uint16_t	inRelativeHeight)
{
	SetRowRange(mRow, mRow + inRelativeHeight-1);
}

/***************************** FillTillEndColumn ******************************/
void DisplayController::FillTillEndColumn(
	uint16_t	inRows,
	uint16_t	inFillColor)
{
	FillBlock(inRows, mColumns, inFillColor);
}

/****************************** StreamCopyBlock *******************************/
bool DisplayController::StreamCopyBlock(
	DataStream*	inDataStream,
	uint16_t		inRows,
	uint16_t		inColumns)
{
	bool	success = WillFit(inRows, inColumns);
	if (success)
	{
		uint16_t	pixelsToCopy = inRows * inColumns;
		if (pixelsToCopy)
		{
			if (mAddressingMode == eHorizontal)
			{
				SetColumnRange(inColumns);
				// The column index will wrap back to the starting point.
				// The page won't so it needs to be reset.
				StreamCopy(inDataStream, pixelsToCopy);
				SetColumnRange(0, mColumns-1);	// Remove the column range clipping
				MoveToRow(mRow);	// Leave the page unchanged
				MoveColumnBy(inColumns); // Advance by inColumns (or wrap to zero if at or past end)
			} else
			{
				MoveToRow(mRow);	// Leave the page unchanged
				SetRowRange(inRows);
				// The row index will wrap back to the starting point.
				// The column won't so it needs to be reset.
				StreamCopy(inDataStream, pixelsToCopy);
				MoveToRow(mRow);	// Leave the page unchanged
				MoveColumnBy(inColumns); // Advance by inColumns (or wrap to zero if at or past end)
			}
		}
	}
	
	return(success);
}


