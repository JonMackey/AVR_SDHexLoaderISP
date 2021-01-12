/*
*	XFontRH1BitDataStream.cpp, Copyright Jonathan Mackey 2020
*	Class that handles shifting and breaking packed rotated 1 bit data for
*	monochrome displays.
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
#include "XFontRH1BitDataStream.h"
#include "XFont.h"
#include <string.h>

/**************************** XFontRH1BitDataStream *****************************/
XFontRH1BitDataStream::XFontRH1BitDataStream(
	XFont*		inXFont,
	DataStream*	inSourceStream)
	: mXFont(inXFont), mSourceStream(inSourceStream)
{
}

/************************************ Seek ************************************/
bool XFontRH1BitDataStream::Seek(
	int32_t		inOffset,
	EOrigin		inOrigin)
{
	// Calling Seek flags the state data as waiting for the glyph header Read.
	mReadGlyphHeader = true;
	return(mSourceStream->Seek(inOffset, inOrigin));
}

/*********************************** AtEOF ************************************/
bool XFontRH1BitDataStream::AtEOF(void) const
{
	return(mSourceStream->AtEOF());
}

/*********************************** GetPos ***********************************/
uint32_t XFontRH1BitDataStream::GetPos(void) const
{
	return(mSourceStream->GetPos());
}

/************************************ Clip ************************************/
uint32_t XFontRH1BitDataStream::Clip(
	uint32_t	inLength) const
{
	return(mSourceStream->Clip(inLength));
}

/********************************** NextByte **********************************/
/*
*	This routine manages a small buffer rather than constantly calling Read of
*	the source stream.
*/
uint8_t XFontRH1BitDataStream::NextByte(void)
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

#if 0
#ifdef __MACH__
#include <string>
#else
#include <Arduino.h>
#endif
void DumpChar(
	uint8_t	inChar)
{
	uint8_t	valueIn = inChar;
	uint8_t	nMask = 0x80;
	uint8_t	binaryStr[32];
	uint8_t*	buffPtr = binaryStr;
	for (; nMask != 0; nMask >>= 1)
	{
		*(buffPtr++) = (valueIn & nMask) ? '1':'0';
		if (nMask & 0xEF)
		{
			continue;
		}
		*(buffPtr++) = ' ';
	}
	*buffPtr = 0;
#ifdef __MACH__
	fprintf(stderr, "0x%02hhX %s\n", valueIn, binaryStr);
#else
	Serial.print("0x");
	Serial.print(valueIn, HEX);
	Serial.print(" ");
	Serial.println((const char*)binaryStr);
#endif
}
#endif

/************************************ Read ************************************/
/*
*	Unpacks 1 bit glyph rotated packed data, MSB bottom.
*	See XFontGlyph.h for packing details.
*/
uint32_t XFontRH1BitDataStream::Read(
	uint32_t	inLength,
	void*		outBuffer)
{
	if (mReadGlyphHeader)
	{
		mReadGlyphHeader = false;
		mBufferIndex = 0;
		mBytesInBuffer = 0;
		mBitsInByteIn = 0;
		mBitsInRowColumn = 0;
		mColumnsLeftInRow = 0;
		return(mSourceStream->Read(inLength, outBuffer));
	}
	if (inLength)
	{
		uint8_t		offsetBitsBy = mXFont->Glyph().y;
		uint8_t		bitsPerColumn = offsetBitsBy + mXFont->Glyph().rows;
		uint8_t*	oBufferPtr = (uint8_t*)outBuffer;
		uint8_t*	oBufferEnd = &oBufferPtr[inLength];
		uint8_t		byteIn = 0;
		uint8_t		byteOut = 0;
		int8_t		bitsInByteIn = mBitsInByteIn;

		/*
		*	If continuing from a paused unpack of an 8 bit byte THEN
		*	continue from where the unpacking stopped.
		*/
		if (bitsInByteIn)
		{
			byteIn = mByteIn;
		}
		
		if (mColumnsLeftInRow == 0)
		{
			mColumnsLeftInRow = mXFont->Glyph().columns;
		}
		
		uint8_t	bitsInByteOut = 0;
		uint8_t	bitsInColumn = mBitsInRowColumn;

		while (oBufferPtr != oBufferEnd)
		{
			if (bitsInColumn < offsetBitsBy)
			{
				if (offsetBitsBy - bitsInColumn > 8)
				{
					*(oBufferPtr++) = 0;
					/*
					*	If there are any columns left in this row THEN
					*	move to the next column
					*/
					if (mColumnsLeftInRow > 1)
					{
						mColumnsLeftInRow--;
					/*
					*	Else, move to the next row.
					*/
					} else
					{
						mColumnsLeftInRow = mXFont->Glyph().columns;
						mBitsInRowColumn += 8;
						bitsInColumn = mBitsInRowColumn;
					}
					//bitsInColumn += 8;
					continue;
				} else
				{
					bitsInByteOut = offsetBitsBy - bitsInColumn;
					bitsInColumn = offsetBitsBy;
					byteOut = 0;
				}
			}
			if (bitsInByteIn == 0)
			{
				byteIn = NextByte();
				bitsInByteIn = 8;
			}

			{
				uint8_t	bitsNeededToFillOut = 8-bitsInByteOut;
				if (bitsInByteOut)
				{
					byteOut |= (byteIn << bitsInByteOut);
				} else
				{
					byteOut = byteIn;
				}
				uint8_t	bitsNeededToFillColumn = bitsPerColumn - bitsInColumn;
				if (bitsNeededToFillOut > bitsNeededToFillColumn)
				{
					if (bitsInByteIn >= bitsNeededToFillColumn)
					{
						bitsInByteOut += bitsNeededToFillColumn;
						*(oBufferPtr++) = (byteOut & (0xFF >> (8-bitsInByteOut)));
						/*
						*	If there are any columns left in this row THEN
						*	move to the next column
						*/
						if (mColumnsLeftInRow > 1)
						{
							mColumnsLeftInRow--;
						/*
						*	Else, move to the next row.
						*/
						} else
						{
							mColumnsLeftInRow = mXFont->Glyph().columns;
							mBitsInRowColumn += 8;
						}
						bitsInColumn = mBitsInRowColumn;
						
						byteOut = 0;
						bitsInByteIn -= bitsNeededToFillColumn;
						byteIn >>= bitsNeededToFillColumn;
						bitsInByteOut = 0;
					} else
					{
						bitsInColumn += bitsInByteIn;
						bitsInByteOut += bitsInByteIn;
						bitsInByteIn = 0;
					}
				} else
				{
					if (bitsInByteIn >= bitsNeededToFillOut)
					{
						*(oBufferPtr++) = byteOut;
						byteOut = 0;
						bitsInByteOut = 0;
						/*
						*	If there are any columns left in this row THEN
						*	move to the next column
						*/
						if (mColumnsLeftInRow > 1)
						{
							mColumnsLeftInRow--;
						/*
						*	Else, move to the next row.
						*/
						} else
						{
							mColumnsLeftInRow = mXFont->Glyph().columns;
							mBitsInRowColumn += 8;
						}
						bitsInColumn = mBitsInRowColumn;

						bitsInByteIn -= bitsNeededToFillOut;
						byteIn >>= bitsNeededToFillOut;
					} else
					{
						bitsInColumn += bitsInByteIn;
						bitsInByteOut += bitsInByteIn;
						bitsInByteIn = 0;
					}
				}
			}
		}
		// Save unpack state
		//mBitsInRowColumn = bitsInColumn;
		mBitsInByteIn = bitsInByteIn;
		mByteIn = byteIn;
#ifdef __MACH__
		{
			// For testing on the Mac, reverse the bits
			oBufferPtr = (uint8_t*)outBuffer;
			uint8_t	maskIn;
			uint8_t maskOut;
			while (oBufferPtr != oBufferEnd)
			{
				byteIn = *oBufferPtr;
				byteOut = 0;
				maskIn = 1;
				maskOut = 0x80;
				while (maskIn)
				{
					if (byteIn & maskIn)
					{
						byteOut |= maskOut;
					}
					maskIn <<= 1;
					maskOut >>= 1;
				}
				*(oBufferPtr++) = byteOut;
			}
		}
#endif
	}
	return(inLength);
}

