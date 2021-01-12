/*
*	UnixTimeEditor.cpp, Copyright Jonathan Mackey 2020
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
#include "UnixTimeEditor.h"
#include "DisplayController.h"
#include "XFont.h"
#include <avr/pgmspace.h>

const char kSetStr[] PROGMEM = "SET";
const char kCancelStr[] PROGMEM = "CANCEL";
const char kFormatStr[] PROGMEM = "FORMAT";
const char k12HStr[] PROGMEM = "12H";
const char k24HStr[] PROGMEM = "24H";
const char kAMStr[] PROGMEM = "AM";		// 51px
// Space is added on either side of PM when drawing over the wider AM string.
const char kPMStr[] PROGMEM = " PM ";	// 48px 


#define FONT_HEIGHT	43
#define DISPLAY_WIDTH	240
const Rect8_t UnixTimeEditor::kField[10] PROGMEM =
{
	{1,		0, 88, FONT_HEIGHT},	// eYearField
	{89,	0, 86, FONT_HEIGHT},	// eMonthField
	{175,	0, 52, FONT_HEIGHT},	// eDayField
	{1,		0, 75, FONT_HEIGHT},	// eFormatField
	{1,		0, 52, FONT_HEIGHT},	// eHourField
	{60,	0, 52, FONT_HEIGHT},	// eMinuteField
	{119,	0, 52, FONT_HEIGHT},	// eSecondField
	{171,	0, 67, FONT_HEIGHT},	// eAMPMField
	{16,	0, 62, FONT_HEIGHT},	// eSet
	{93,	0, 131, FONT_HEIGHT}	// eCancel
};
/*		
		.0000..MAY..00.		226px
		.0000.	 			88px
			  .MAY.			86px
				   .00.		52px
		FORMAT.12H.
		FORMAT				131px
			  .24H.			75px
		.00.:.00.:.00..PM.	237px
		.00.			  	52px
		.00.:				59px
		.00.:.00.:			118px
		.00.:.00.:.00.		170px
					  .AM.	67px
		.SET..CANCEL.		16 62 15 131 16 = 240
		
*/

/******************************* UnixTimeEditor *******************************/
UnixTimeEditor::UnixTimeEditor(void)
{
}

/********************************* Initialize *********************************/
void UnixTimeEditor::Initialize(
	XFont*	inXFont,
	bool	inDrawTime)
{
	mXFont = inXFont;
	mDisplay = inXFont->GetDisplay();
	mTop = inDrawTime ? FONT_HEIGHT*3 : 0;
	mDirtyField = 0;
	mDrawingTime = inDrawTime;
}

/*************************** LeftRightButtonPressed ***************************/
/*
*	Moves to the next or previous field.
*	This may cause the entire row/group to redraw.
*/
void UnixTimeEditor::LeftRightButtonPressed(
	bool	inIncrement)
{
	mDisplay->DrawFrame8(&mSelectionRect, XFont::eBlack, 2);
	uint8_t	visibleGroup = mVisibleGroup;
	if (inIncrement)
	{
		mSelection++;
		if (mSelection >= eNumFields)
		{
			mSelection = 0;
		/*
		*	Else if 24 hour format AND
		*	trying to move to AM/PM field THEN
		*	move to Cancel instead.
		*/
		} else if (mFormat24Hour &&
			mSelection == eAMPMField)
		{
			mSelection = eSet;
		}
	} else if (mSelection > 0)
	{
		mSelection--;
		/*
		*	If 24 hour format AND
		*	trying to move to AM/PM field THEN
		*	move to the seconds field instead.
		*/
		if (mFormat24Hour &&
			mSelection == eAMPMField)
		{
			mSelection = eSecondField;
		}
	} else
	{
		mSelection = eNumFields-1;
	}
	
	// Update the visible group
	if (mSelection < eFormatField)
	{
		visibleGroup = eDateGroup;
	} else if (mSelection == eFormatField)
	{
		visibleGroup = eFormatGroup;
	} else if (mSelection < eSet)
	{
		visibleGroup = eTimeGroup;
	} else
	{
		visibleGroup = eSetCancelGroup;
	}
	/*
	*	If the visible group changed THEN
	*	draw the entire group.
	*/
	if (visibleGroup != mVisibleGroup)
	{
		mVisibleGroup = visibleGroup;
		DrawVisibleGroup();
	}
	GetAdjustedFieldRect(mSelection, mSelectionRect);

}

/**************************** UpDownButtonPressed *****************************/
void UnixTimeEditor::UpDownButtonPressed(
	bool	inIncrement)
{
	switch (mSelection)
	{
		case eYearField:
			// The DS3231 is a 100 year clock.
			// For this implementation it's 2000 to 2099
			if (inIncrement)
			{
				mDSTime.dt.year++;
				if (mDSTime.dt.year > 99)
				{
					mDSTime.dt.year = 0;
				}
			} else
			{
				mDSTime.dt.year--;
				// If the number wrapped around (now 0xFF) THEN
				// reset to 99
				if (mDSTime.dt.year > 99)
				{
					mDSTime.dt.year = 99;
				}
			}
			if (mDSTime.dt.date > 28)
			{
				uint8_t	daysInMonth = DaysInMonthForYear(mDSTime.dt.month, mDSTime.dt.year);
				if (mDSTime.dt.date > daysInMonth)
				{
					mDSTime.dt.date = daysInMonth;
					mDirtyField |= _BV(eDayField);
				}
			}
			mDirtyField |= _BV(eYearField);
			break;
		case eMonthField:
			if (inIncrement)
			{
				mDSTime.dt.month++;
				if (mDSTime.dt.month > 12)
				{
					mDSTime.dt.month = 1;
				}
			} else if (mDSTime.dt.month > 1)
			{
				mDSTime.dt.month--;
			} else
			{
				mDSTime.dt.month = 12;
			}
			if (mDSTime.dt.date > 28)
			{
				uint8_t	daysInMonth = DaysInMonthForYear(mDSTime.dt.month, mDSTime.dt.year);
				if (mDSTime.dt.date > daysInMonth)
				{
					mDSTime.dt.date = daysInMonth;
					mDirtyField |= _BV(eDayField);
				}
			}
			mDirtyField |= _BV(eMonthField);
			break;
		case eDayField:
			if (inIncrement)
			{
				mDSTime.dt.date++;
				if (mDSTime.dt.date > 28 &&
					mDSTime.dt.date > DaysInMonthForYear(mDSTime.dt.month, mDSTime.dt.year))
				{
					mDSTime.dt.date = 1;
				}
			} else if (mDSTime.dt.date == 1)
			{
				mDSTime.dt.date = DaysInMonthForYear(mDSTime.dt.month, mDSTime.dt.year);
			} else
			{
				mDSTime.dt.date--;
			}
			mDirtyField |= _BV(eDayField);
			break;
		case eFormatField:
			mFormat24Hour = !mFormat24Hour;
			mDirtyField |= _BV(eFormatField);
			//mDirtyField |= (_BV(eFormatField)+_BV(eAMPMField)+_BV(eHourField));
			break;
		case eHourField:
		{
			bool	isPM = mDSTime.dt.hour >= 12;
			if (inIncrement)
			{
				mDSTime.dt.hour++;
				if (mDSTime.dt.hour > 23)
				{
					mDSTime.dt.hour = 0;
				}
			} else if (mDSTime.dt.hour > 0)
			{
				mDSTime.dt.hour--;
			} else
			{
				mDSTime.dt.hour = 23;
			}
			mDirtyField |= _BV(eHourField);
			if (!mFormat24Hour &&
				isPM != (mDSTime.dt.hour >= 12))
			{
				mDirtyField |= _BV(eAMPMField);
			}
			break;
		}
		case eMinuteField:
			if (inIncrement)
			{
				mDSTime.dt.minute++;
				if (mDSTime.dt.minute > 59)
				{
					mDSTime.dt.minute = 0;
				}
			} else if (mDSTime.dt.minute > 0)
			{
				mDSTime.dt.minute--;
			} else
			{
				mDSTime.dt.minute = 59;
			}
			mDirtyField |= _BV(eMinuteField);
			break;
		case eSecondField:
			if (inIncrement)
			{
				mDSTime.dt.second++;
				if (mDSTime.dt.second > 59)
				{
					mDSTime.dt.second = 0;
				}
			} else if (mDSTime.dt.second > 0)
			{
				mDSTime.dt.second--;
			} else
			{
				mDSTime.dt.second = 59;
			}
			mDirtyField |= _BV(eSecondField);
			break;
		case eAMPMField:
			if (mDSTime.dt.hour >= 12)
			{
				mDSTime.dt.hour -= 12;
			} else
			{
				mDSTime.dt.hour += 12;
			}
			mDirtyField |= _BV(eAMPMField);
			break;
		default:
			return;
	}
	DrawTime();
}

/******************************** EnterPressed ********************************/
bool UnixTimeEditor::EnterPressed(void)
{
	bool done = mVisibleGroup == eSetCancelGroup;
	if (!done)
	{
		LeftRightButtonPressed(true);
	}
	return(done);
}


/**************************** GetAdjustedFieldRect ****************************/
void UnixTimeEditor::GetAdjustedFieldRect(
	uint8_t		inFieldIndex,
	Rect8_t&	outRect)
{
	memcpy_P(&outRect, &kField[inFieldIndex], 4);
	outRect.y += mTop;
}

/********************************** DrawTime **********************************/
void UnixTimeEditor::DrawTime(void)
{
	if (mDrawingTime)
	{
		uint32_t	unixTime = DSDateTimeToUnixTime(mDSTime);
		char dataTimeStr[32];
		CreateDateStr(unixTime, dataTimeStr);
		mDisplay->MoveToRow(0);
		mXFont->SetTextColor(XFont::eCyan);
		mXFont->DrawCentered(dataTimeStr);
		bool isPM = CreateTimeStr(unixTime, dataTimeStr);
		mDisplay->MoveTo(FONT_HEIGHT, 31);
		mXFont->DrawStr(dataTimeStr);
		if (!mFormat24Hour)
		{
			mXFont->DrawStr(isPM ? " PM":" AM", true);
		} else
		{
			mXFont->EraseTillEndOfLine();
		}
	}
}

/****************************** DrawVisibleGroup ******************************/
void UnixTimeEditor::DrawVisibleGroup(void)
{
	static const uint8_t kGroupFields[] = {0,2, 3,3, 4,7, 8,9};
	uint8_t field, endField;
	Rect8_t	fieldRect;
	{
		uint8_t grpInx = mVisibleGroup << 1;
		field = kGroupFields[grpInx++];
		endField = kGroupFields[grpInx];
	}

	mDisplay->MoveTo(mTop, 0);
	mXFont->EraseTillEndOfLine();
	for (; field <= endField; field++)
	{
		GetAdjustedFieldRect(field, fieldRect);
		DrawField(field, fieldRect);
		if (field == eHourField ||
			field == eMinuteField)
		{
			// Draw the colon between the time fields
			mXFont->SetTextColor(XFont::eWhite);
			mDisplay->MoveToColumn(fieldRect.x+fieldRect.width);
			mXFont->DrawStr(":");
		} else if (field == eFormatField)
		{
			char formatStr[10];
			mDisplay->MoveColumnBy(18);
			strcpy_P(formatStr, kFormatStr);
			mXFont->SetTextColor(XFont::eGray);
			mXFont->DrawStr(formatStr);
		}
	}
}

/********************************** SetTime ***********************************/
void UnixTimeEditor::SetTime(
	time32_t	inTime)
{
	Rect8_t	fieldRect;
	mDisplay->Fill();	// Erase the display
	UnixTimeToDSDateTime(inTime, mDSTime);
	mDSTime.dt.second = 0;
	mFormat24Hour = sFormat24Hour;

	mVisibleGroup = eDateGroup;
	mSelection = 0;
	mSelectionIndex = 0;
	GetAdjustedFieldRect(0, mSelectionRect);
	mSelectionPeriod.Set(500);
	mSelectionPeriod.Start();
	
	DrawTime();
	DrawVisibleGroup();
}

/********************************** GetTime ***********************************/
time32_t UnixTimeEditor::GetTime(
	bool&	outFormat24Hour) const
{
	outFormat24Hour = mFormat24Hour;
	return(DSDateTimeToUnixTime(mDSTime));
}

/********************************* DrawField **********************************/
void UnixTimeEditor::DrawField(
	uint8_t		inField,
	Rect8_t&	inFieldRect)
{
	char fieldStr[10];
	mDisplay->MoveToRow(inFieldRect.y+5);
	mXFont->SetTextColor(XFont::eMagenta);
	switch (inField)
	{
		case eYearField:
			Uint16ToDecStr(mDSTime.dt.year+2000, fieldStr);
			fieldStr[4] = 0;
			break;
		case eMonthField:
			memcpy_P(fieldStr, &kMonth3LetterAbbr[(mDSTime.dt.month-1)*3], 3);
			fieldStr[3] = 0;
			// Erase before draw
			mDisplay->MoveToColumn(inFieldRect.x+8);
			mDisplay->FillBlock(mXFont->FontRows(), 70, XFont::eBlack);
			break;
		case eDayField:
			DecStrValue(mDSTime.dt.date, fieldStr);
			fieldStr[2] = 0;
			break;
		case eFormatField:
			strcpy_P(fieldStr, mFormat24Hour ? k24HStr : k12HStr);
			break;
		case eHourField:
		{
			uint8_t	hour = mDSTime.dt.hour;
			if (!mFormat24Hour &&
				//mDSTime.dt.year > kOneYear &&	// not elapsed time
				hour > 12)
			{
				hour -= 12;
			}
			DecStrValue(hour, fieldStr);
			fieldStr[2] = 0;
			break;
		}
		case eMinuteField:
			DecStrValue(mDSTime.dt.minute, fieldStr);
			fieldStr[2] = 0;
			break;
		case eSecondField:
			DecStrValue(mDSTime.dt.second, fieldStr);
			fieldStr[2] = 0;
			break;
		case eAMPMField:
			if (mFormat24Hour)
			{
				mDisplay->FillRect8(&inFieldRect, XFont::eBlack);
				return;
			} else
			{
				strcpy_P(fieldStr, mDSTime.dt.hour >= 12 ? kPMStr : kAMStr);
			}
			break;
		case eSet:
			mXFont->SetTextColor(XFont::eGreen);
			strcpy_P(fieldStr, kSetStr);
			break;
		case eCancel:
			mXFont->SetTextColor(XFont::eRed);
			strcpy_P(fieldStr, kCancelStr);
			break;
	}
	mXFont->DrawCentered(fieldStr, inFieldRect.x, inFieldRect.x+inFieldRect.width);
	
}

/*********************************** Update ***********************************/
void UnixTimeEditor::Update(void)
{
	if (mDirtyField)	// This assumes 8 fields
	{
		uint8_t	field = 0;
		Rect8_t	fieldRect;
		for (uint8_t mask = 1; mask; mask <<= 1, field++)
		{
			if ((mDirtyField & mask) == 0)
			{
				continue;
			}
			GetAdjustedFieldRect(field, fieldRect);
			DrawField(field, fieldRect);
		}
		mDirtyField = 0;
	}
	if (mSelectionPeriod.Passed())
	{
		mSelectionIndex++;
		uint16_t	selectionColor = (mSelectionIndex & 1) ? XFont::eWhite : XFont::eBlack;
		mDisplay->DrawFrame8(&mSelectionRect, selectionColor, 2);
		mSelectionPeriod.Start();
	}
}
