//
/*******************************************************************************
	License
	****************************************************************************
	This program is free software; you can redistribute it
	and/or modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation; either version 3 of the License, or
	(at your option) any later version.
 
	This program is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the
	implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE. See the GNU General Public
	License for more details.
 
	Licence can be viewed at
	http://www.gnu.org/licenses/gpl-3.0.txt

	Please maintain this license information along with authorship
	and copyright notices in any redistribution of this code
*******************************************************************************/
/*
*	XFontGlyph
*	
*	Created by Jon Mackey on 4/2/19.
*	Copyright © 2019 Jon. All rights reserved.
*/


#ifndef XFontGlyph_h
#define XFontGlyph_h
/*
*	The FreeType library uses 1/64 of a point for metrics values (advance,
*	ascent, etc..)  Because xfnt data is used for very low res displays, all
*	font metrics are in even pixels (points).  Doing this saves memory.
*/
struct FontHeader
{
	// A bit field can be used here because both the Arduino IDE and Xcode
	// use the GCC compiler.  GCC stores/formats the data exactly as shown.
	uint8_t		version : 4,	// structs version, currently 1
				oneBit : 1,		// One bit per pixel, else 8 bit (antialiased)
				rotated : 1,	// each data byte represents 8 pixels of a column (applies to 1 bit only)
				horizontal : 1,	// addressing for rotated data, else vertical (applies to 1 bit only)
				monospaced : 1;	// fixed width font (for this subset)
	int8_t 		ascent;			// font in pixels
	int8_t 		descent;		// font in pixels
	uint8_t 	height;			// font height (ascent+descent+leading) in pixels
	uint8_t		width;			// widest glyph within subset in pixels
	uint16_t	numCharcodeRuns;// size of the CharcodeRuns array
	uint16_t	numCharCodes;	// size of the GlyphDataOffsets array.
};

/*
*	CharcodeRuns is an array of CharcodeRun of consecutive charcodes.  This makes
*	it somewhat efficient in locating glyph data.  The runs are sorted
*	lowest to highest so a simple algorithm can be used to quickly locate
*	a charcode/glyph.
*	algorithm:
*	  -	find the start charcode that is lessthan or equal to the desired
*		charcode.
*	  - the data offset index = entryIndex + charcode - start
*
*	Sanity check...
*	If the calculated entry index is less than the next run's entry index
*	then the calculated entry index is valid (else use index 0)
*	An invalid charcode does not have a corresponding glyph.  There is always
*	an unused last run to use for this sanity check.
*/
struct CharcodeRun
{
	uint16_t	start;		// First charcode in this run
	uint16_t	entryIndex;	// Base index to the data offsets in this run
};

/*
*	GlyphDataOffsets is an array of uint16_t, one offset per glyph.
*	The actual length is numCharCodes +1.  The +1 accounts for the extra offset
*	used to calculate the size of the last glyph.
*
*	The first Glyph in a xfnt file is immediately after the last glyph offset.
*	GlyphDataOffsets are relative to the start of the glyph data (first glyph.)
*/

/*
*	The glyphs immediately follow the GlyphDataOffsets array.
*/
struct GlyphHeader
{
							GlyphHeader(void)
							: rows(0), columns(0){}
	uint8_t	advanceX;	// distance to advance the pen in the X direction in points
	int8_t x;			// distance to the first pixel
	int8_t y;			// distance to the first pixel
	uint8_t rows;
	uint8_t columns;
};

/*
*	The data is either 1 bit per pixel or run length encoded 8 bit.
*
*	Run length encoded 8 bit data: For each run the data starts with a signed
*	length byte.  If the length byte is positive the next byte should be
*	repeated length times.  If the length byte is negative, -length bytes should
*	be copied.  This optimizes the case where there are runs of unique pixel
*	values.
*
*	Note that the encoded data runs do not break at the end of each row.  The
*	data is assumed to be written to a defined window within the device RAM so
*	there is no need to break runs at the end of each row.
*
*	Example: CCCCABC would be encoded as 4C, -3ABC.
*	As an optimization, a positive run, such as 4C above, must have a minimum
*	length of 3 otherwise they're treated as unique pixels.
*
*	1 bit per pixel:  Each bit is a pixel as scanned horizontally, where the
*	most significant bit is visually on the left.  The data is stored packed.
*	The bits in a data byte may extend into the next row.
*
*	Example: Unrotated packed 5 pixel wide data with 3 rows is stored as
*			aaaaabbb bbcccccx  This saves quite a bit of space in some cases.
*			The same packing takes place for rotated, it's just columns rather
*			than rows.
*
*	Meaning of the FontHeader.vertical flag:
*	The vertical flag controls whether the rotated and packed data is stored
*	as horizontal or vertical strips.	Horizontal: 123		Vertical:	147
*													456					258
*													789					369
*
*	Horizontal is needed because some display controllers don't support
*	vertical addressing.  These controllers require multile bytes sent for each
*	data byte written in order to implement vertical addressing in software.
*	(e.g. ST7567 controller)
*/
#endif /* XFontGlyph_h */
