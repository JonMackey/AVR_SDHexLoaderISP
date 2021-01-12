/*
*	AVRStreamISP.h, Copyright Jonathan Mackey 2020

	This is a port of ArduinoISP.ino that allows for other devices on the SPI
	bus. This requires extra hardware to disconnect the ISP SPI signals when
	using other SPI devices.  When the ISP SPI signals are disconnected these
	signals appear to the target MCU as idle (SCK, MOSI pulled low, reset held
	low.)
	
	This port exposes some of the STK500 routines of ArduinoISP so they can be
	used to simulate a serial avrdude session.
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
#ifndef AVRStreamISP_h
#define AVRStreamISP_h

#include <inttypes.h>
#ifdef __MACH__
#define Stream	ContextualStream
#define DEBUG_AVR_STREAM	1
#else
#include <SPI.h>
/*
*	Defining DEBUG_AVR_STREAM will send a dump of the SDK500 commands/responses
*	to Serial1.
*/
//#define DEBUG_AVR_STREAM	1
#endif

class Stream;
struct SAVRConfig;

class AVRStreamISP
{
public:
							AVRStreamISP(void);
	void					begin(void);
	void					SetStream(
								Stream*					inStream);
	void					SetSPIClock(
								uint32_t				inClock = 0);
	bool					Update(void);
	void					SetAVRConfig(
								const SAVRConfig&		inAVRConfig);
	uint8_t					Error(void) const
								{return(mError);}
	void					ResetError(
								bool					inLeaveProgMode = false);
	void					Halt(void);
	bool					InProgMode(void) const
								{return(mInProgMode);}
	enum EErrors
	{
		eNoErr,
		eSyncErr,
		eBufferOverflowErr,
		eEEPROMBufferErr,
		eUnknownErr
	};
protected:
	Stream*		mStream;
#ifdef __MACH__
	uint32_t	mAddress;	// 2 byte word address
#else
	uint16_t	mAddress;	// 2 byte word address
#endif
//	uint16_t	mEEPromMinWriteDelay;
	uint16_t	mEEPromSize;
//	uint16_t	mFlashMinWriteDelay;
	uint16_t	mProgramPageSize;
	uint8_t		mBuffer[256];
	uint8_t		mEEPromPageSize; 
	uint8_t		mError;
#ifdef __MACH__
	uint8_t		mSignature[3];
	uint8_t		mFlashMem[0x40000];
	uint32_t	mBaseAddress;
#else
	uint8_t		mISP_OE_BitMask;
#endif
	bool		mInProgMode;
	bool		mReset;
#ifndef __MACH__
	volatile uint8_t*	mISP_OE_PortReg;
	SPISettings	mSPISettings;
#endif
#ifdef DEBUG_AVR_STREAM
	bool		mReceiving;
#endif

#ifndef __MACH__
	inline void				BeginTransaction(void)
							{
								SPI.beginTransaction(mSPISettings);
								*mISP_OE_PortReg &= ~mISP_OE_BitMask;
							}

	inline void				EndTransaction(void)
							{
								*mISP_OE_PortReg |= mISP_OE_BitMask;
								SPI.endTransaction();
							}
#endif
	void					Heartbeat(void);
	void					LogError(
								uint8_t					inError);
	uint8_t					read(void);
	void					write(
								uint8_t					inChar);
	void					FillBuffer(
								uint16_t 				inLength);
	void					PulseLED(
								uint8_t					inPin,
								uint8_t					inPulses);
	uint8_t					TransferInstruction(
								uint8_t 				inByte1,
								uint8_t					inByte2,
								uint8_t					inByte3,
								uint8_t					inByte4);
	void					DoEmptyReply(void);
	void					DoOneByteReply(
								uint8_t					inByte);
	void					GetParameterValue(
								uint8_t					inParameter);
	void					SetDeviceProgParams(void);
	void					SetExtDeviceProgParams(void);
	void					EnterProgMode(void);
	void					LeaveProgMode(void);
	void					Universal(void);
	void					WriteMemoryPage(
								uint8_t					inInst,
								uint16_t				inAddress);
	void					WriteProgram(
								uint16_t				inLength);
	uint8_t					WriteProgramPages(
								uint16_t				inLength);
	uint8_t					WriteEeprom(
								uint16_t				inLength);
	uint8_t					WriteEepromChunk(
								uint16_t				inStart,
								uint16_t 				inLength,
								uint16_t 				inDataOffset);
	void					ProgramPage(void);
	uint8_t					ReadPageByte(
								uint8_t					inInst,
								uint16_t				inAddress);
	void					WritePageByte(
								uint8_t					inInst,
								uint16_t				inAddress,
								uint8_t					inByte);
	uint8_t					ReadProgramPage(
								uint16_t				inLength);
	uint8_t					ReadEepromPage(
								uint16_t				inLength);
	void					ReadPage(void);
	void					ReadSignature(void);
	void					ReadCalibration(void);
};

#endif // AVRStreamISP_h
