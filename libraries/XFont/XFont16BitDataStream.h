/*
*	XFont16BitDataStream.h, Copyright Jonathan Mackey 2019
*	Class that handles expanding 1 bit and 8 bit packed data to 16 bit 565 colors
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
#ifndef XFont16BitDataStream_h
#define XFont16BitDataStream_h

#include "DataStream.h"
class XFont;

class XFont16BitDataStream : public DataStream
{
public:
							XFont16BitDataStream(
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
	bool		mOneBit;
	bool		mReadGlyphHeader;
	XFont*		mXFont;
	DataStream*	mSourceStream;
	
	// State data
	union
	{
		struct
		{
			uint8_t		bitsInByteIn;
			uint8_t		byteIn;
		} oneBit;
		struct
		{
			uint16_t	color;
			int8_t		length;
		} run;
	} mSavedState;

	uint8_t		mBuffer[32];
	uint8_t		mBufferIndex;
	uint8_t		mBytesInBuffer;
	
	uint8_t					NextByte(void);
};
#endif // XFont16BitDataStream_h
