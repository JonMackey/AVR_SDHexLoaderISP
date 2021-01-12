/*
*	AVRStreamISP.cpp, Copyright Jonathan Mackey 2020
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

#include "AVRStreamISP.h"
#include "stk500.h"
#ifdef __MACH__
#include "ContextualStream.h"
#include <stdio.h>
#else
#include <Arduino.h>
#include "SDHexLoaderConfig.h"
#endif
#include "AVRConfig.h"

/******************************** AVRStreamISP ********************************/
AVRStreamISP::AVRStreamISP(void)
: mInProgMode(false)
{
}

/*********************************** begin ************************************/
void AVRStreamISP::begin(void)
{
	mStream = nullptr;
#ifndef __MACH__
	pinMode(Config::kProgModePin, OUTPUT);
	PulseLED(Config::kProgModePin, 2);
	pinMode(Config::kErrorPin, OUTPUT);
	PulseLED(Config::kErrorPin, 2);
	pinMode(Config::kHeartbeatPin, OUTPUT);
	PulseLED(Config::kHeartbeatPin, 2);
	pinMode(Config::kResetPin, INPUT_PULLUP);
#if (HEX_LOADER_VER >= 12)
	pinMode(Config::kReset3v3OEPin, OUTPUT);
	digitalWrite(Config::kReset3v3OEPin, HIGH);
#endif

	mISP_OE_BitMask = digitalPinToBitMask(Config::kISP_OE_Pin);
	mISP_OE_PortReg = portOutputRegister(digitalPinToPort(Config::kISP_OE_Pin));
	digitalWrite(Config::kISP_OE_Pin, HIGH);
	pinMode(Config::kISP_OE_Pin, OUTPUT);
#endif
}

/********************************* SetStream **********************************/
/*
*	This should be called before SetAVRConfig.  In addition to setting the
*	stream, this routine sets up defaults used if SetAVRConfig isn't called.
*/
void AVRStreamISP::SetStream(
	Stream*		inStream)
{
	mStream = inStream;
	mEEPromPageSize = 4;
#ifndef __MACH__
//	mEEPromMinWriteDelay = 4500;	// default
//	mFlashMinWriteDelay = 4500;		// default
#endif
#ifdef DEBUG_AVR_STREAM
	mReceiving = false;
#endif
}

/********************************* SetSPIClock ********************************/
/*
*	As per the docs - Depending on CKSEL Fuses, a valid clock must be present.
*	The minimum low and high periods for the serial clock (SCK) input are
*	defined as follows:
*		Low:  more than 2 CPU clock cycles for fck < 12 MHz, 3 for fck >= 12 MHz
*		High: more than 2 CPU clock cycles for fck < 12 MHz, 3 for fck >= 12 MHz
*
*	This translates to dividing by 6 for CPU clocks below 12 MHz and 8 for
*	anything for 12 MHz and up.
*
*	The original ArduinoISP code uses 1MHz/6.
*
*	If the target MCU's CKSEL fuses haven't been set then you have to assume
*	1MHz/6 as the ArduinoISP does.  Assuming a slow clock speed slows the
*	loading time for everything.  To get around this the settings screen allows
*	you to set the target clock speed.  This setting is only used in USB
*	pass-through mode.  Because you can't load hex from SD if the CKSEL fuses
*	haven't been set, when loading hex from SD the frequency read from the
*	config file is always used. 
*/
void AVRStreamISP::SetSPIClock(
	uint32_t	inClock)
{
#ifndef __MACH__
	uint32_t	spiClock = inClock ? (inClock/(inClock < 12000000 ? 6:8)) :
						(1000000/6);	// 1MHz default when inClock is 0. 
	mSPISettings = SPISettings(spiClock, MSBFIRST, SPI_MODE0);

#endif
}

/******************************** SetAVRConfig ********************************/
/*
*	Extract any SAVRConfig params needed.
*	This is only called after SetStream when reading from the SD card.
*/
void AVRStreamISP::SetAVRConfig(
	const SAVRConfig&	inAVRConfig)
{
#ifdef __MACH__
	memcpy(mSignature, inAVRConfig.signature, 3);
#else
//	mEEPromMinWriteDelay = inAVRConfig.eepromMinWriteDelay;
//	mFlashMinWriteDelay = inAVRConfig.flashMinWriteDelay;
	SetSPIClock(inAVRConfig.fCPU);
#endif
}

/********************************* Heartbeat **********************************/
/*
*	Provides a Heartbeat, so you can tell the software is running.
*/
void AVRStreamISP::Heartbeat(void)
{
#ifndef __MACH__
	static uint32_t	lastTime = 0;
	static uint8_t	hbval = 128;
	static int8_t	hbdelta = 8;
	
	uint32_t now = millis();
	if ((now - lastTime) >= 40)
	{
		lastTime = now;
		if (hbval > 192)
		{
			hbdelta = -hbdelta;
		} else if (hbval < 32)
		{
			hbdelta = -hbdelta;
		}
		hbval += hbdelta;
	/*
	*	The heartbeat on the wrong pin.  In version 1.3 it *is* on a timer, but
	*	that timer is being used by the RTC (TIMER2A).  If there will be another
	*	version, PD4 (TIMER1B) should probably work.
	*/
	#if (HEX_LOADER_VER > 13)
		analogWrite(Config::kHeartbeatPin, hbval);
	#else
		digitalWrite(Config::kHeartbeatPin, hbval > 128);
	#endif
	}
#endif
}

/********************************** LogError **********************************/
void AVRStreamISP::LogError(
	uint8_t	inError)
{
	mError = inError;
#ifndef __MACH__
	digitalWrite(Config::kErrorPin, HIGH);
#endif
}

/********************************* ResetError *********************************/
void AVRStreamISP::ResetError(
	bool	inLeaveProgMode)
{
	mError = eNoErr;
#ifndef __MACH__
	digitalWrite(Config::kErrorPin, LOW);
#endif
	if (inLeaveProgMode &&
		mInProgMode)
	{
		LeaveProgMode();
	}
}

/************************************ Halt ************************************/
void AVRStreamISP::Halt(void)
{
	if (mStream)
	{
		ResetError(true);
		SetStream(nullptr);	// SetStream also resets several settings possibly
							// changed by SetAVRConfig
	#ifndef __MACH__
		digitalWrite(Config::kHeartbeatPin, LOW);
	#endif
	}
}

/************************************ read ************************************/
uint8_t AVRStreamISP::read(void)
{
	/*
	*	Note that this will block when the stream is empty.
	*/
	while (!mStream->available());
	uint8_t	thisChar = mStream->read();
#ifdef DEBUG_AVR_STREAM
#ifdef __MACH__
	if (!mReceiving)
	{
		mReceiving = true;
		fprintf(stderr, "\n<");
	}
	fprintf(stderr, " %02hhX", thisChar);
#else
	if (!mReceiving)
	{
		mReceiving = true;
		Serial1.print('\n');
		Serial1.print('<');
	}
	Serial1.print(' ');
	if (thisChar < 0x10)
	{
		Serial1.print('0');
	}
	Serial1.print(thisChar, HEX);
#endif
#endif
  	return(thisChar);
}

/*********************************** write ************************************/
/*
*	Bottleneck for all stream writes.
*/
void AVRStreamISP::write(
	uint8_t	inChar)
{
	mStream->write(inChar);
#ifdef DEBUG_AVR_STREAM
#ifdef __MACH__
	if (mReceiving)
	{
		mReceiving = false;
		fprintf(stderr, "\n>");
	}
	fprintf(stderr, " %02hhX", inChar);
#else
	if (mReceiving)
	{
		mReceiving = false;
		Serial1.write('\n');
		Serial1.write('>');
	}
	Serial1.print(' ');
	if (inChar < 0x10)
	{
		Serial1.write('0');
	}
	Serial1.print(inChar, HEX);
#endif
#endif
}

/********************************* FillBuffer *********************************/
void AVRStreamISP::FillBuffer(
	uint16_t inLength)
{
	if (inLength <= sizeof(mBuffer))
	{
		for (uint16_t i = 0; i < inLength; i++)
		{
			mBuffer[i] = read();
		}
	} else
	{
		LogError(eBufferOverflowErr);
	}
}

#define PTIME_30MS 30
/********************************** PulseLED **********************************/
void AVRStreamISP::PulseLED(
	uint8_t inPin,
	uint8_t inPulses)
{
#ifndef __MACH__
	do
	{
		digitalWrite(inPin, HIGH);
		delay(PTIME_30MS);
		digitalWrite(inPin, LOW);
		delay(PTIME_30MS);
	} while (inPulses--);
#endif
}

/**************************** TransferInstruction *****************************/
uint8_t AVRStreamISP::TransferInstruction(
	uint8_t inByte1,
	uint8_t inByte2,
	uint8_t inByte3,
	uint8_t inByte4)
{
#ifdef __MACH__
	return(0);
#else
	SPI.transfer(inByte1);
	SPI.transfer(inByte2);
	SPI.transfer(inByte3);
	return(SPI.transfer(inByte4));
#endif
}

/******************************** DoEmptyReply ********************************/
void AVRStreamISP::DoEmptyReply(void)
{
	if (read() == CRC_EOP)
	{
		write(STK_INSYNC);
		write(STK_OK);
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/******************************* DoOneByteReply *******************************/
void AVRStreamISP::DoOneByteReply(
	uint8_t inByte)
{
	if (read() == CRC_EOP)
	{
		write(STK_INSYNC);
		write(inByte);
		write(STK_OK);
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/***************************** GetParameterValue ******************************/
void AVRStreamISP::GetParameterValue(
	uint8_t inParameter)
{
	switch (inParameter)
	{
		case 0x80:		// Hardware Version
			DoOneByteReply(2);
			break;
		case 0x81:		// Software Major Version 
			DoOneByteReply(1);
			break;
		case 0x82:		// Software Minor Version
			DoOneByteReply(18);
			break;
		case 0x93:
			DoOneByteReply('S'); // Serial programmer
			break;
		//0x83		Status LED
		//0x84		Target Voltage
		//0x85		Adjustable Voltage
		//0x86		Oscillator Timer Prescaler Value
		//0x87		Oscillator Timer Compare Match Value
		//0x89		ISP SCK Duration
		//0x90:0x91	Buffer Size Low:High
		//0x98		Topcard Detect
		default:
			DoOneByteReply(0);
			break;
	}
}

/**************************** SetDeviceProgParams *****************************/
/*
*	Most of the parameters of the STK_SET_DEVICE command aren't used.
*	See "Set Device Programming Parameters" in the AVR061 doc.
*
*	[0] devicecode - Device code as defined in “devices.h”
*	[1] revision - Device revision. Currently not used. Should be 0.
*	[2] progtype - Defines which Program modes is supported:
*			“0” – Both Parallel/High-voltage and Serial mode
*			“1” – Only Parallel/High-voltage mode
*	[3] parmode - Defines if the device has a full parallel interface or a
*			pseudo parallel programming interface:
*			“0” – Pseudo parallel interface
*			“1” – Full parallel interface
*	[4] polling - Defines if polling may be used during SPI access:
*			“0” – No polling may be used
*			“1” – Polling may be used
*	[5] selftimed - Defines if programming instructions are self timed:
*			“0” – Not self timed
*			“1” – Self timed
*	[6] lockbytes - Number of Lock bytes. Currently not used. Should be set to
*			actual number of Lock bytes for future compability.
*	[7] fusebytes - Number of Fuse bytes. Currently not used. Should be set to
*			actual number of Fuse bytes for future caompability.
*
*	Multi byte parameters values are big endian...
*
*	[8:9] flashpollval - FLASH polling value. See Data Sheet for the device.
*	[10:11] eeprompollval - EEPROM polling value, See data sheet for the device.
*	[12:13] pagesize - Page size in bytes for pagemode parts
*	[14:15] eepromsize - EEPROM size in bytes
*	[16:19] flashsize - FLASH size in bytes.
*/
void AVRStreamISP::SetDeviceProgParams(void)
{
	FillBuffer(20);
	mProgramPageSize = ((mBuffer[12] << 8) | mBuffer[13]);

	mEEPromSize = ((mBuffer[14] << 8) | mBuffer[15]);

	// AVR devices have active low reset, AT89Sx are active high.
	//
	// If devicecode is not an AVR device
	mReset = mBuffer[0] >= 0xE0;
	DoEmptyReply();
}

/*************************** SetExtDeviceProgParams ***************************/
/*
*	Most of the parameters of the STK_SET_DEVICE_EXT command aren't used.
*	See "Set Extended Device Programming Parameters" in the AVR061 doc.
*
*	[0] commandsize - >> Differs from doc specification.  The doc says it's the
*			number of bytes to follow.  Avrdude sends the total size including
*			commandsize, so 5 instead of 4 as noted in the spec.
*	[1]	eeprompagesize - EEPROM page size in bytes.
*	[2] signalpagel - Defines to which port pin the PAGEL signal should be
*			mapped. Example: signalpagel = 0xD7. In this case PAGEL should be
*			mapped to PORTD7.
*	[3] signalbs2 - Defines to which port pin the BS2 signal should be mapped.
*			See signalpagel.
*	[4] ResetDisable - Defines whether a part has RSTDSBL Fuse (value = 1) or
*			not (value = 0).
*/
void AVRStreamISP::SetExtDeviceProgParams(void)
{
	uint8_t	commandsize = read();
	mEEPromPageSize = read();
	FillBuffer(commandsize-2);
	DoEmptyReply();
}

/******************************* EnterProgMode ********************************/
/*
*	This differs from the original ArduinoIDE code in that the target MCU
*	appears to the host MCU as a normal SPI device where the bus is released
*	when the transaction ends by putting hex buffers driving MOSI, MISO and SCK
*	between the board and the target MCU into tristate mode.  The MOSI and SCK
*	signals have pullup resistors between the hex buffer and the MCU, so to the
*	target MCU it appears that the SPI bus is idle.  The target MCU is kept in
*	programming mode till the reset line is released by calling LeaveProgMode().
*/
void AVRStreamISP::EnterProgMode(void)
{
#ifdef __MACH__
	mInProgMode = true;
#else
#if (HEX_LOADER_VER >= 12)
	digitalWrite(Config::kReset3v3OEPin, LOW);
#endif
	pinMode(Config::kResetPin, OUTPUT);
	digitalWrite(Config::kResetPin, mReset);
	BeginTransaction();

	// See AVR datasheets, chapter "SERIAL_PRG Programming Algorithm":

	// Pulse the kResetPin after Config::kSCK is low:
	digitalWrite(Config::kSCK, LOW);
	delay(20); // discharge SCK, value arbitrarily chosen
	digitalWrite(Config::kResetPin, !mReset);
	// Pulse must be a minimum of 2 target CPU clock cycles, so 100 usec is ok
	// for CPU speeds above 20 KHz
	delayMicroseconds(100);
	digitalWrite(Config::kResetPin, mReset);

	// Send the enable programming command:
	delay(50); // datasheet: must be > 20 msec
	TransferInstruction(0xAC, 0x53, 0x00, 0x00);
	mInProgMode = true;
	digitalWrite(Config::kProgModePin, HIGH);
#endif
}

/******************************* LeaveProgMode ********************************/
void AVRStreamISP::LeaveProgMode(void)
{
#ifdef __MACH__
	mInProgMode = false;
#else
	EndTransaction();
	digitalWrite(Config::kResetPin, !mReset);
	delay(1); // See comment in SDHexSession::Halt()
	pinMode(Config::kResetPin, INPUT_PULLUP);
#if (HEX_LOADER_VER >= 12)
	digitalWrite(Config::kReset3v3OEPin, HIGH);
#endif
	mInProgMode = false;
	digitalWrite(Config::kProgModePin, LOW);
#endif
}

/********************************* Universal **********************************/
void AVRStreamISP::Universal(void)
{
	FillBuffer(4);
#ifdef __MACH__
	if (mBuffer[0] == 0x4D)
	{
		mBaseAddress = ((uint32_t)mBuffer[2]) << 17;
	}
	DoOneByteReply(0);
#else
	uint8_t reply = TransferInstruction(mBuffer[0], mBuffer[1], mBuffer[2], mBuffer[3]);
	DoOneByteReply(reply);
#endif
}

/****************************** WriteMemoryPage *******************************/
void AVRStreamISP::WriteMemoryPage(
	uint8_t		inInst,	// 0x4C or 0xC2, Program or EEPROM
	uint16_t	inAddress)
{
#ifdef __MACH__
	WritePageByte(inInst, inAddress, 0);
#else
	//digitalWrite(Config::kProgModePin, LOW);

	WritePageByte(inInst, inAddress, 0);

	/*
	*	The 30ms delay below is from the original ArduinoISP code, is I assume
	*	both to delay the flicker of the program LED as well as provide a delay
	*	for the write page command to complete, although a delay isn't needed
	*	because the caller should manage the delay via the stream (avrdude or
	*	the internal SDHexSession.)
	*/
	// delay(PTIME_30MS);
	//digitalWrite(Config::kProgModePin, HIGH);
#endif
}

/********************************* WriteProgram *********************************/
void AVRStreamISP::WriteProgram(
	uint16_t inLength)
{
	FillBuffer(inLength);
	if (read() == CRC_EOP)
	{
		write(STK_INSYNC);
		write(WriteProgramPages(inLength));
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/***************************** WriteProgramPages ******************************/
uint8_t AVRStreamISP::WriteProgramPages(
	uint16_t inLength)
{
	/*
	*	AVR program addressing is per word, so mAddress is a word index.
	*	For mProgramPageSize, mask is one of:
	*	4	= FFFE
	*	8	= FFFC
	*	16	= FFF8
	*	32	= FFF0
	*	64	= FFE0
	*	128	= FFC0
	*	256	= FF80
	*/
	uint16_t	wordsPerPage = mProgramPageSize >> 1;
	uint16_t	pageAddress = mAddress & (~(wordsPerPage -1));
	uint16_t	nextPageAddress = pageAddress + wordsPerPage;
	for (uint16_t i = 0; i < inLength; )
	{
		/*
		*	The test below may actually be detecting an error.  Only complete
		*	pages should ever be written, and only one page should be passed at
		*	a time.  If pageAddress != mAddress when entering this routine, the
		*	result will be corrupted flash because the page buffer bits are
		*	undefined.  This test was part of the original ArduinoISP code.
		*/
		if (nextPageAddress == mAddress)
		{
			WriteMemoryPage(0x4C, pageAddress);
			pageAddress = nextPageAddress;
			nextPageAddress += wordsPerPage;
		}
		// As per doc, the low byte must be written before the high byte.
		// 0x40 - write low, 0x48 - write high
		WritePageByte(0x40, mAddress, mBuffer[i++]);
		WritePageByte(0x48, mAddress, mBuffer[i++]);
		mAddress++;	// Increment word address
	}

	WriteMemoryPage(0x4C, pageAddress);

	return(STK_OK);
}

#define EECHUNK (32)
/******************************** WriteEeprom *********************************/
uint8_t AVRStreamISP::WriteEeprom(
	uint16_t	inLength)
{
	// mAddress is a word address, 'start' is the byte address
	uint16_t start = mAddress << 1;
	uint16_t remaining = inLength;
	uint16_t dataOffset = 0;
	if (inLength > mEEPromSize ||
		inLength > 256)
	{
		LogError(eEEPROMBufferErr);
		return(STK_FAILED);
	}
	FillBuffer(inLength);
	/*
	*	If the address is page aligned AND
	*	the length is the EEPROM page size THEN
	*	use page mode to write it.
	*	Most EEPROM pages sizes are either 4 or 8 bytes.
	*/
	if ((start % mEEPromPageSize) == 0 &&
		inLength == mEEPromPageSize)
	{
		for (uint16_t i = 0; i < inLength; i++)
		{
			WritePageByte(0xC1, start+i, mBuffer[i]);
		}
		WriteMemoryPage(0xC2, start);
	/*
	*	Else fall back to the original ArduinoISP code.
	*/
	} else
	{
		while (remaining > EECHUNK)
		{
			WriteEepromChunk(start, EECHUNK, dataOffset);
			start += EECHUNK;
			dataOffset += EECHUNK;
			remaining -= EECHUNK;
		}
		WriteEepromChunk(start, remaining, dataOffset);
	}
	return(STK_OK);
}
/****************************** WriteEepromChunk ******************************/
// write (inLength) bytes, (inStart) is a byte address
uint8_t AVRStreamISP::WriteEepromChunk(
	uint16_t inStart,
	uint16_t inLength,
	uint16_t inDataOffset)
{
#ifdef __MACH__
	for (uint16_t i = 0; i < inLength; i++)
	{
		uint16_t addr = inStart + i;
		TransferInstruction(0xC0, addr >> 8, addr, mBuffer[i+inDataOffset]);
	}
#else
//	digitalWrite(Config::kProgModePin, LOW);
	for (uint16_t i = 0; i < inLength; i++)
	{
		uint16_t addr = inStart + i;
		TransferInstruction(0xC0, addr >> 8, addr, mBuffer[i+inDataOffset]);
		/*
		*	The original ArduinoISP code had the delay set to 45ms.  My guess is
		*	the author was shooting for 4.5ms
		*/
		delay(5);	// I haven't seen a documented delay greaterthan 4.5ms
	}
//	digitalWrite(Config::kProgModePin, HIGH);
#endif
	return(STK_OK);
}

/******************************** ProgramPage *********************************/
void AVRStreamISP::ProgramPage(void)
{
	uint8_t result = STK_FAILED;
	uint16_t length = read() << 8;
	length |= read();

	// flash memory @mAddress, (length) bytes
	switch (read())
	{
		case 'F':
			WriteProgram(length);
			break;
		case 'E':
		{
			result = WriteEeprom(length);
			if (read() == CRC_EOP)
			{
				write(STK_INSYNC);
				write(result);
			} else
			{
				LogError(eSyncErr);
				write(STK_NOSYNC);
			}
			break;
		}
		default:
			write(STK_FAILED);
			break;
	}
}

/******************************** ReadPageByte ********************************/
uint8_t AVRStreamISP::ReadPageByte(
	uint8_t		inInst,
	uint16_t	inAddress)
{
#ifdef __MACH__
	uint8_t	byteOut = 0;
	/*
	*	If not reading byte aligned EEPROM...
	*/
	if (inInst != 0xA0)	// 0xA0 = Read EEPROM Memory
	{
		uint32_t	address = mBaseAddress + ((uint32_t)inAddress<<1);
		if (inInst == 0x20)	// 0x20 = Read Flash Memory, Low byte
		{
			byteOut = mFlashMem[address];
		} else if (inInst == 0x28)	// 0x28 = Read Flash Memory, High byte
		{
			byteOut = mFlashMem[address +1];
		}
	} else
	{
		byteOut = mFlashMem[inAddress];
	}
	return(byteOut);
#else
	/*
	*	The transfer instruction below is misleading in that it doesn't follow
	*	the "Serial Programming Instruction Set" definition where byte2 should
	*	be zero, and only the 6 LSB of the address as byte3.  Apparently the
	*	extra bits are ignored.
	*/
	return(TransferInstruction(inInst, inAddress >> 8, inAddress, 0));
#endif
}

/******************************* WritePageByte ********************************/
void AVRStreamISP::WritePageByte(
	uint8_t		inInst,
	uint16_t	inAddress,
	uint8_t		inByte)
{
#ifdef __MACH__
	/*
	*	If not writing byte aligned EEPROM...
	*/
	if (inInst != 0xC1)	// 0xC1 = Load EEPROM Memory Page (page access)
	{
		uint32_t	address = mBaseAddress + ((uint32_t)inAddress<<1);
		if (inInst == 0x40)	// 0x40 = Load Flash Memory Page, Low byte
		{
			mFlashMem[address] = inByte;
		} else if (inInst == 0x48)	// 0x48 = Load Flash Memory Page, High byte
		{
			mFlashMem[address +1] = inByte;
		}
	} else
	{
		mFlashMem[inAddress] = inByte;
	}
#else
	/*
	*	The transfer instruction below is misleading in that it doesn't follow
	*	the "Serial Programming Instruction Set" definition where byte2 should
	*	be zero, and only the 6 LSB of the address as byte3.  Apparently the
	*	extra bits are ignored.
	*/
	TransferInstruction(inInst, inAddress >> 8, inAddress, inByte);
#endif
}

/****************************** ReadProgramPage *******************************/
uint8_t AVRStreamISP::ReadProgramPage(
	uint16_t inLength)
{
	for (uint16_t i = 0; i < inLength; i += 2)
	{
		write(ReadPageByte(0x20, mAddress));
		write(ReadPageByte(0x28, mAddress));
		mAddress++;
	}
	return(STK_OK);
}

/******************************* ReadEepromPage *******************************/
uint8_t AVRStreamISP::ReadEepromPage(
	uint16_t inLength)
{
	// mAddress is a word address, 'start' is the byte address
	uint16_t start = mAddress << 1;
	for (uint16_t i = 0; i < inLength; i++)
	{
		uint16_t addr = start + i;
		write(ReadPageByte(0xA0, addr));
	}
	return(STK_OK);
}

/********************************** ReadPage **********************************/
void AVRStreamISP::ReadPage(void)
{
	uint16_t	length;
	{
		uint8_t		hi = read();
		uint8_t		lo = read();
		length = (hi << 8) | lo;
	}
	uint8_t memtype = read();
	if (read() == CRC_EOP)
	{
		uint8_t		result = STK_FAILED;
		write(STK_INSYNC);
		if (memtype == 'F')
		{
			result = ReadProgramPage(length);
		} else if (memtype == 'E')
		{
			result = ReadEepromPage(length);
		}
		write(result);
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/******************************* ReadSignature ********************************/
void AVRStreamISP::ReadSignature(void)
{
	if (read() == CRC_EOP)
	{
		write(STK_INSYNC);
		for (uint8_t i = 0; i < 3; i++)
		{
		#ifdef __MACH__
			write(mSignature[i]);
		#else
			write(TransferInstruction(0x30, 0x00, i, 0x00));
		#endif
		}
		write(STK_OK);
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/****************************** ReadCalibration *******************************/
void AVRStreamISP::ReadCalibration(void)
{
	if (read() == CRC_EOP)
	{
		write(STK_INSYNC);
		write(TransferInstruction(0x38, 0x00, 0x00, 0x00));
		write(STK_OK);
	} else
	{
		LogError(eSyncErr);
		write(STK_NOSYNC);
	}
}

/*********************************** Update ***********************************/
bool AVRStreamISP::Update(void)
{
	if (mStream &&
		mStream->available())
	{
	#ifndef __MACH__
		if (mInProgMode)
		{
			BeginTransaction();
		}
	#endif
		uint8_t command = read();
		switch (command)
		{
			// Expecting a command, not CRC_EOP
			// this is how we can get back in sync
			case CRC_EOP:				// 0x20
				LogError(eSyncErr);
				write(STK_NOSYNC);
				break;
			case STK_GET_SYNC:			// 0x30
				ResetError();
				DoEmptyReply();
				break;
			case STK_GET_SIGN_ON:		// 0x31
				if (read() == CRC_EOP)
				{
					write(STK_INSYNC);
					mStream->write((const uint8_t*)"AVR ISP", 7);
				#ifdef DEBUG_AVR_STREAM
				#ifdef __MACH__
					fprintf(stderr, "AVR ISP");
				#else
					Serial1.print("AVR ISP");
				#endif
				#endif
					write(STK_OK);
				} else
				{
					LogError(eSyncErr);
					write(STK_NOSYNC);
				}
				break;
			case STK_GET_PARAMETER:		// 0x41
				GetParameterValue(read());
				break;
			case STK_SET_DEVICE:		// 0x42
				SetDeviceProgParams();
				break;
			case STK_SET_DEVICE_EXT:	// 0x45
				SetExtDeviceProgParams();
				break;
			case STK_ENTER_PROGMODE:	// 0x50
				if (!mInProgMode)
				{
					EnterProgMode();
				}
				DoEmptyReply();
				break;
			case STK_LEAVE_PROGMODE:	// 0x51
				ResetError(true);	// Will call LeaveProgMode()
				DoEmptyReply();
				break;
			case STK_LOAD_ADDRESS:		// 0x55 set address (word)
				mAddress = read();
				mAddress |= (read() << 8);
				DoEmptyReply();
				break;
			case STK_UNIVERSAL:			// 0x56
				Universal();
				break;
			case STK_PROG_FLASH:		// 0x60
				read(); // low addr
				read(); // high addr
				DoEmptyReply();
				break;
			case STK_PROG_DATA:			// 0x61
				read(); // data
				DoEmptyReply();
				break;
			case STK_PROG_PAGE:			// 0x64
				ProgramPage();
				break;
			case STK_READ_PAGE:			// 0x74
				ReadPage();
				break;
			case STK_READ_SIGN:			// 0x75
				ReadSignature();
				break;
			case STK_READ_OSCCAL:		// 0x76
				ReadCalibration();
				break;
			// anything else we will return STK_UNKNOWN
			default:
				LogError(eUnknownErr);
				write(read() == CRC_EOP ? STK_UNKNOWN : STK_NOSYNC);
				break;
		}
	#ifndef __MACH__
		if (mInProgMode)
		{
			EndTransaction();
		}
	#endif
	} else
	{
		Heartbeat();
	}
	return(mError <= eSyncErr);
}
