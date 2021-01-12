/*
*	UnixTimeEditor.h, Copyright Jonathan Mackey 2020
*	Edits and draws the time fields.  This is written for a 240 pixel wide
*	display with a font height of 43 pixels.
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
#ifndef UnixTimeEditor_h
#define UnixTimeEditor_h

#include "MSPeriod.h"
#include "UnixTime.h"
#include "DS3231SN.h"
#include "DisplayController.h"

class XFont;

class UnixTimeEditor : public UnixTime
{
public:
	enum EField
	{
		eYearField,
		eMonthField,
		eDayField,
		eFormatField,
		eHourField,
		eMinuteField,
		eSecondField,
		eAMPMField,
		eSet,
		eCancel,
		eNumFields
	};
	enum EGroup
	{
		eDateGroup,
		eFormatGroup,
		eTimeGroup,
		eSetCancelGroup,
		eNumGroups
	};
							UnixTimeEditor(void);
	void					Initialize(
								XFont*					inXFont,
								bool					inDrawTime = true);
	void					Update(void);
	void					SetTime(
								time32_t				inTime);
	time32_t				GetTime(
								bool&					outFormat24Hour) const;
	bool					CancelIsSelected(void) const
								{return(mSelection == eCancel);}
	bool					EnterPressed(void);
	void					UpDownButtonPressed(
								bool					inIncrement);
	void					LeftRightButtonPressed(
								bool					inIncrement);
protected:
	MSPeriod			mSelectionPeriod;
	XFont*				mXFont;
	DisplayController*	mDisplay;
	bool				mFormat24Hour;
	bool				mDrawingTime;
	uint8_t				mTop;
	uint8_t				mDirtyField;
	uint8_t				mVisibleGroup;
	uint8_t				mSelection;
	uint8_t				mSelectionIndex;
	Rect8_t				mSelectionRect;
	DSDateTime			mDSTime;
	static const Rect8_t kField[];
	
	void					DrawVisibleGroup(void);
	void					DrawTime(void);
	void					GetAdjustedFieldRect(
								uint8_t					inFieldIndex,
								Rect8_t&				outRect);
	void					DrawField(
								uint8_t					inField,
								Rect8_t&				inFieldRect);
};

#endif // UnixTimeEditor_h
