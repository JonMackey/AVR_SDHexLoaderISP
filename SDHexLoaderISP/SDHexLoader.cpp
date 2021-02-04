/*
*	SDHexLoader.cpp, Copyright Jonathan Mackey 2020
*	Handles input from the UI.
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
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#include "SDHexLoader.h"
#include "SerialUtils.h"
#include "ATmega644RTC.h"
#include "AVRConfig.h"

bool SDHexLoader::sButtonPressed;
bool SDHexLoader::sSDInsertedOrRemoved;


const char kSetTimeLStr[] PROGMEM = "Set Time";
const char kSleepStr[] PROGMEM = "Sleep: ";			// 98px
const char kEnabledStr[] PROGMEM = "Enabled";
const char kDisabledStr[] PROGMEM = "Disabled";
const char kNoMessageStr[] PROGMEM = " ";

const char kSourceStr[] PROGMEM = "Source: ";		// 119px
const char kUSBStr[] PROGMEM = "USB";
const char kSDStr[] PROGMEM = "SD";
const char kSDBLStr[] PROGMEM = "SD BL";

const char kInsertSDCardStr[] PROGMEM = "Insert SD Card";
const char kNoHexFilesStr[] PROGMEM = "No hex files";

const char kWritingStr[] PROGMEM = "Writing ";		// 118px
const char kVerifyingStr[] PROGMEM = "Verifying ";	// 143px
const char kPassThroughStr[] PROGMEM = "Pass through...";

const char kISPStr[] PROGMEM = "ISP: ";			// 82px
const char kForceOnStr[] PROGMEM = "Force on";
const char kAutoStr[] PROGMEM = "Auto";

const char kClockStr[] PROGMEM = "Clock: ";
const char kMHzStr[] PROGMEM = " MHz";

const char kSuccessStr[] PROGMEM = "Success!";
const char kErrorNumStr[] PROGMEM = "Error: ";			// 89px

//const char kYesStr[] PROGMEM = "Yes";
//const char kNoStr[] PROGMEM = "No";

const char kStartISPStr[] PROGMEM = "Start ISP";
const char kStartSerialStr[] PROGMEM = "Start Serial";
const char kStopStr[] PROGMEM = "Stop";

const char kOKStr[] PROGMEM = "OK";

const char kSDCardErrorStr[] PROGMEM = "SD card error";
const char kFileOpenErrorStr[] PROGMEM = "File open error";
const char kInternalISPStr[] PROGMEM = "Internal ISP";
const char kSDSessionStr[] PROGMEM = "SD Session";

const char kTimeoutErrorStr[] PROGMEM = "Timeout error";
const char kSyncErrorStr[] PROGMEM = "Sync error";
const char kUnknownErrorStr[] PROGMEM = "Unknown error";
const char kHexDataErrorStr[] PROGMEM = "Hex data error";
const char kSignatureErrorStr[] PROGMEM = "Signature error";
const char kVerifyFailedStr[] PROGMEM = "Verify failed";
const char kUnlockErrorStr[] PROGMEM = "Unlock error";
const char kLockErrorStr[] PROGMEM = "Lock error";
const char kEFuseErrorStr[] PROGMEM = "EFuse error";
const char kHFuseErrorStr[] PROGMEM = "HFuse error";
const char kLFuseErrorStr[] PROGMEM = "LFuse error";

struct SStringDesc
{
	const char*	descStr;
	uint16_t	color;
};

const SStringDesc kTextDesc[] PROGMEM =
{
	{kNoMessageStr, XFont::eWhite},
	{kOKStr, XFont::eWhite},
	// Error Messages
	{kSDCardErrorStr, XFont::eWhite},
	{kFileOpenErrorStr, XFont::eWhite},
	{kInternalISPStr, XFont::eWhite},
	{kSDSessionStr, XFont::eWhite},
	// SD Session errors must be in the same order as SDHexSession::EErrors
	{kTimeoutErrorStr, XFont::eYellow},
	{kSyncErrorStr, XFont::eYellow},
	{kUnknownErrorStr, XFont::eWhite},
	{kHexDataErrorStr, XFont::eRed},
	{kSignatureErrorStr, XFont::eRed},
	{kVerifyFailedStr, XFont::eRed},
	{kUnlockErrorStr, XFont::eRed},
	{kLockErrorStr, XFont::eRed},
	{kEFuseErrorStr, XFont::eRed},
	{kHFuseErrorStr, XFont::eRed},
	{kLFuseErrorStr, XFont::eRed},
	
	{kSuccessStr, XFont::eWhite},
//	{kYesStr, XFont::eGreen},
//	{kNoStr, XFont::eRed},
	{kErrorNumStr, XFont::eWhite}
};

#define DEBOUNCE_DELAY		20		// ms
/********************************* SDHexLoader **********************************/
SDHexLoader::SDHexLoader(void)
:  mDebouncePeriod(DEBOUNCE_DELAY)
{

}

/*********************************** begin ************************************/
void SDHexLoader::begin(
	DisplayController*	inDisplay,
	Font*				inNormalFont,
	Font*				inSmallFont)
{
	Serial1.begin(BAUD_RATE);
	pinMode(Config::kUpBtnPin, INPUT_PULLUP);
	pinMode(Config::kLeftBtnPin, INPUT_PULLUP);
	pinMode(Config::kEnterBtnPin, INPUT_PULLUP);
	pinMode(Config::kRightBtnPin, INPUT_PULLUP);
	pinMode(Config::kDownBtnPin, INPUT_PULLUP);
	
	pinMode(Config::kSDDetectPin, INPUT_PULLUP);
	pinMode(Config::kSDSelectPin, OUTPUT);
	digitalWrite(Config::kSDSelectPin, HIGH);	// Deselect the SD card.

	cli();						// Disable interrupts
	/*
	*	To respond to button presses and the interrupt line from an SD Card
	*	being inserted, setup pin change interrupts for the associated pins. All
	*	of the button pins are on the same port.
	*/
	PCMSK0 = _BV(PCINT3) | _BV(PCINT4) | _BV(PCINT5) | _BV(PCINT6) | _BV(PCINT7);
	PCMSK3 = _BV(PCINT29);	// PD5 SD Card Inserted
	PCICR = _BV(PCIE0) | _BV(PCIE3);
	sei();					// Enable interrupts

	mNormalFont = inNormalFont;
	mSmallFont = inSmallFont;
	SetDisplay(inDisplay, inNormalFont);
	mUnixTimeEditor.Initialize(this);
	mPrevMode = 99;
	mAVRStreamISP.begin();

	sSDInsertedOrRemoved = true;
	mSDCardPresent = false;	// This will be updated on the first call to Update if present.
	mInSession = eIdle;
	mMode = 99;	// Forces GoToMainMode to setup mode vars
	mSource = eUSBSource;
	GoToMainMode();
	ShowSelectionFrame();
	{
		uint8_t	flags;
		EEPROM.get(Config::kFlagsAddr, flags);
		mSleepEnabled = (flags & _BV(Config::kEnableSleepBit)) != 0;
		mOnlyUseISP = (flags & _BV(Config::kOnlyUseISPBit)) == 0;
		EEPROM.get(Config::kISP_SPIClockAddr, mISPClockIndex);
		if (mISPClockIndex > 5) mISPClockIndex = 0;
	}
}

/*************************** LeftRightButtonPressed ***************************/
void SDHexLoader::LeftRightButtonPressed(
	bool	inIncrement)
{
	switch (mMode)
	{
		case eMainMode:
			if (mCurrentFieldOrItem == eSourceItem)
			{
				if (!mInSession)
				{
					if (inIncrement)
					{
						if (mSource < eSDBLSource)
						{
							mSource++;
						} else
						{
							mSource = eUSBSource;
						}
					} else if (mSource > eUSBSource)
					{
						mSource--;
					} else
					{
						mSource = eSDBLSource;
					}
					mMaxMainModeItem = mSource != eUSBSource ? eFilenameItem : eStartStopItem;
					mPrevHexFileIndex = 0xFFFF;	// Force hex file line to redraw
				}
			} else if (mCurrentFieldOrItem == eFilenameItem &&
				!mInSession)
			{
				LoadNextHexFilename(inIncrement);	// Does nothing if there are no hex files on SD
			}
			break;
		case eSettingsMode:
			switch(mCurrentFieldOrItem)
			{
				case eEnableSleepItem:
				{
					mSleepEnabled = !mSleepEnabled;
					uint8_t	flags;
					EEPROM.get(Config::kFlagsAddr, flags);
					if (mSleepEnabled)
					{
						// 1 = Enable
						flags |= _BV(Config::kEnableSleepBit);
					} else
					{
						// 0 = Disable
						flags &= ~_BV(Config::kEnableSleepBit);
					}
					EEPROM.put(Config::kFlagsAddr, flags);
					break;
				}
				case eISPItem:
					if (!mInSession)
					{
						mOnlyUseISP = !mOnlyUseISP;
						uint8_t	flags;
						EEPROM.get(Config::kFlagsAddr, flags);
						if (mOnlyUseISP)
						{
							// 0 = force ISP
							flags &= ~_BV(Config::kOnlyUseISPBit);
						} else
						{
							// 1 = Auto based on mUploadSpeed
							flags |= _BV(Config::kOnlyUseISPBit);
						}
						EEPROM.put(Config::kFlagsAddr, flags);
					}
					break;
				case eClockItem:
					// clock index * 4 MHz, where 0 is 1 MHz
					// index  0  1  2  3   4   5
					// Clocks 1, 4, 8, 12, 16, 20 MHz
					if (inIncrement)
					{
						if (mISPClockIndex < 5)
						{
							mISPClockIndex++;
						} else
						{
							mISPClockIndex = 0;
						}
					} else if (mISPClockIndex > 0)
					{
						mISPClockIndex--;
					} else
					{
						mISPClockIndex = 5;
					}
					EEPROM.put(Config::kISP_SPIClockAddr, mISPClockIndex);
					break;
			}
			break;
		case eSetTimeMode:
			mUnixTimeEditor.LeftRightButtonPressed(inIncrement);
			break;
	}
}

/**************************** UpDownButtonPressed *****************************/
void SDHexLoader::UpDownButtonPressed(
	bool	inIncrement)
{
	switch (mMode)
	{
		case eMainMode:
			if (inIncrement)
			{
				if ((mSource == eUSBSource || mHexFileIndex != 0) &&
					mCurrentFieldOrItem < mMaxMainModeItem)
				{
					mCurrentFieldOrItem++;
				} else
				{
					mCurrentFieldOrItem = eSourceItem;
				}
			} else if (mCurrentFieldOrItem > eSourceItem)
			{
				mCurrentFieldOrItem--;
			} else
			{
				mMode = eSettingsMode;
				mCurrentFieldOrItem = eSetTimeItem;
			}
			break;
		case eSettingsMode:
			if (inIncrement)
			{
				if (mCurrentFieldOrItem < eClockItem)
				{
					mCurrentFieldOrItem++;
				} else
				{
					mCurrentFieldOrItem = eSetTimeItem;
				}
			} else if (mCurrentFieldOrItem > eSetTimeItem)
			{
				mCurrentFieldOrItem--;
			} else
			{
				mMode = eMainMode;
				mCurrentFieldOrItem = eSourceItem;
			}
			break;
		case eSetTimeMode:
			mUnixTimeEditor.UpDownButtonPressed(!inIncrement);
			break;
	}
}

/******************************** EnterPressed ********************************/
void SDHexLoader::EnterPressed(void)
{
	switch (mMode)
	{
		case eMainMode:
			/*
			*	Main mode only responds to enter for start/stop
			*/
			if (mCurrentFieldOrItem == eStartStopItem)
			{
				mTargetIsISP = true;
				if (mInSession)
				{
					mAVRStreamISP.Halt();	// Does nothing if not target
					mSDHexSession.Halt();	// Does nothing if not source
					mInSession = eIdle;
					mPrevHexFileIndex = 0xFFFF;	// Force the filename to redraw (if SD source)
					mPrevSource = eUSBSource;	// Force the source to redraw (if SD source)
				} else if (mSource != eUSBSource)
				{
					/*
					*	The stored mFilename is truncated.  Filenames > 50 bytes
					*	or contain multibyte UTF8 chars are skipped and won't be
					*	listed (meaning it won't get here.)
					*
					*	Get the untruncated filename using the file index.
					*/
					FatFile		hexFile;
					mInSession = hexFile.open(mSD.vwd(), mHexFileIndex, O_RDONLY);
					if (mInSession)
					{
						char hexFilename[52];
						hexFile.getName(hexFilename, 51);
						hexFile.close();

						if (mOnlyUseISP ||
							mUploadSpeed == 0 ||
							mSource == eSDBLSource)
						{
							// nullptr means "use contextual stream"
							mInSession = mSDHexSession.begin(hexFilename, nullptr,
												&mAVRStreamISP, mSource == eSDBLSource,
													UnixTime::Time());
						} else
						{
							Serial1.end();
							Serial1.begin(mUploadSpeed);
							// nullptr means "using HardwareSerial USB stream"
							mInSession = mSDHexSession.begin(hexFilename, &Serial1,
												nullptr, UnixTime::Time());
							mTargetIsISP = false;
						}
					}
					if (mInSession)
					{
						mPrevHexFileIndex = 0xFFFF;	// Force the filename to redraw
						mPrevSource = eUSBSource;	// Force the source to redraw
					} else
					{
						mAVRStreamISP.Halt();	// Does nothing if not target
						mSDHexSession.Halt();
						// File open error
						QueueMessage(eFileOpenErrorDesc,
							eNoMessage, eMainMode, eSourceItem);
					}
				} else
				{
					mAVRStreamISP.SetStream(&Serial);
					mAVRStreamISP.SetSPIClock(((uint32_t)mISPClockIndex)*4000000);
					mInSession = ePassThrough;
				}
				mMaxMainModeItem = (mSource != eUSBSource && !mInSession) ? eFilenameItem : eStartStopItem;
			}
			break;
		case eSettingsMode:
			if (mCurrentFieldOrItem == eSetTimeItem)
			{
				mMode = eSetTimeMode;
				mUnixTimeEditor.SetTime(UnixTime::Time());
			}
			break;
		case eSetTimeMode:
			// If enter was pressed on SET or CANCEL
			if (mUnixTimeEditor.EnterPressed())
			{
				if (!mUnixTimeEditor.CancelIsSelected())
				{
					bool	isFormat24Hour;
					time32_t time = mUnixTimeEditor.GetTime(isFormat24Hour);
					UnixTime::SetTime(time);
					if (UnixTime::Format24Hour() != isFormat24Hour)
					{
						UnixTime::SetFormat24Hour(isFormat24Hour);
						uint8_t	flags;
						EEPROM.get(Config::kFlagsAddr, flags);
						if (isFormat24Hour)
						{
							// 0 = 24 hour
							flags &= ~_BV(Config::k12HourClockBit);
						} else
						{
							// 1 = 12 hour (default for new/erased EEPROMs)
							flags |= _BV(Config::k12HourClockBit);
						}
						EEPROM.put(Config::kFlagsAddr, flags);
					}
				}
				mMode = eSettingsMode;
				mCurrentFieldOrItem = eSetTimeItem;
			}
			break;
		case eMessageMode:
			if (mMessageReturnMode == eSettingsMode)
			{
				GoToMainMode();
			} else
			{
				mMode = mMessageReturnMode;
				mCurrentFieldOrItem = mMessageReturnItem;
			}
			break;
	}
}

/******************************** QueueMessage ********************************/
// Not a queue.  Only one message at a time is supported.
void SDHexLoader::QueueMessage(
	uint8_t	inMessageLine0,
	uint8_t	inMessageLine1,
	uint8_t	inReturnMode,
	uint8_t	inReturnItem)
{
	mMessageLine0 = inMessageLine0;
	mMessageLine1 = inMessageLine1;
	mMode = eMessageMode;
	mMessageReturnMode = inReturnMode;
	mMessageReturnItem = inReturnItem;
}

/*********************************** Update ***********************************/
/*
*	Called from loop()
*/
void SDHexLoader::Update(void)
{
	UpdateDisplay();
	UpdateActions();
	/*
	*	If there is an active session...
	*/
	if (mInSession)
	{
		/*
		*	If the target is ISP THEN
		*	give time to the AVRStreamISP.
		*/
		if (mTargetIsISP)
		{
			/*
			*	If there is an ISP fatal error...
			*/
			if (!mAVRStreamISP.Update())
			{
				mInSession = eIdle;
				mError = mAVRStreamISP.Error();
				QueueMessage(eInternalISPDesc, eErrorNumDesc, eMainMode, eSourceItem);
				if (mSource != eUSBSource)
				{
					mSDHexSession.Halt();
				}
				UnixTime::ResetSleepTime();
			}
		}
		if (mInSession &&
			mSource != eUSBSource)
		{
			/*
			*	If the session is still in progress THEN
			*	update the session state.
			*/
			if (mSDHexSession.Update())
			{
				mInSession = (mSDHexSession.Stage() & SDHexSession::eVerifyingMemory) ? eVerifying : eWriting;
			/*
			*	Else the session finished or there was an error...
			*/
			} else
			{
				mError = mSDHexSession.Error();
				if (mError)
				{
					QueueMessage(eSDSessionDesc, mError + eSDSessionDesc, eMainMode, eSourceItem);
				} else
				{
					QueueMessage(eSuccessDesc, eNoMessage, eMainMode, eSourceItem);
				}
				mInSession = eIdle;
				mPrevHexFileIndex = 0xFFFF;	// Force the filename to redraw
				mPrevSource = eUSBSource;	// Force the source to redraw
				mSDHexSession.Halt();
				mAVRStreamISP.Halt();
				UnixTime::ResetSleepTime();
				mMaxMainModeItem = eFilenameItem;
			}
		}
	}
}

/******************************* UpdateActions ********************************/
void SDHexLoader::UpdateActions(void)
{
	/*
	*	Some action states need to be reflected in the display before
	*	performing an action.
	*/
	if (sButtonPressed)
	{
		/*
		*	Wakeup the display when any key is pressed.
		*/
		WakeUp();
		/*
		*	If a debounce period has passed
		*/
		{
			uint8_t		pinsState = ((~PINA) & Config::kPINABtnMask);

			/*
			*	If debounced
			*/
			if (mStartPinState == pinsState)
			{
				if (mDebouncePeriod.Passed())
				{
					sButtonPressed = false;
					mStartPinState = 0xFF;
					if (!mIgnoreButtonPress)
					{
						switch (pinsState)
						{
							case Config::kUpBtn:	// Up button pressed
								UpDownButtonPressed(false);
								break;
							case Config::kEnterBtn:	// Enter button pressed
								EnterPressed();
								break;
							case Config::kLeftBtn:	// Left button pressed
								LeftRightButtonPressed(false);
								break;
							case Config::kDownBtn:	// Down button pressed
								UpDownButtonPressed(true);
								break;
							case Config::kRightBtn:	// Right button pressed
								LeftRightButtonPressed(true);
								break;
							default:
								mDebouncePeriod.Start();
								break;
						}
					} else
					{
						mIgnoreButtonPress = false;
					}
				}
			} else
			{
				mStartPinState = pinsState;
				mDebouncePeriod.Start();
			}
		}
	} else if (UnixTime::TimeToSleep() &&
		mMode < eSetTimeMode &&	// Don't allow sleep or mode change when the mode is modal.
		!mInSession)
	{
		/*
		*	If sleep is enabled THEN
		*	put the display to sleep.
		*/
		if (mSleepEnabled)
		{
			GoToSleep();
			GoToMainMode();
		} else
		{
			HideSelectionFrame();
		}
	}
	
	if (sSDInsertedOrRemoved)
	{
		/*
		*	Wakeup the display when an SD card is inserted or removed.
		*/
		WakeUp();
		
		uint8_t		pinsState = (~PIND) & Config::kSDDetect;
		/*
		*	If debounced
		*/
		if (mStartPinState == pinsState)
		{
			if (mDebouncePeriod.Passed())
			{
				sSDInsertedOrRemoved = false;
				mStartPinState = 0xFF;
				SetSDCardPresent(pinsState != 0);
			}
		} else
		{
			mStartPinState = pinsState;
			mDebouncePeriod.Start();
		}
	}
}

/******************************* UpdateDisplay ********************************/
void SDHexLoader::UpdateDisplay(void)
{
	if (!mDisplaySleeping)
	{
		bool updateAll = mMode != mPrevMode;
	
		if (updateAll)
		{
			mPrevMode = mMode;
			if (mMode != eSetTimeMode)
			{
				mDisplay->Fill();
				InitializeSelectionRect();
			}
		}

		switch (mMode)
		{
			case eMainMode:
			{
				// When in session, disable the ability to change sources
				if (updateAll ||
					mPrevSource != mSource)
				{
					mPrevSource = mSource;
					DrawItemP(0, kSourceStr, eWhite);
					DrawItemValueP(mSource == eUSBSource ? kUSBStr :
						(mSource == eSDSource ? kSDStr : kSDBLStr),
							mInSession ? eGray : eMagenta);
				}
				if (updateAll ||
					mPrevInSession != mInSession ||
					mPrevHexFileIndex != mHexFileIndex)
				{
					mPrevInSession = mInSession;
					if (mInSession)
					{
						DrawItemP(1, kStopStr, eRed, Config::kTextInset, true);
						if (mInSession == ePassThrough)
						{
							DrawCenteredItemP(4, kPassThroughStr, eGray);
						} else
						{
							if (mInSession == eWriting)
							{
								DrawItemP(4, kWritingStr, eYellow, Config::kTextInset, true);
							} else	// Verifying
							{
								DrawItemP(4, kVerifyingStr, eGreen, Config::kTextInset, true);
							}
							mPrevPercentage = 0xFF;	// Force the percentage to draw
							DrawPercentComplete();
						}
					} else
					{
						ClearLines(4, 1);	// Idle, clear status line
						/*
						*	If not using SD as the source OR
						*	SD is the source and a hex file is selected THEN
						*	display "Start"
						*/
						if (mSource == eUSBSource ||
							mHexFileIndex != 0)
						{
							DrawItemP(1, (mOnlyUseISP || mSource == eUSBSource
											|| mSource == eSDBLSource
											|| !mUploadSpeed) ?
												kStartISPStr : kStartSerialStr, eGreen,
												Config::kTextInset, true);
						/*
						*	Else, SD is the source but there is no file selected.
						*/
						} else
						{
							DrawItemP(1, mSDCardPresent ? kNoHexFilesStr : kInsertSDCardStr,
												eGray, Config::kTextInset, true);
						}
					}
				} else if (mInSession & (eWriting | eVerifying))
				{
					DrawPercentComplete();
				}

				if (updateAll ||
					mPrevHexFileIndex != mHexFileIndex)
				{
					mPrevHexFileIndex = mHexFileIndex;
					ClearLines(2, 2);
					if (mSource != eUSBSource && mHexFileIndex)
					{
						DrawItem(2, mFilename, mInSession ? eGray : eMagenta);
						DrawItem(3, mMCUDesc, mInSession ? eGray : eCyan);
					}
				}
				
				break;
			}
			case eSettingsMode:
			{
				if (UnixTime::TimeChanged())
				{
					UnixTime::ResetTimeChanged();
					char timeStr[32];
					bool isPM = UnixTime::CreateTimeStr(timeStr);

					DrawCenteredItem(0, timeStr, eCyan);
					/*
					*	If updating everything OR
					*	the AM/PM suffix state changed THEN
					*	draw or erase the suffix.
					*/
					if (updateAll ||
						mPrevIsPM != isPM)
					{
						mPrevIsPM = isPM;
						if (!UnixTime::Format24Hour())
						{
							SetFont(mSmallFont);
							DrawStr(isPM ? " PM":" AM");
							SetFont(mNormalFont);
							// The width of a P is slightly less than an A, so erase any
							// artifacts left over when going from A to P.
							// The width of an 18pt A - the width of a P = 1
							mDisplay->FillBlock(FontRows(), 1, eBlack);
						}
					}
				}
			#if 0
				/*
				*	The date constancy isn't critical, there's no need to ensure
				*	that the date is updated when it changes because it's highly
				*	unlikely that the settings screen will be displayed for more
				*	than a few minutes.  Only draw the date when updateAll is
				*	set.
				*/
				if (updateAll)
				{
					char dateStr[32];
					UnixTime::CreateDateStr(dateStr);
					DrawCenteredItem(0, dateStr, eCyan);
					
					DrawItemP(2, kSetTimeLStr, eWhite);
				}
			#else
				if (updateAll)
				{
					DrawItemP(1, kSetTimeLStr, eWhite);
				}
			#endif
				
				if (updateAll ||
					mPrevSleepEnabled != mSleepEnabled)
				{
					mPrevSleepEnabled = mSleepEnabled;
					DrawItemP(2, kSleepStr, eWhite);
					DrawItemValueP(mSleepEnabled ? kEnabledStr : kDisabledStr, eMagenta);
				}
				
				if (updateAll ||
					mPrevOnlyUseISP != mOnlyUseISP)
				{
					mPrevOnlyUseISP = mOnlyUseISP;
					DrawItemP(3, kISPStr, eWhite);
					DrawItemValueP(mOnlyUseISP ? kForceOnStr : kAutoStr, eMagenta);
				}
				
				if (updateAll ||
					mPrevISPClockIndex != mISPClockIndex)
				{
					mPrevISPClockIndex = mISPClockIndex;
					DrawItemP(4, kClockStr, eWhite);
					char mhzValueStr[5];
					UInt8ToDecStr(mISPClockIndex ? (mISPClockIndex * 4) : 1, mhzValueStr);
					SetTextColor(eMagenta);
					DrawStr(mhzValueStr);
					DrawItemValueP(kMHzStr, eMagenta);
				}
				break;
			}
			case eSetTimeMode:
				mUnixTimeEditor.Update();
				break;
			case eMessageMode:
				if (updateAll)
				{
					DrawCenteredDescP(0, mMessageLine0);
					if (mMessageLine1 == eErrorNumDesc)
					{
						DrawDescP(1, mMessageLine1);
						char errorNumStr[15];
						UInt8ToDecStr(mError, errorNumStr);
						DrawStr(errorNumStr, true);
					} else
					{
						DrawCenteredDescP(1, mMessageLine1);
					}
					DrawCenteredDescP(2, eOKItemDesc);
					mCurrentFieldOrItem = eOKItemItem;
					mSelectionFieldOrItem = 0;	// Force the selection frame to update
				}
				break;
		}
		
		/*
		*	Set time mode has its own selection frame...
		*/
		if (mMode != eSetTimeMode)
		{
			UpdateSelectionFrame();
		}
	}
}

/********************************* ClearLines *********************************/
void SDHexLoader::ClearLines(
	uint8_t	inFirstLine,
	uint8_t	inNumLines)
{
	mDisplay->MoveTo(inFirstLine*Config::kFontHeight, 0);
	mDisplay->FillBlock(inNumLines*Config::kFontHeight, Config::kDisplayWidth, eBlack);
}

/************************** InitializeSelectionRect ***************************/
void SDHexLoader::InitializeSelectionRect(void)
{
	mSelectionRect.x = mMode < eMessageMode ? 0 : 89;
	mSelectionRect.y = mCurrentFieldOrItem * Config::kFontHeight;
	mSelectionRect.width = mMode < eMessageMode ? Config::kDisplayWidth : 62;
	mSelectionRect.height = Config::kFontHeight;
	mSelectionFieldOrItem = mCurrentFieldOrItem;
	mSelectionIndex = 0;
}

/***************************** HideSelectionFrame *****************************/
void SDHexLoader::HideSelectionFrame(void)
{
	if (mSelectionPeriod.Get())
	{
		/*
		*	If the selection frame was last drawn as white THEN
		*	draw it as black to hide it.
		*/
		if (mSelectionIndex & 1)
		{
			mSelectionIndex = 0;
			mDisplay->DrawFrame8(&mSelectionRect, eBlack, 2);
		}
		mSelectionPeriod.Set(0);
	}
}

/***************************** ShowSelectionFrame *****************************/
void SDHexLoader::ShowSelectionFrame(void)
{
	if (!mSelectionPeriod.Get())
	{
		mSelectionPeriod.Set(500);
		mSelectionPeriod.Start();
	}
}

/**************************** UpdateSelectionFrame ****************************/
void SDHexLoader::UpdateSelectionFrame(void)
{
	if (mSelectionPeriod.Get())
	{
		if (mSelectionFieldOrItem != mCurrentFieldOrItem)
		{
			if (mSelectionIndex & 1)
			{
				mDisplay->DrawFrame8(&mSelectionRect, eBlack, 2);
			}
			InitializeSelectionRect();
		}
		if (mSelectionPeriod.Passed())
		{
			mSelectionPeriod.Start();
			mSelectionIndex++;
			uint16_t	selectionColor = (mSelectionIndex & 1) ? eWhite : eBlack;
			mDisplay->DrawFrame8(&mSelectionRect, selectionColor, 2);
		}
	}
}

/******************************** GoToMainMode ********************************/
void SDHexLoader::GoToMainMode(void)
{
	HideSelectionFrame();
	if (mMode != eMainMode)
	{
		mMode = eMainMode;
		mCurrentFieldOrItem = eSourceItem;
		mMaxMainModeItem = (mSource != eUSBSource && !mInSession )? eFilenameItem : eStartStopItem;
		InitializeSelectionRect();
	}
}

#if 0
/****************************** DrawCenteredList ******************************/
void SDHexLoader::DrawCenteredList(
	uint8_t		inLine,
	uint8_t		inTextEnum, ...)
{
	va_list arglist;
	va_start(arglist, inTextEnum);
	for (uint8_t textEnum = inTextEnum; textEnum; textEnum = va_arg(arglist, int))
	{
		DrawCenteredDescP(inLine, textEnum);
		inLine++;
	}
	va_end(arglist);
}
#endif
/***************************** DrawCenteredDescP ******************************/
void SDHexLoader::DrawCenteredDescP(
	uint8_t		inLine,
	uint8_t		inTextEnum)
{
	SStringDesc	textDesc;
	memcpy_P(&textDesc, &kTextDesc[inTextEnum-1], sizeof(SStringDesc));
	DrawCenteredItemP(inLine, textDesc.descStr, textDesc.color);
}

/********************************* DrawDescP **********************************/
void SDHexLoader::DrawDescP(
	uint8_t		inLine,
	uint8_t		inTextEnum,
	uint8_t		inColumn)
{
	SStringDesc	textDesc;
	memcpy_P(&textDesc, &kTextDesc[inTextEnum-1], sizeof(SStringDesc));
	DrawItemP(inLine, textDesc.descStr, textDesc.color, inColumn);
}

/***************************** DrawCenteredItemP ******************************/
void SDHexLoader::DrawCenteredItemP(
	uint8_t		inLine,
	const char*	inTextStrP,
	uint16_t	inColor)
{
	char			textStr[20];	// Assumed all strings are less than 20 bytes
	strcpy_P(textStr, inTextStrP);
	DrawCenteredItem(inLine, textStr, inColor);
}

/****************************** DrawCenteredItem ******************************/
void SDHexLoader::DrawCenteredItem(
	uint8_t		inLine,
	const char*	inTextStr,
	uint16_t	inColor)
{
	mDisplay->MoveToRow((inLine*Config::kFontHeight) + Config::kTextVOffset);
	SetTextColor(inColor);
	DrawCentered(inTextStr);
}

/********************************* DrawItemP **********************************/
void SDHexLoader::DrawItemP(
	uint8_t		inLine,
	const char*	inTextStrP,
	uint16_t	inColor,
	uint8_t		inColumn,
	bool		inClearTillEOL)
{
	char	textStr[20];	// Assumed all strings are less than 20 bytes
	strcpy_P(textStr, inTextStrP);
	DrawItem(inLine, textStr, inColor, inColumn, inClearTillEOL);
}

/********************************** DrawItem **********************************/
void SDHexLoader::DrawItem(
	uint8_t		inLine,
	const char*	inTextStr,
	uint16_t	inColor,
	uint8_t		inColumn,
	bool		inClearTillEOL)
{
	mDisplay->MoveTo((inLine*Config::kFontHeight) + Config::kTextVOffset, inColumn);
	SetTextColor(inColor);
	DrawStr(inTextStr, inClearTillEOL);
}

/******************************* DrawItemValueP *******************************/
/*
*	Draws from the current row and column, then erases till end of line.
*/
void SDHexLoader::DrawItemValueP(
	const char*	inTextStrP,
	uint16_t	inColor)
{
	char	textStr[20];	// Assumed all strings are less than 20 bytes
	strcpy_P(textStr, inTextStrP);
	SetTextColor(inColor);
	DrawStr(textStr, true);
}

/**************************** DrawPercentComplete *****************************/
void SDHexLoader::DrawPercentComplete(void)
{
	uint8_t	percentage = mSDHexSession.PercentageProcessed();
	if (mPrevPercentage != percentage)
	{
		mPrevPercentage = percentage;
		char percentStr[15];
		char* endOfStrPtr = UInt8ToDecStr(percentage, percentStr);
		*(endOfStrPtr++) = '%';
		*endOfStrPtr = 0;
		DrawItem(4, percentStr, mInSession == eWriting ? eYellow : eGreen, 143 + Config::kTextInset, true);
	}
}

/******************************* UInt8ToDecStr ********************************/
/*
*	Returns the pointer to the char after the last char (the null terminator)
*/
char* SDHexLoader::UInt8ToDecStr(
	uint8_t	inNum,
	char*	inBuffer)
{
	if (inNum == 0)
	{
		*(inBuffer++) = '0';
	} else
	{
		int8_t num = inNum;
		for (; num/=10; inBuffer++){}
		char*	bufPtr = inBuffer;
		while (inNum)
		{
			*(bufPtr--) = (inNum % 10) + '0';
			inNum /= 10;
		}
		inBuffer++;
	}
	*inBuffer = 0;
	
	return(inBuffer);
}

/*********************************** WakeUp ***********************************/
/*
*	Wakup the display from sleep or keep it awake if not sleeping.
*/
void SDHexLoader::WakeUp(void)
{
	if (mDisplaySleeping)
	{
		mDisplaySleeping = false;
		// If a button press that caused the display to wake then ignore
		// the current button press (after it debounces)
		// If the wake was because of an SD card being inserted/removed, then
		// don't ignore the next button press.
		mIgnoreButtonPress = sButtonPressed;
		mDisplay->WakeUp();
		mPrevMode = mMode+1;	// Force a full update
	}
	if (mSelectionPeriod.Get() == 0)
	{
		// Display the flashing selection frame.
		ShowSelectionFrame();
		mIgnoreButtonPress = sButtonPressed;
	}
	UnixTime::ResetSleepTime();
}

/********************************* GoToSleep **********************************/
/*
*	Puts the display to sleep.
*/
void SDHexLoader::GoToSleep(void)
{
	if (!mDisplaySleeping)
	{
		mDisplay->Fill();
		mDisplay->Sleep();
		mDisplaySleeping = true;
	}
}

/****************************** SetSDCardPresent ******************************/
void SDHexLoader::SetSDCardPresent(
	bool	inSDCardPresent)
{
	mSDCardPresent = inSDCardPresent;
	if (inSDCardPresent)
	{
		/*
		*	SdFat::initErrorHalt isn't called here when SdFat::begin fails.
		*	SdFat::initErrorHalt doesn't do much other than write to serial why
		*	SdFat::begin failed.
		*/
		mSDCardPresent = mSD.begin(Config::kSDSelectPin);
		if (mSDCardPresent)
		{
			/*
			*	Change the volume's working directory to root.
			*	(This also opens the root.)
			*/
			mSD.chdir();
			mNumSDRootEntries = CountRootDirEntries();
			/*
			*	Per the SdFat header, file directory indexes start at 1. If
			*	there are no root entries then set the file index to 0 as a flag
			*	that the card is empty.  Otherwise set the index to the last
			*	entry then LoadNextHexFilename is called to load the next hex
			*	file in the forward direction.
			*/
			mHexFileIndex = mNumSDRootEntries ? mNumSDRootEntries : 0;
			LoadNextHexFilename(true);
		} else
		{
			//mSD.initErrorHalt();
			mHexFileIndex = 0;
			QueueMessage(eSDCardErrorDesc,
				eNoMessage, eMainMode, eSourceItem);
		}
	/*
	*	Else, if the SD card was removed while an active session is in progress THEN
	*	halt the session and display an SD card error message.
	*/
	} else
	{
		mNumSDRootEntries = 0;
		mHexFileIndex = 0;
		if (mInSession)
		{
			/*
			*	If there's an active SD Hex session THEN
			*	halt the ISP as well.  This will release the target mcu if it's in
			*	program mode connected to the ISP.  If the session is with a
			*	bootloader, the bootloader will simply timeout and attempt to
			*	restart.
			*/
			if (mSDHexSession.Halt())
			{
				mAVRStreamISP.Halt();	// If ISP session in progress.
			}
			QueueMessage(eSDCardErrorDesc,
				eNoMessage, eMainMode, eSourceItem);
		}
	}
	/*
	*	If in main mode AND
	*	the source is SD THEN
	*	move the selection frame to the first line.
	*/
	if (mMode == eMainMode &&
		mSource != eUSBSource)
	{
		mCurrentFieldOrItem = eSourceItem;
	}
	mPrevHexFileIndex = 0xFFFF;	// Force a redraw
}

/**************************** CountRootDirEntries *****************************/
uint16_t SDHexLoader::CountRootDirEntries(void)
{
	FatFile*	vwd = mSD.vwd();
	FatFile		thisFile;
	uint16_t	entryCount = 0;
	vwd->rewind();
	/*
	*	There doesn't appear to be a FatFile routine to get the number of
	*	directory entries.  Loop till the last entry, then get its directory
	*	index.
	*/
	while (thisFile.openNext(vwd))
	{
		entryCount = thisFile.dirIndex();
		thisFile.close();
	}
	vwd->rewind();
	return(entryCount);
}

/**************************** LoadNextHexFilename *****************************/
/*
*	This routine attempts to load the next hex file.  If a valid hex/Config file
*	pair isn't found, false is returned and the mHexFileIndex is set to 0.
*/
bool SDHexLoader::LoadNextHexFilename(
	bool	inIncrement)
{
	bool	success = false;
	if (mHexFileIndex)
	{
		FatFile*	vwd = mSD.vwd();
		FatFile		thisFile;
		uint16_t	startIndex = mHexFileIndex;
		uint16_t	fileIndex = mHexFileIndex;
		do
		{
			if (inIncrement)
			{
				fileIndex++;
				if (fileIndex > mNumSDRootEntries)
				{
					fileIndex = 1;
				}
			} else if (fileIndex > 1)
			{
				fileIndex--;
			} else
			{
				fileIndex = mNumSDRootEntries;
			}
		
			/*
			*	open will fail for all unused entry indexes.
			*	For each valid entry see if it has the expected hex or eep extension AND
			*	that there is a sibling with a txt extension that points to a valid config file.
			*/
			if (thisFile.open(vwd, fileIndex, O_RDONLY))
			{
				bool interested = thisFile.isFile() && !thisFile.isHidden();
				if (interested)
				{
					char filename[52];
					thisFile.getName(filename, 51);
					thisFile.close();
					size_t		pathLen = strlen(filename);
					if (pathLen < 50)
					{
						// Case sensitive test for hex or eep file extension.
						mIsHexFile = memcmp(&filename[pathLen-3], "hex", 3) == 0;
						if (mIsHexFile ||
							memcmp(&filename[pathLen-3], "eep", 3) == 0)
						{
							strcpy(&filename[pathLen-3], "txt");
							AVRConfig	configFile;
							/*
							*	Note that if the filename is >= 50 bytes or it
							*	contains multibyte UTF8 characters, ReadFile()
							*	below will fail and the file will be skipped.
							*	The SdFat lib doesn't appear to support UTF8.
							*	SdFat replaces multibyte characters with '?',
							*	which will cause an open failure in ReadFile().
							*
							*	If there's a corresponding valid config file THEN
							*	save the name, mcu description, and upload speed.
							*/
							if (configFile.ReadFile(filename))
							{
								pathLen -= 4;
								if (memcmp(&filename[pathLen-3], "ino", 3) == 0)
								{
									pathLen -= 4;
								}
								if (pathLen >= (sizeof(mFilename)-1))
								{
									pathLen = sizeof(mFilename) -1;
								}
								filename[pathLen] = 0;
								strcpy(mFilename, filename);
								strcpy(mMCUDesc, configFile.Config().desc);
								mUploadSpeed = configFile.Config().uploadSpeed;
								success = true;
								break;
							}
						}
					}
				} else
				{
					thisFile.close();
				}
			}
		} while (startIndex != fileIndex);
		mHexFileIndex = success ? fileIndex : 0;
	}
	return(success);
}

/************************* Pin change interrupt PCI0 **************************/
/*
*
*	Sets a flag to show that buttons have been pressed.
*	This will also wakeup the mcu if it's sleeping.
*/
ISR(PCINT3_vect)
{
	SDHexLoader::SetSDInsertedOrRemoved();
}

/************************* Pin change interrupt PCI3 **************************/
/*
*
*	Sets a flag to show that buttons have been pressed.
*	This will also wakeup the mcu if it's sleeping.
*/
ISR(PCINT0_vect)
{
	SDHexLoader::SetButtonPressed((PINA & Config::kPINABtnMask) != Config::kPINABtnMask);
}

