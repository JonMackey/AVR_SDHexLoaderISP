/*
*	DataStream.h, Copyright Jonathan Mackey 2019
*	Base class for a generic data stream.
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
#ifndef DataStream_h
#define DataStream_h

#include <inttypes.h>

class DataStream
{
public:
							DataStream(void){}

	enum EOrigin
	{
		eSeekSet,
		eSeekCur,
		eSeekEnd
	};

	virtual uint32_t		Read(
								uint32_t				inLength,
								void*					outBuffer) = 0;
	virtual uint32_t		Write(
								uint32_t				inLength,
								const void*				inBuffer) = 0;
	virtual bool			Seek(
								int32_t					inOffset,
								EOrigin					inOrigin) = 0;
	virtual uint32_t		GetPos(void) const = 0;
	virtual bool			AtEOF(void) const = 0;
	virtual uint32_t		Clip(
								uint32_t				inLength) const = 0;	
};

class DataStreamImpl : public DataStream
{
public:
							DataStreamImpl(
								const void*				inStartAddress,
								uint32_t				inLength);

	virtual bool			Seek(
								int32_t					inOffset,
								EOrigin					inOrigin);
	virtual uint32_t		GetPos(void) const;
	virtual bool			AtEOF(void) const;
	virtual uint32_t		Clip(
								uint32_t				inLength) const;

protected:
	const uint8_t*	mStartAddr;
	const uint8_t*	mCurrent;
	const uint8_t*	mEndAddr;
	
};

// SRAM data stream.
class DataStream_S : public DataStreamImpl
{
public:
							DataStream_S(
								const void*				inStartAddress,
								uint32_t				inLength);
	virtual uint32_t		Read(
								uint32_t				inLength,
								void*					outBuffer);
	virtual uint32_t		Write(
								uint32_t				inLength,
								const void*				inBuffer);

};

// Read-only PROGMEM data stream.
class DataStream_P : public DataStreamImpl
{
public:
							DataStream_P(
								const void*				inStartAddress,
								uint32_t				inLength);
	virtual uint32_t		Read(
								uint32_t				inLength,
								void*					outBuffer);
	virtual uint32_t		Write(
								uint32_t				inLength,
								const void*				inBuffer);

};

// EEPROM data stream.
class DataStream_E : public DataStreamImpl
{
public:
							DataStream_E(
								const void*				inStartAddress,
								uint32_t				inLength);
	virtual uint32_t		Read(
								uint32_t				inLength,
								void*					outBuffer);
	virtual uint32_t		Write(
								uint32_t				inLength,
								const void*				inBuffer);

};
#endif // DataStream_h
