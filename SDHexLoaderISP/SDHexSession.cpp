/*
*	SDHexSession.cpp, Copyright Jonathan Mackey 2020
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
*
*	This isn't a very flexible class at this point.  It originally only
*	supported loading the application section.  Support for setting fuses, 
*	lock bits and bootloader, if any, was addded later.  To differentiate
*	between the 4 possible operations the mOperation member was added.
*/

#include "SDHexSession.h"
#include "stk500.h"
#ifdef __MACH__
#include "ContextualStream.h"
#else
#include <Arduino.h>
#include "SDHexLoaderConfig.h"
#endif
#include "AVRStreamISP.h"
#include "UnixTime.h"

#ifndef __MACH__
const uint32_t	kSessionTimeout = 2000;	// milliseconds
#endif
const char kBootloaderPathPrefixStr[] PROGMEM = "bootloaders/B";
const char kHexExtensionStr[] PROGMEM = ".hex";

const SDHexSession::SFuseInst SDHexSession::kFuseInst[] =
{
	{0x50, 8, 0xA4},	// Extended
	{0x58, 8, 0xA8},	// High
	{0x50, 0, 0xA0}		// Low
};


/******************************** SDHexSession ********************************/
SDHexSession::SDHexSession(void)
: mStage(eSessionCompleted)
{
}

/*********************************** begin ************************************/
/*
*	Supported functions:
*	- load + verify
*	- load fuses + load bootloader (if applicable) + verify (if bootloader)
*	For load + verify to Serial, inStream should point to the Serial1 stream.
*	For load + verify to ISP, inStream should be nil
*	For load fuses (and bootloader if applicable), inStream should be nil
*	because this takes place via the ISP.
*/
bool SDHexSession::begin(
	const char*		inPath,
	Stream*			inStream,
	AVRStreamISP*	inAVRStreamISP,
	bool			inSetFusesAndBootloader,
	uint32_t		inTimestamp)
{
	mStream = inStream == nullptr ? &mContextualStream : inStream;
	/*
	*	The AVRStreamISP is saved so that the SPI speed can be adjusted before
	*	and after setting fuses.  Before fuses are set the SPI speed is set low.
	*	This low speed is assumed because the AVR default is to use the internal
	*	RC oscillator resulting in a 1 MHz instruction clock.  After the fuses
	*	are set mAVRStreamISP is used to set the SPI speed to whatever is
	*	reflected in the configuration file.
	*
	*	Because AVR fuse settings are latched when entering programming mode,
	*	programming mode is re-entered in order for the new fuse settings to
	*	take effect.  This is only done if the fuses are actually changed.  In
	*	any case, the low SPI speed is used until after the fuse settings are
	*	verified.
	*/
	mAVRStreamISP = inAVRStreamISP;
	bool	loadingFlash = true;	// Is .hex file
	bool success = mStream != nullptr;
	if (success)
	{
		AVRConfig	avrConfig;
		char		configPath[100];
		size_t		pathLen = strlen(inPath);
		memcpy(configPath, inPath, pathLen);
		strcpy(&configPath[pathLen-3], "txt");
		// Case sensitive test for eep hex file.
		loadingFlash = memcmp(&inPath[pathLen-3], "eep", 3) != 0;
		
		success = avrConfig.ReadFile(configPath);
		if (success)
		{
			memcpy(&mConfig, &avrConfig.Config(), sizeof(SAVRConfig));
			if (inSetFusesAndBootloader)
			{
				/*
				*	There is only a subset of the AVR family supported by this
				*	loader.  If the loader doesn't support setting this specific
				*	AVR's fuses, the lock mask will be zero.
				*	The lock mask is extracted from the avrdude.conf file's
				*	memory "lock" setting for this AVR.  The last byte of the
				*	4 byte instruction must include an 'i', meaning it is the
				*	input byte for the write lock bits instruction, of the
				*	form: 0xAC 0xE0 0x00 <lock byte>.
				*/
				success = mConfig.lockBits[0] != 0 && inAVRStreamISP != nullptr;
				if (success)
				{
					/*
					*	If there is a bootloader THEN
					*	setup the path to point to the bootloader.
					*	Bootloaders are stored in the root /bootloaders folder.
					*	All bootloaders have 'B' as the prefix and the decimal
					*	string value of mConfig.bootloader as the suffix.
					*	Ex: if mConfig.bootloader is 12, the filename is B12, and
					*	the full path would be /bootloaders/B12.hex
					*/
					if (mConfig.bootloader)
					{
						mOperation = eSetFusesAndBootloader;
						strcpy_P(configPath, kBootloaderPathPrefixStr);
						pathLen = strlen(configPath);
						UnixTime::Uint16ToDecStr(mConfig.bootloader, &configPath[pathLen]);
						pathLen = strlen(configPath);
						strcpy_P(&configPath[pathLen], kHexExtensionStr);
						loadingFlash = true;
						success = IntelHexFile::begin(configPath);
						/*
						*	If the bootloader exists THEN
						*	estimate its length.
						*/ 
						if (success)
						{
							mConfig.byteCount = EstimateLength();
						}
					} else
					{
						mOperation = eSetFuses;
					}
					inAVRStreamISP->SetStream(&mContextualStream);
					inAVRStreamISP->SetSPIClock(0);	// Assume 1 MHz fCPU
				} else
				{
					mError = eLockErr;
				}
			} else
			{
				mOperation = loadingFlash ? eProgramFlash : eProgramEEPROM;
				success = IntelHexFile::begin(inPath);
				if (success)
				{
					/*
					*	If the config doesn't contain the byte count THEN
					*	estimate its size from the hex file. The estimation takes
					*	time depending on the size of the hex file.
					*/
					if (mConfig.byteCount == 0)
					{
						mConfig.byteCount = EstimateLength();
					}
				#ifdef __MACH__
					fprintf(stderr, "%d\n", mConfig.byteCount);
				#endif
					if (inAVRStreamISP)
					{
						inAVRStreamISP->SetStream(&mContextualStream);
						inAVRStreamISP->SetAVRConfig(avrConfig.Config());
					}
				}
			}
		}
	}
	if (success)
	{
		mSyncRetries = 0;
		mError = 0;
		mSerialISP = !inAVRStreamISP;
		mCmdHandler = &SDHexSession::SetDevice;
		/*
		*	When using AVRStreamISP, AVRStreamISP manages the reset line.
		*	
		*	If AVRStreamISP isn't being used THEN
		*	manage the reset line here (for Serial1 stream)
		*/
	#ifndef __MACH__
		if (mSerialISP)
		{
			/*
			*	In this case where Serial1 is used, resetting the target MCU is
			*	done by holding DTR/reset low for the duration of the session.
			*
			*	At this point the reset pin/DTR is essentially floating. Change
			*	the pin to output, enable the the buffered 3v3 DTR/reset signal
			*	for possible 3v3 serial use.
			*/
			pinMode(Config::kResetPin, OUTPUT);
			digitalWrite(Config::kResetPin, HIGH);
		#if (HEX_LOADER_VER >= 12)
			digitalWrite(Config::kReset3v3OEPin, LOW);	// The OE pin on the level shifter
		#endif
			delay(1);	// Allow the DTR/reset cap on the target board time to charge.
						// If this isn't done the board may not notice reset going low.
			digitalWrite(Config::kResetPin, LOW);
			/*
			*	avrdude appears to delay 300ms before sending the first bit of
			*	data.  I didn't see anything in the acrdude.conf that implied
			*	this was configurable.
			*/
			delay(300);
		}
	#endif
		/*
		*	Setup the contextual stream even if the stream is serial.
		*/
		mContextualStream.flush();		
		{
			mContextualStream.ReadFrom1(true);
			GetSync(false);	// Load the 1st command for either stream
			mContextualStream.ReadFrom1(false);
		}
		mStage = eVerifySignature;
		// Page variables:
		mBytesPerPage = loadingFlash ? mConfig.flashPageSize : mConfig.eepromPageSize;
		mWordsPerPage = mBytesPerPage >> 1;
		mBytesProcessed = 0;
		mPercentageProcessed = 0;
		mPageAddressMask = (uint32_t)~(mWordsPerPage -1);
		// When loading flash, if the target device capacity is greaterthan 128KB
		// then initializing mCurrentAddressH to 0xFF will generate a Load Extended
		// Address command for extended address 0.
		mCurrentAddressH = (!loadingFlash || (mConfig.devcode < 0xB0)) ? 0 : 0xFF;
		mDataIndex = 0;
		// Initializing mCurrentPageAddress to 0xFFFF will generate the initial Load
		// Address command for the first address.
		mCurrentPageAddress = 0xFFFF;
	#ifdef SUPPORT_REPLACEMENT_DATA
		// See ReplaceData() for an explanation of what replacement data is.
		mReplacementAddress = mConfig.timestamp;
		mReplacementDataIndex = 0;
		mReplacementData[3] = inTimestamp >> 24;
		mReplacementData[2] = inTimestamp >> 16;
		mReplacementData[1] = inTimestamp >> 8;
		mReplacementData[0] = inTimestamp;
	#endif
	}
	return(success);
}

/************************************ Halt ************************************/
bool SDHexSession::Halt(void)
{
	bool haltedSession = mStream != nullptr;
	if (haltedSession)
	{
		mStage = eSessionCompleted;
		mStream = nullptr;
	#ifndef __MACH__
		if (mSerialISP)
		{
			/*
			*	Restore DTR/reset to floating for 3v3 serial, and INPUT_PULLUP
			*	for 5v. Both will allow the capacitor attached to DTR/reset on
			*	the target board to rise to a HIGH state so the target MCU will
			*	boot normally.
			*/
			digitalWrite(Config::kResetPin, HIGH);
			/*
			*	Something funky is happening and I don't understand why. If I
			*	remove the delay before setting the pin mode, any SPI activity
			*	following the call to pinMode will hang the MCU.  The reset pin
			*	is on the same port as the SPI pins. I found this using code
			*	elimination and have no idea why it happens or what the delay
			*	does to keep it from hanging. I noticed that it stopped hanging
			*	when I placed a few debugging print statements to isolate where
			*	it was hanging.  I replaced the print statements with a delay.
			*/
			delay(1);
			pinMode(Config::kResetPin, INPUT_PULLUP);
		#if (HEX_LOADER_VER >= 12)
			digitalWrite(Config::kReset3v3OEPin, HIGH);
		#endif
		}
	#endif
		end();	// Release/close SD file
		mTimeout.Set(0);
	}
	return(haltedSession);
}

/****************************** WaitForAvailable ******************************/
bool SDHexSession::WaitForAvailable(
	uint8_t	inBytesToWaitFor)
{
	if (mSerialISP)
	{
		MSPeriod	timeout((uint32_t)inBytesToWaitFor * 10);
		timeout.Start();
		while (mStream->available() < inBytesToWaitFor)
		{
			if (!timeout.Passed())
			{
				continue;
			}
			return(false);
		}
	}
	return(true);
}

/************************** WaitForAvailableForWrite **************************/
void SDHexSession::WaitForAvailableForWrite(
	uint8_t	inBytesToWaitFor)
{
	if (mSerialISP)
	{
		while (((HardwareSerial*)mStream)->availableForWrite() < inBytesToWaitFor){}
	}
}

/****************************** ResponseStatusOK ******************************/
bool SDHexSession::ResponseStatusOK(void)
{
	
	bool	statusOK = WaitForAvailable(1) &&
						mStream->read() == STK_OK;
	if (!statusOK)
	{
		GetSync(false);
	}
	return(statusOK);
}

/********************************** GetSync ***********************************/
/*
*	Prior to calling GetSync(false), the mCmdHandler should point to the handler
*	to be called after attaining sync.
*/
void SDHexSession::GetSync(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(2);
		mStream->write(STK_GET_SYNC);
		mStream->write(CRC_EOP);
		/*
		*	If not resync...
		*/
		if (mCmdHandler != &SDHexSession::GetSync)
		{
			mOnSyncCmdHandler = mCmdHandler;
			mCmdHandler = &SDHexSession::GetSync;
		}
	} else if (ResponseStatusOK())
	{
		mSyncRetries = 0;
		mCmdHandler = mOnSyncCmdHandler;
		(this->*(mCmdHandler))(false);
	}
}

/********************************* SetDevice **********************************/
/*
*	Most of the parameters of the STK_SET_DEVICE command aren't used by the
*	internal AVRStreamISP.  AVRStreamISP only uses the pagesize and eepromsize.
*	Bootloaders ignore all of these params.
*
*	
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
/********************************* SetDevice **********************************/
void SDHexSession::SetDevice(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(21);
		mStream->write(STK_SET_DEVICE);	// 0x42
		uint8_t	zeroParams[11] = {0};
		mStream->write(mConfig.devcode);			// 0
		mStream->write(zeroParams, 11);				// 1:11
		// These are written out big endian.
		mStream->write(mConfig.flashPageSize >> 8);	// 12
		mStream->write(mConfig.flashPageSize);		// 13
		mStream->write(mConfig.eepromSize >> 8);	// 14
		mStream->write(mConfig.eepromSize);			// 15
		mStream->write(zeroParams, 4);				// 16:19
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::SetDevice;
	} else if (ResponseStatusOK())
	{
		SetDeviceExt(false);
	}
}

/******************************** SetDeviceExt ********************************/
/*
*	Most of the parameters of the STK_SET_DEVICE_EXT command aren't used by the
*	internal AVRStreamISP.  AVRStreamISP only uses the eeprompagesize.
*	Bootloaders ignore all of these params.
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
void SDHexSession::SetDeviceExt(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(7);
		mStream->write(STK_SET_DEVICE_EXT);	// 0x45
		uint8_t	zeroParams[3] = {0};
		mStream->write(5);						// 0
		mStream->write(mConfig.eepromPageSize);	// 1
		mStream->write(zeroParams, 3);			// 2:4
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::SetDeviceExt;
	} else if (ResponseStatusOK())
	{
		EnterProgramMode(false);
	}
}

/****************************** EnterProgramMode ******************************/
void SDHexSession::EnterProgramMode(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(2);
		mStream->write(STK_ENTER_PROGMODE);	// 0x50
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::EnterProgramMode;
	} else if (ResponseStatusOK())
	{
		if (mStage == eVerifySignature)
		{
			ReadSignature(false);
		} else if (mStage == eVerifyUnlocked)
		{
			// This is only done when setting fuses (with or without bootloader.)
			mStageModifier = 0;
			VerifyUnlocked(false);
		} else if (mOperation == eSetFusesAndBootloader)
		{
			// This is only done when loading a bootloader. (called via VerifyFuse)
			// Now that the fuses have been verified, adjust the SPI speed to
			// the max for fCPU.
			mAVRStreamISP->SetAVRConfig(mConfig);
			ProcessPage(false);
		} else // assumed to be mOperation == eSetFuses
		{
			// This is done when just the fuses are being set (no bootloader)
			mStage = eVerifyLockBits;
			mStageModifier = 0;
			VerifyLockBits(false);
		}
	}
}

/****************************** LeaveProgramMode ******************************/
void SDHexSession::LeaveProgramMode(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(2);
		mStream->write(STK_LEAVE_PROGMODE);	// 0x51
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::LeaveProgramMode;
	} else if (ResponseStatusOK())
	{
		Halt();
	}
}

/******************************* ReadSignature ********************************/
void SDHexSession::ReadSignature(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(2);
		mStream->write(STK_READ_SIGN);	// 0x75
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::ReadSignature;
	} else if (WaitForAvailable(4) && mStream->available() > 3)
	{
		for (uint8_t i = 0; i < 3; i++)
		{
			if (mConfig.signature[i] == mStream->read())
			{
				continue;
			}
			mError = eSignatureErr;
			break;
		}
		if (!mError &&
			ResponseStatusOK())
		{
			ChipErase(false);
		}
	}
}

/******************************* SetupUniversal *******************************/
void SDHexSession::SetupUniversal(
	uint8_t	inByte1,
	uint8_t	inByte2,
	uint8_t	inByte3,
	uint8_t	inByte4)
{
	WaitForAvailableForWrite(6);
	mStream->write(STK_UNIVERSAL);	// 0x56
	mStream->write(inByte1);
	mStream->write(inByte2);
	mStream->write(inByte3);
	mStream->write(inByte4);
	mStream->write(CRC_EOP);
}

/********************************* ChipErase **********************************/
/*
*	This will only erase the chip when done through the ISP, not through a
*	bootloader.  Most bootloaders ignore this command and self-erase as needed
*	when writing pages.
*/
void SDHexSession::ChipErase(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		// Chip Erase as per AVR Serial Programming Instruction Set
		SetupUniversal(0xAC, 0x80, 0, 0);
		mCmdHandler = &SDHexSession::ChipErase;
	#ifndef __MACH__
		mCmdDelay.Set(mConfig.chipEraseDelay);
		mCmdDelay.Start();
	#endif
	} else
	{
		if (WaitForAvailable(1))
		{
			mStream->read();	// Skip response
			if (ResponseStatusOK())
			{
				if (mOperation & eIsProgramming)	// Either flash or EEPROM
				{
					mStage = mOperation == eProgramFlash ? eLoadingFlash : eLoadingEEPROM;
					ProcessPage(false);
				} else	// Else, setting fuses, with or without a bootloader
				{
					// Re-enter program mode to latch new lock bits which should
					// have been cleared (i.e. set to 0xFF) after chip erase.
					mStage = eVerifyUnlocked;
					EnterProgramMode(false);
				}
			}
		}
	}
}

/******************************* VerifyUnlocked *******************************/
void SDHexSession::VerifyUnlocked(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		// Read the lock bits as per AVR Serial Programming Instruction Set
		SetupUniversal(0x58, 0, 0, 0);
		mCmdHandler = &SDHexSession::VerifyUnlocked;
	} else
	{
		if (WaitForAvailable(1))
		{
			uint8_t	lockBits = mStream->read() & mConfig.lockBits[SAVRConfig::eMask];
			if (ResponseStatusOK())
			{
				if (lockBits == mConfig.lockBits[SAVRConfig::eUnlock])
				{
					/*
					*	If the 2nd verification passed THEN
					*	move to the next stage.
					*/
					if (mStageModifier & eFuseVerified)
					{
						mStage = eVerifyExtendedFuse;
						mStageModifier = 0;
						mFuseInst = kFuseInst[0];
						VerifyFuse(false);
					} else
					{
						mStageModifier |= eFuseVerified;
						VerifyUnlocked(false);
					}
				} else
				{
					mError = eUnlockErr;
				}
			}
		}
	}
}

/********************************* VerifyFuse *********************************/
/*
*	Handler used to verify the extended, high and low fuses, in that order.
*	If first verification fails an attempt is made to set the fuse (only once.)
*	If subsequent verifications fail the session fails with a fuse error.
*	This mimics the avrdude behavior.
*/
void SDHexSession::VerifyFuse(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		// Read the fuse as per AVR Serial Programming Instruction Set
		SetupUniversal(mFuseInst.readInstByte1, mFuseInst.readInstByte2, 0, 0);
		mCmdHandler = &SDHexSession::VerifyFuse;
	#ifndef __MACH__
		// Avrdude delays after each universal command sent.
		// When this delay is removed, the set transaction below fails.
		mCmdDelay.Set(mConfig.lockMinWriteDelay);
		mCmdDelay.Start();
	#endif
	} else
	{
		if (WaitForAvailable(1))
		{
			uint8_t	fuseVal = mStream->read();
			if (ResponseStatusOK())
			{
				if (mStageModifier & eFuseWriteResponse)
				{
					mStageModifier &= ~eFuseWriteResponse;
					VerifyFuse(false);
				// mStage is assumed to be one of eVerifyXXXFuse, so subtracting
				// eVerifyFuse results in an index 0 to 2
				} else if (fuseVal == mConfig.fuses[mStage - eVerifyFuse])
				{
					/*
					*	If the 2nd verification passed THEN
					*	move to the next stage.
					*/
					if (mStageModifier & eFuseVerified)
					{
						if (mStage < eVerifyLowFuse)
						{
							mStage++;
							mStageModifier = 0;
							mFuseInst = kFuseInst[mStage - eVerifyFuse];
							VerifyFuse(false);
						} else
						{
							mStage = eLoadingFlash;
							EnterProgramMode(false);
						}
					} else
					{
						mStageModifier = eFuseVerified;
						VerifyFuse(false);
					}
				/*
				*	Else, if there was already a write attempt OR
				*	the 2nd verification just failed THEN
				*	fail.
				*/
				} else if (mStageModifier)
				{
					mError = eFuseErr + (mStage - eVerifyFuse);
				/*
				*	Else, attempt to write the fuse value.
				*/
				} else
				{
					mStageModifier = eFuseWriteResponse + eFuseWritten;
					SetupUniversal(0xAC, mFuseInst.writeInstByte2, 0, mConfig.fuses[mStage - eVerifyFuse]);
				#ifndef __MACH__
					mCmdDelay.Set(mConfig.lockMinWriteDelay);
					mCmdDelay.Start();
				#endif
				}
			}
		}
	}
}

/******************************* VerifyLockBits *******************************/
/*
*	Handler used to verify the lock bits.
*	If first verification fails an attempt is made to set the lock bits (only once.)
*	If subsequent verifications fail the session fails with a lock error.
*	This mimics the avrdude behavior.
*/
void SDHexSession::VerifyLockBits(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		// Read the lock bits as per AVR Serial Programming Instruction Set
		SetupUniversal(0x58, 0, 0, 0);
		mCmdHandler = &SDHexSession::VerifyLockBits;
	#ifndef __MACH__
		// Avrdude delays after each universal command sent.
		// When this delay is removed, the set transaction below may fail.
		mCmdDelay.Set(mConfig.lockMinWriteDelay);
		mCmdDelay.Start();
	#endif
	} else
	{
		if (WaitForAvailable(1))
		{
			uint8_t	lockBits = mStream->read() & mConfig.lockBits[SAVRConfig::eMask];
			if (ResponseStatusOK())
			{
				if (mStageModifier & eFuseWriteResponse)
				{
					mStageModifier &= ~eFuseWriteResponse;
					VerifyLockBits(false);
				} else if (lockBits == mConfig.lockBits[SAVRConfig::eLock])
				{
					/*
					*	If the 2nd verification passed THEN
					*	we're done.
					*/
					if (mStageModifier & eFuseVerified)
					{
						LeaveProgramMode(false);
					} else
					{
						mStageModifier = eFuseVerified;
						VerifyLockBits(false);
					}
				/*
				*	Else, if there was already a write attempt OR
				*	the 2nd verification just failed THEN
				*	fail.
				*/
				} else if (mStageModifier)
				{
					mError = eLockErr;
				/*
				*	Else, attempt to write the lockBits.
				*/
				} else
				{
					mStageModifier = eFuseWriteResponse + eFuseWritten;
					// As per AVR docs, unused lock bits should be set. ORing
					// the lock value with the ~mask satisfies this requirement.
					SetupUniversal(0xAC, 0xE0, 0,
										(~mConfig.lockBits[SAVRConfig::eMask]) |
											mConfig.lockBits[SAVRConfig::eLock]);
				#ifndef __MACH__
					mCmdDelay.Set(mConfig.lockMinWriteDelay);
					mCmdDelay.Start();
				#endif
				}
			}
		}
	}
}

/******************************** LoadAddress *********************************/
void SDHexSession::LoadAddress(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		WaitForAvailableForWrite(4);
		mStream->write(STK_LOAD_ADDRESS);	// 0x55
		mStream->write((uint8_t)mCurrentPageAddress);
		mStream->write((uint8_t)(mCurrentPageAddress >> 8));
		mStream->write(CRC_EOP);
		mCmdHandler = &SDHexSession::LoadAddress;
	} else if (ResponseStatusOK())
	{
		ProcessPage(false);
	}
}

/******************************* LoadExtAddress *******************************/
void SDHexSession::LoadExtAddress(
	bool	inIsResponse)
{
	if (!inIsResponse)
	{
		// Load Extended Address byte as per AVR Serial Programming Instruction Set
		SetupUniversal(0x4D, 0, mCurrentAddressH, 0);
		mCmdHandler = &SDHexSession::LoadExtAddress;
	} else
	{
		if (WaitForAvailable(1))
		{
			mStream->read();	// Skip response
			if (ResponseStatusOK())
			{
				ProcessPage(false);
			}
		}
	}
}

#ifdef SUPPORT_REPLACEMENT_DATA
/******************************** ReplaceData *********************************/
/*
*	My DCSensor.ino bases the unique ID used on the CAN bus on the timestamp of
*	when it was compiled.  There are several DCSensor boards on the bus, and
*	when updating the software, each ID needs to be unique.  This requirement is
*	satisified when using the Arduino IDE to upload the sketch.  The code below
*	is a solution when SDHexSession is used to load the sketch on several boards
*	with the same SD hex file. The SDHexSession::begin() function is passed the
*	unix uint32_t timestamp as read from the loader's RTC.  This 32 bit value is
*	divided up into 4 bytes placed in mReplacementData.  A new timestamp is
*	passed each time a DCSensor board is loaded.
*
*	This proof of concept only targets sketches named DCSensor.ino that have a
*	global variable named kTimestamp.  The address of kTimestamp within the
*	DCSensor.ino binary is determined by the HexLoaderUtility application by
*	reading the DCSensor.ino.elf file that's generated whenever a sketch is
*	compiled by the Arduino IDE.  This address is written to the configuration
*	file DCSensor.ino.txt with the key "timestamp".
*/
void SDHexSession::ReplaceData(void)
{
	if (mReplacementAddress &&
		mAddress <= mReplacementAddress &&
		(mAddress + mByteCount) > mReplacementAddress)
	{
		// The code below allows for replacing the 4 bytes across an Intel
		// Hex record boundary (as needed.)
		uint16_t	dataIndex = mReplacementAddress - mAddress;
		uint16_t	rIndex = mReplacementDataIndex;
		while (rIndex < 4 && dataIndex < mByteCount)
		{
			mData[dataIndex++] = mReplacementData[rIndex++];
		}
		if (rIndex < 4)
		{
			mReplacementAddress += (rIndex - mReplacementDataIndex);
			mReplacementDataIndex = rIndex;
		} else
		{
			mReplacementAddress = 0;
		}
	}
}
#endif

/***************************** LoadNextDataRecord *****************************/
/*
*	Bottleneck for loading data records.
*/
bool SDHexSession::LoadNextDataRecord(void)
{
	bool success;
	while ((success = NextRecord()) && mRecordType > eEndOfFileRecord){}
	if (success)
	{
		mDataIndex = 0;
#ifdef SUPPORT_REPLACEMENT_DATA
		ReplaceData();
#endif
	} else
	{
		mError = eLoadHexDataErr;
	}
	return(success);
}

/******************************** ProcessPage *********************************/
/*
*	This routine handles the page access data stream, both for verifying and
*	loading.
*
*	EEPROM: Note that even for eeprom this is PAGE access, so full page
*	blocks will be used when read from the eep file.  Attempting to write a
*	non-aligned odd length eep will overwrite existing eeprom data.  The
*	AVRStreamISP does support non-aligned odd length eeprom data because it
*	checks alignment and length to determine the most efficient method of
*	writing the eeprom data for the specific target MCU based on the eeprom page
*	size.
*/
void SDHexSession::ProcessPage(
	bool	inIsResponse)
{
	if (mDataIndex == mByteCount &&
		mRecordType != eEndOfFileRecord &&
		!LoadNextDataRecord())
	{
		return;	// Fail
	}

	if (mByteCount)
	{
		uint32_t	wordAddress = (Address32() + mDataIndex) >> 1;
		uint32_t	pageAddress = wordAddress & mPageAddressMask;
		uint32_t	nextPageAddress = pageAddress + mWordsPerPage;
		/*
		*	The code below assumes 2^1 alignment even though the data section is
		*	2^0 aligned.  I checked several hex files and tried to force an odd
		*	alignment. I always ended up with an even number of bytes.  If the
		*	assumption that there will always be an even number of bytes is
		*	wrong, the code below needs to be modified to allow for this.
		*/
		if (!inIsResponse)
		{
			/*
			*	Extended address support
			*/
			{
				bool	highAddressChanged = mCurrentAddressH != (mAddressH >> 1);
				/*
				*	When the high address changes then the current page needs to be
				*	completed before issuing the STK_UNIVERSAL command to change the
				*	upper address.
				*/
				if (highAddressChanged)
				{
					mCurrentAddressH = mAddressH >> 1;
					LoadExtAddress(false);
					return;	// Send command
				}
			}
	
			/*
			*	Send load address command if needed
			*/
			{// Testing for inIsResponse == false would probably also work here
				if (pageAddress != mCurrentPageAddress)
				{
					mCurrentPageAddress = pageAddress;
					LoadAddress(false);
					return;	// Send command
				}
			}
			/*
			*	When writing via SPI in page mode, only full pages should be
			*	sent otherwise the result is undefined.
			*
			*	Future optimization - It would be faster to verify immediately
			*	after loading each page.  That way the SD would only need to be
			*	read once.  There's no reason to load everything, then verify
			*	everything.
			*/
			WaitForAvailableForWrite(4);
			mStream->write(mStage & eLoadingMemory ? STK_PROG_PAGE : STK_READ_PAGE);	// 0x64 : 0x74
			mStream->write((uint8_t)(mBytesPerPage>>8));
			mStream->write((uint8_t)(mBytesPerPage));
			mBytesProcessed+=mBytesPerPage;
			mPercentageProcessed = (mBytesProcessed*100)/mConfig.byteCount;
			if (mPercentageProcessed > 100)
			{
				mPercentageProcessed = 100;
			}
			mStream->write((mStage & eIsFlash) ? 'F' : 'E');
			mCmdHandler = &SDHexSession::ProcessPage;
			/*
			*	If loading Flash or EEPROM THEN
			*	load the stream with the page data.
			*/
			if (mStage & eLoadingMemory)
			{
				if (!LoadPageFromSD(wordAddress, pageAddress, nextPageAddress, mStream))
				{
					return;	// Fail
				}
			#ifndef __MACH__
				mCmdDelay.Set(mStage == eLoadingFlash ? mConfig.flashMinWriteDelay : mConfig.eepromMinWriteDelay);
				mCmdDelay.Start();
			#endif
			}
			WaitForAvailableForWrite(1);
			mStream->write(CRC_EOP);
		} else
		{
			/*
			*	If verifying Flash or EEPROM THEN
			*	compare the stream with the page data returned.
			*/
			if (mStage & eVerifyingMemory)
			{
				/*
				*	The contextual stream should be read-from-1 / write-to-2
				*	at this point, regardless of the ISP used.
				*	The call to LoadPageFromSD will therefore write to buffer 2.
				*	Buffer 2 should be empty at this point so it's available as
				*	a general use stream so long as a context switch isn't made.
				*/ 
				if (!LoadPageFromSD(wordAddress, pageAddress, nextPageAddress, &mContextualStream))
				{
					return;	// Fail
				}
					uint8_t*	sdData = mContextualStream.Buffer2();
					
					uint16_t bytes2cmp = mBytesPerPage;
					for (uint16_t i = 0; i < bytes2cmp; i++)
					{
						if (WaitForAvailable(1) &&
							mStream->read() == sdData[i])
						{
							continue;
						}
						mError = eVerificationErr;
						return;
					}
					mContextualStream.FlushBuffer2();
			}
			/*
			*	If response was terminated with the expected OK status THEN
			*	recursively call this routine again.  This won't turn into a
			*	runaway loop because the ProcessPage(false) case always returns
			*	to the main loop.
			*/
			if (ResponseStatusOK())
			{
				ProcessPage(false);
			}
		}
	} else if (mRecordType == eEndOfFileRecord)
	{
		if (mStage & eLoadingMemory)
		{
			if (ResponseStatusOK())
			{
				Rewind();
				mStage += eLoadingMemory;	// Change from "Loading" to "Verifying"
				mCurrentPageAddress = 0xFFFF;
				mBytesProcessed = 0;
				mPercentageProcessed = 0;
				/*
				*	Because the Serial1 Rx buffer is only 64 bytes, and there's
				*	no clean way of increasing the size without editing the core
				*	sources (which affects all sketches/Serial instances, and
				*	would be a pain to maintain), the requested read size for
				*	verification is reduced to a size that won't overrun Rx.
				*/
				if (mSerialISP)
				{
					mWordsPerPage = 16;
					mBytesPerPage = 32;
					mPageAddressMask = (uint32_t)~(16 -1);
				}
				mCurrentAddressH = mConfig.devcode < 0xB0 ? 0 : 0xFF;
			#ifdef SUPPORT_REPLACEMENT_DATA
				mReplacementAddress = mConfig.timestamp;
				mReplacementDataIndex = 0;
			#endif
				ProcessPage(false);
			}
		} else if (mOperation & eIsProgramming)
		{
			LeaveProgramMode(false);
		} else
		{
			mStage = eVerifyLockBits;
			mStageModifier = 0;
			VerifyLockBits(false);
		}
	}
}

/******************************* LoadPageFromSD *******************************/
bool SDHexSession::LoadPageFromSD(
	uint32_t	inWordAddress,
	uint32_t	inPageAddress,
	uint32_t	inNextPageAddress,
	Stream*		inStream)
{
	/*
	*	avrdude always loads full blocks even when there
	*	isn't enough hex data.  This mimics that behavior.
	*/
	if (inPageAddress < inWordAddress)
	{
		for (uint16_t i = (inWordAddress-inPageAddress) << 1; i; i--)
		{
			WaitForAvailableForWrite(1);
			inStream->write(0xFF);
		}
	}
	while (inWordAddress < inNextPageAddress)
	{
		uint16_t	wordsInData = (mByteCount - mDataIndex) >> 1;
		if ((inWordAddress + wordsInData) > inNextPageAddress)
		{
			wordsInData = inNextPageAddress - inWordAddress;
		}
		{
			uint16_t i = mDataIndex;
			uint16_t endIndex = i + (wordsInData << 1);
			while (i < endIndex)
			{
				WaitForAvailableForWrite(1);
				inStream->write(mData[i++]);
			}
			mDataIndex = i;
		}
		inWordAddress += wordsInData;

		if (inWordAddress < inNextPageAddress)
		{
			if (!LoadNextDataRecord())
			{
				return(false);	// Fail
			}
			/*
			*	If the page changed after reading the next line ||
			*	the high address changed THEN
			*	pad the rest of the current page.
			*/
			if (((Address32() >> 1) & mPageAddressMask) != inPageAddress ||
					mCurrentAddressH != (mAddressH >> 1))
			{
				for (uint16_t i = (inNextPageAddress - inWordAddress) << 1; i; i--)
				{
					WaitForAvailableForWrite(1);
					inStream->write(0xFF);
				}
				inWordAddress = inNextPageAddress;
				break;
			}
		}
	}
	return(true);
}

/*********************************** Update ***********************************/
bool SDHexSession::Update(void)
{
	/*
	*	If there's an active session AND
	*	there are no errors
	*/
	if (mStream &&
		CanContinue())
	{
		// StReadFrom1 is defined in ContextualStream.h
		StReadFrom1	readFrom1(mContextualStream, true);
	#ifndef __MACH__
		if (mCmdDelay.Get())
		{
			// Most bootloaders self delay, so the call to Delay does nothing.
			mCmdDelay.Delay();
			mCmdDelay.Set(0);
		}
	#endif
		/*
		*	If there is a response available...
		*/
		if (mStream->available())
		{
		#ifndef __MACH__
			mTimeout.Set(0);	// Cancel timeout timer.
		#endif
			uint8_t	response = mStream->read();
			if (response == STK_INSYNC)			// 0x14
			{
				(this->*(mCmdHandler))(true);
			} else if (response == STK_NOSYNC/* ||
				response == STK_OK*/)
			{
				if (mSyncRetries < 4)
				{
					mSyncRetries++;
					GetSync(false);
				} else
				{
					mError = eSyncErr;
				}
			
			} else if (response != 0)	// If not a frame error
			{
			#if 0
				Serial.print(response, HEX);
				Serial.print(' ');
				Serial.print(mStream->available());
				Serial.print('s');
				while(mStream->available())
				{
					Serial.print(' ');
					Serial.print(mStream->read(), HEX);
				}
			#endif
				mError = eUnknownErr;
			}
		/*
		*	Else if there hasn't been a response for kSessionTimeout ms THEN
		*	quit the session and flag the timeout error.
		*/
	#ifndef __MACH__
		} else if (mTimeout.Passed())
		{
			mError = eTimeoutErr;
		/*
		*	Else start the timeout timer if not yet started
		*/
		} else if (mTimeout.Get() == 0)
		{
			mTimeout.Set(kSessionTimeout);
			mTimeout.Start();
	#endif
		}
	}
	return(CanContinue());
}

