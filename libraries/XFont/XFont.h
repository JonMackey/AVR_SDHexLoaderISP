/*
*	XFont.h, Copyright Jonathan Mackey 2019
*	Class for displaying subset bitmap fonts on select common displays.
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
#ifndef XFont_h
#define XFont_h

#include <inttypes.h>
#include "XFontGlyph.h"

class DataStream;
class DisplayController;

class XFont
{
public:
	struct Font
	{
		const FontHeader*	header;
		const CharcodeRun*	charcodeRuns;
		const uint16_t*		glyphDataOffsets;
		DataStream*			glyphData;
							Font(
								const FontHeader*	inHeader,
								const CharcodeRun*	inCharcodeRuns,
								const uint16_t*		inGlyphDataOffsets,
								DataStream*			inGlyphData = nullptr)
								: header(inHeader),
								  charcodeRuns(inCharcodeRuns),
								  glyphDataOffsets(inGlyphDataOffsets),
								  glyphData(inGlyphData){}
	};
							XFont(void);

	/*
	*	SetDisplay: Sets the target display.
	*/
	void					SetDisplay(
								DisplayController*		inDisplay,
								Font*					inFont = nullptr);
	DisplayController*		GetDisplay(void) const
								{return(mDisplay);}

	/*
	*	SetFont: called to set the initial font or to change fonts.
	*	SetFont should be called after setting the display.
	*/
	void					SetFont(
								Font*					inFont);
	Font*					GetFont(void) const
								{return(mFont);}
	
	/*
	*	Relative move by N text rows and the absolute pixel column.
	*/
	bool					AdvanceRow(
								uint16_t				inNumRows = 1,
								uint16_t				inColumn = 0) const;
	/*
	*	Absolute move to the Nth text row and the pixel column.
	*/
	bool					MoveTo(
								uint16_t				inTextRow,
								uint16_t				inColumn = 0) const;
	/*
	*	Draws the UTF-8 string at the current display x,y position, stopping
	*	on the first character that doesn't fit without being truncated.  At
	*	that point the string is scanned for a newline. If a newline is found
	*	and there is space to start a newline, the drawing continues on the new
	*	line.
	*
	*	Passing a inFakeMonospaceWidth value other than zero will attempt to
	*	draw the string simulating monospace.  If any of the glyphs are wider
	*	than the passed value, then the wider value is used otherwise the glyph
	*	is centered within inFakeMonospaceWidth and inFakeMonospaceWidth becomes
	*	the advanceX.  This is used mostly with number fields that change.
	*	WidestGlyph() can be used to calculate the fake monospace width for a
	*	range of character codes.
	*/
	void					DrawStr(
								const char*				inUTF8Str,
								bool					inClearTillEOL = false,
								uint8_t					inFakeMonospaceWidth = 0);
	/*
	*	DrawRightJustified draws the passed string from the right side of the
	*	display, ignoring the display's current x position.  It's assumed there
	*	is only a single line, no newlines in string.  If the passed string is
	*	wider than the display, the string is drawn left justified, truncated
	*	on the right.  This routine returns the width of the passed string.
	*	As an optimization, if inWidth is not zero it will be used to determine
	*	the starting column.
	*	When inRight is zero the display width is used as the right edge, else
	*	inRight is used.
	*/
	uint16_t				DrawRightJustified(
								const char*				inUTF8Str,
								uint16_t				inRight = 0,
								uint16_t				inWidth = 0);
	/*
	*	DrawCentered draws the passed string centered in the display, ignoring
	*	the display's current x position.  It's assumed there is only a single
	*	line, no newlines in string.  If the passed string is wider than the
	*	display, the string is drawn left justified, truncated on the right.
	*	This routine returns the width of the passed string. As an optimization,
	*	if inWidth is not zero it will be used to determine the starting column.
	*/
	uint16_t				DrawCentered(
								const char*				inUTF8Str,
								uint16_t				inLeft = 0,
								uint16_t				inRight = 0,
								uint16_t				inWidth = 0);
	void					EraseTillEndOfLine(void);
	void					EraseTillColumn(
								uint16_t				inColumn);
	uint8_t					FontRows(void) const
								{return(mFontRows);}
	void					DrawLoadedGlyph(void);
	uint16_t				FindGlyph(
								uint16_t				inCharcode);
	uint16_t				LoadGlyph(
								uint16_t				inCharcode);
	bool					LoadGlyphHeader(
								uint16_t				inEntryIndex);
	/*
	*	Returns the widest glyph in the passed range string.  The format of the
	*	range string is the same as you would use within SubsetFontCreator.
	*	Returns zero if any of the chars within inUTF8RangeStr don't exist or
	*	the format of inUTF8RangeStr is invalid.
	*/
	uint8_t					WidestGlyph(
								const char*				inUTF8RangeStr);
	/*
	*	inUTF8Str is the string to be measured
	*	outHeight is the overall height of all lines within inUTF8Str
	*	outWidth is the width of the widest line within inUTF8Str
	*	inFakeMonospaceWidth if not zero, is what will be used for the width of each glyph
	*	ioLineCount on entry is the max number of elements in outLineWidths
	*	ioLineCount on exit is the actual number of lines in inUTF8Str
	*	outLineWidths on exit will contain the width of each line.
	*/
	bool					MeasureStr(
								const char*				inUTF8Str,
								uint16_t&				outHeight,
								uint16_t&				outWidth,
								uint8_t					inFakeMonospaceWidth = 0,
								uint8_t*				ioLineCount = nullptr,
								uint16_t*				outLineWidths = nullptr);
	// Returns the last glyph loaded by LoadGlyph
	const GlyphHeader&		Glyph(void) const
								{return(mGlyph);}
	uint16_t				Charcode(void) const
								{return(mCharcode);}
	const FontHeader&		GetFontHeader(void) const
								{return(mFontHeader);}
	void					SetTextColor(
								uint16_t				inTextColor);
	uint16_t				GetTextColor(void) const
								{return(mTextColor);}
	void					SetBGTextColor(
								uint16_t				inBGTextColor);
	uint16_t				GetBGTextColor(void) const
								{return(mTextBGColor);}
	void					SetHighlightColors(
								uint16_t				inHighlightColor,
								uint16_t				inBGHighlightColor);
	void					EnableHighlighting(
								bool					inEnable = true);
	uint16_t				Calc565Color(
								uint8_t					inTint);
	static uint16_t			Calc565Color(
								uint16_t				inFG,
								uint16_t				inBG,
								uint8_t					inTint);
	static uint16_t			NextChar(
								const char*&			inUTF8Str);
	static bool 			SkipToNextLine(
								const char*&			inUTF8Str);
	enum E565Colors
	{
		eBlack		= 0,
		eRed		= 0x31DF,
		eGreen		= 0x4665,
		eBlue		= 0xFBC0,
		eCyan		= 0xFFE0,
		eMagenta	= 0xFA1F,
		eYellow		= 0x07DF,
		eBrown		= 0x43D5,
		ePurple		= 0x9112,
		eOrange		= 0x049F,
		eGray		= 0xCE79,
		eWhite		= 0xFFFF
	};
protected:
	FontHeader			mFontHeader;
	Font*				mFont;
	DisplayController*	mDisplay;
	uint16_t			mTextColor;
	uint16_t			mTextBGColor;
	uint16_t			mHighlightColor;
	uint16_t			mBGHighlightColor;
	GlyphHeader			mGlyph;
	uint8_t				mFontRows;
	uint16_t			mCharcode;
	bool				mHighlightEnabled;
};

#endif // XFont_h
