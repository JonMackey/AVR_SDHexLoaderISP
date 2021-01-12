/*
*	XFontRH1BitDataStream.h, Copyright Jonathan Mackey 2020
*	Class that handles shifting and breaking packed rotated 1 bit data for
*	monochrome displays.
*
*	The difference between this class and the other rotated 1 bit class is that
*	this class handles rotated and packed data stored as horizontal strips and
*	the other handles vertical strips.
*
*		Horizontal: 123		Vertical:	147
*					456					258
*					789					369
*
*	This was written for display controllers that don't support vertical/page
*	addressing, such as the ST7567.  You can simulate vertical addressing in
*	software but for the ST7567 it requires 3 command bytes sent for each data
*	byte written.  Because most display controllers support horizontal
*	addressing, this is the default unpacking class for rotated 1 bit.
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
#ifndef XFontRH1BitDataStream_h
#define XFontRH1BitDataStream_h

#include "DataStream.h"
class XFont;

class XFontRH1BitDataStream : public DataStream
{
public:
							XFontRH1BitDataStream(
								XFont*					inXFont,
								DataStream*				inSourceStream);

	virtual uint32_t		Read(
								uint32_t				inLength,
								void*					outBuffer);
	virtual uint32_t		Write(
								uint32_t				inLength,
								const void*				inBuffer)
								{return(0);}
	virtual bool			Seek(
								int32_t					inOffset,
								EOrigin					inOrigin);
	virtual uint32_t		GetPos(void) const;
	virtual bool			AtEOF(void) const;
	virtual uint32_t		Clip(
								uint32_t				inLength) const;
protected:
	bool		mReadGlyphHeader;
	XFont*		mXFont;
	DataStream*	mSourceStream;
	
	// State data
	uint8_t		mBitsInByteIn;
	uint8_t		mByteIn;
	uint8_t		mBitsInRowColumn;
	uint8_t		mColumnsLeftInRow;

	uint8_t		mBuffer[32];
	uint8_t		mBufferIndex;
	uint8_t		mBytesInBuffer;
	
	uint8_t					NextByte(void);
};
#endif // XFontRH1BitDataStream_h
