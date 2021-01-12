/*
*	XFont16BitDataStream.cpp, Copyright Jonathan Mackey 2019
*	Class that handles expanding unrotated 1 bit and 8 bit packed data to
*	16 bit 565 colors.
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
#include "XFont16BitDataStream.h"
#include "XFont.h"
#include <string.h>

/*************************** XFont16BitDataStream *****************************/
XFont16BitDataStream::XFont16BitDataStream(
	XFont*		inXFont,
	DataStream*	inSourceStream)
	: mXFont(inXFont), mSourceStream(inSourceStream)
{
}

/************************************ Seek ************************************/
bool XFont16BitDataStream::Seek(
	int32_t		inOffset,
	EOrigin		inOrigin)
{
	// Calling Seek flags the state data as waiting for the glyph header Read.
	mReadGlyphHeader = true;
	return(mSourceStream->Seek(inOffset, inOrigin));
}

/*********************************** AtEOF ************************************/
bool XFont16BitDataStream::AtEOF(void) const
{
	return(mSourceStream->AtEOF());
}

/*********************************** GetPos ***********************************/
uint32_t XFont16BitDataStream::GetPos(void) const
{
	return(mSourceStream->GetPos());
}

/************************************ Clip ************************************/
uint32_t XFont16BitDataStream::Clip(
	uint32_t	inLength) const
{
	return(mSourceStream->Clip(inLength));
}

/********************************** NextByte **********************************/
/*
*	This routine manages a small buffer rather than constantly calling Read of
*	the source stream.
*/
uint8_t XFont16BitDataStream::NextByte(void)
{
	if (mBufferIndex == mBytesInBuffer)
	{
		mBytesInBuffer = (uint8_t)mSourceStream->Read(sizeof(mBuffer), mBuffer);
		mBufferIndex = 0;
	}
	if (mBytesInBuffer)
	{
		return(mBuffer[mBufferIndex++]);
	}
	return(0);
}

/************************************ Read ************************************/
/*
*	Unpacks either 1 bit or 8 bit glyph data to 565 pixel data.
*	See XFontGlyph.h for packing details.
*/
uint32_t XFont16BitDataStream::Read(
	uint32_t	inLength,
	void*		outBuffer)
{
	if (mReadGlyphHeader)
	{
		mReadGlyphHeader = false;
		mBufferIndex = 0;
		mBytesInBuffer = 0;
		mSavedState.run = {0};
		return(mSourceStream->Read(inLength, outBuffer));
	}
	if (inLength)
	{
		uint16_t*	oBufferPtr = (uint16_t*)outBuffer;
		uint16_t*	oBufferEnd = &oBufferPtr[inLength];
		if (mXFont->GetFontHeader().oneBit)
		{
			uint8_t	byteIn;
			int8_t	bitsInByteIn = mSavedState.oneBit.bitsInByteIn;

			/*
			*	If not continuing from a paused unpack of an 8 bit byte THEN
			*	load a new byte.
			*/
			if (bitsInByteIn == 0)
			{
				byteIn = NextByte();
				bitsInByteIn = 8;
			/*
			*	Else continue from where the unpacking stopped.
			*/
			} else
			{
				byteIn = mSavedState.oneBit.byteIn;
			}
			do
			{
				for (; oBufferPtr != oBufferEnd && bitsInByteIn; byteIn <<= 1, bitsInByteIn--)
				{
					*(oBufferPtr++) = (byteIn & 0x80) ? mXFont->GetTextColor() : mXFont->GetBGTextColor();
				}
				/*
				*	If not at the end of the output buffer THEN
				*	load the next 8 bit byte to unpack.
				*/
				if (oBufferPtr != oBufferEnd)
				{
					byteIn = NextByte();
					bitsInByteIn = 8;
				/*
				*	Else save the unpack state and exit.
				*/
				} else
				{
					mSavedState.oneBit.bitsInByteIn = bitsInByteIn;
					mSavedState.oneBit.byteIn = byteIn;
					break;
				}
			} while (true);
		} else
		{
			int8_t runLength = mSavedState.run.length;
			uint16_t	runColor;
			if (runLength == 0)
			{
				runLength = NextByte();
				runColor = mXFont->Calc565Color(NextByte());
			} else
			{
				runColor = mSavedState.run.color;
			}
			do
			{
				/*
				*	If the run length is negative THEN
				*	this is a run of unique values.
				*/
				if (runLength < 0)
				{
					while (oBufferPtr != oBufferEnd)
					{
						*(oBufferPtr++) = runColor;
						runLength++;
						if (runLength)
						{
							runColor = mXFont->Calc565Color(NextByte());
							continue;
						}
						break;
					}
				/*
				*	Else this is a run of same values.
				*/
				} else if (runLength > 0)
				{
					for (; oBufferPtr != oBufferEnd && runLength; runLength--)
					{
						*(oBufferPtr++) = runColor;
					}
				} else
				{
					// Fatal error in source data.  A zero run length was read.
					oBufferPtr = oBufferEnd;
				}
				/*
				*	If not at the end of the output buffer THEN
				*	load the next run length and its color
				*/
				if (oBufferPtr != oBufferEnd)
				{
					runLength = NextByte();
					runColor = mXFont->Calc565Color(NextByte());
				/*
				*	else, save the state and exit.
				*/
				} else
				{
					mSavedState.run.length = runLength;
					mSavedState.run.color = runColor;
					break;
				}
			} while (true);
		}
	}
	return(inLength);
}

