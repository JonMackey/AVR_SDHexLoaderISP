/*
*	SDHexLoader.h, Copyright Jonathan Mackey 2020
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
#ifndef SDHexLoader_h
#define SDHexLoader_h

#include "SdFat.h"
#include "XFont.h"
#include <inttypes.h>
#include <time.h>
#include "MSPeriod.h"
#include "UnixTimeEditor.h"
#include "SDHexSession.h"
#include "AVRStreamISP.h"
#include "SDHexLoaderConfig.h"

class SDHexLoader : public XFont
{
public:
							SDHexLoader(void);
	void					begin(
								DisplayController*		inDisplay,
								Font*					inNormalFont,
								Font*					inSmallFont);
		
	void					Update(void);
	static void				SetButtonPressed(
								bool					inButtonPressed)
								{sButtonPressed = sButtonPressed || inButtonPressed;}
	static void				SetSDInsertedOrRemoved(void)
								{sSDInsertedOrRemoved = true;}

protected:
	UnixTimeEditor			mUnixTimeEditor;
	SDHexSession			mSDHexSession;
	AVRStreamISP			mAVRStreamISP;
	SdFat					mSD;
	Font*					mNormalFont;
	Font*					mSmallFont;
	Rect8_t					mSelectionRect;
	MSPeriod				mDebouncePeriod;	// For buttons and SD card
	MSPeriod				mSelectionPeriod;	// Selection frame flash rate
	
	uint8_t					mSleepState;
	uint8_t					mMode;
	uint8_t					mPrevMode;
	uint8_t					mCurrentFieldOrItem;
	uint8_t					mMaxMainModeItem;
	uint8_t					mSelectionFieldOrItem;
	uint8_t					mStartPinState;
	uint8_t					mError;	// Used by eErrorDesc
	uint8_t					mPrevPercentage;
	uint8_t					mISPClockIndex;
	uint8_t					mPrevISPClockIndex;
	bool					mSDCardPresent;
	bool					mIgnoreButtonPress;
	bool					mSleepEnabled;
	bool					mPrevSleepEnabled;
	bool					mDisplaySleeping;
	bool					mPrevIsPM;
	bool					mUseSDSource;	// Else use USB
	bool					mPrevUseSDSource;
	bool					mOnlyUseISP;	// Else auto based on mUploadSpeed
	bool					mPrevOnlyUseISP;
	bool					mIsHexFile;
	uint8_t					mInSession;
	uint8_t					mPrevInSession;
	uint8_t					mSelectionIndex;
	uint8_t					mMessageLine0;
	uint8_t					mMessageLine1;
	uint8_t					mMessageReturnMode;
	uint8_t					mMessageReturnItem;
	uint16_t				mHexFileIndex;	// Of hex file on SD
	uint16_t				mPrevHexFileIndex;
	uint16_t				mNumSDRootEntries;
	char					mFilename[20];	// Of hex file with extension removed
	char					mMCUDesc[20];	// from current SD config (ATtiny84A, etc)
	uint32_t				mUploadSpeed;	// from current SD config (0 if ISP only)
	static bool				sButtonPressed;
	static bool				sSDInsertedOrRemoved;

	void					SetSDCardPresent(
								bool					inSDCardPresent);
	void					UpdateActions(void);
	void					UpdateDisplay(void);
	uint8_t					Mode(void) const
								{return(mMode);}
	uint8_t					CurrentFieldOrItem(void) const
								{return(mCurrentFieldOrItem);}
	void					GoToMainMode(void);
	void					EnterPressed(void);
	void					UpDownButtonPressed(
								bool					inIncrement);
	void					LeftRightButtonPressed(
								bool					inIncrement);

	void					WakeUp(void);
	void					GoToSleep(void);

	void					ClearLines(
								uint8_t					inFirstLine,
								uint8_t					inNumLines);
//	void					DrawCenteredList(
//								uint8_t					inLine,
//								uint8_t					inTextEnum, ...);
	void					DrawCenteredDescP(
								uint8_t					inLine,
								uint8_t					inTextEnum);
	void					DrawCenteredItemP(
								uint8_t					inLine,
								const char*				inTextPStr,
								uint16_t				inColor);
	void					DrawCenteredItem(
								uint8_t					inLine,
								const char*				inTextStr,
								uint16_t				inColor);
	void					DrawDescP(
								uint8_t					inLine,
								uint8_t					inTextEnum,
								uint8_t					inColumn = Config::kTextInset);
	void					DrawItemP(
								uint8_t					inLine,
								const char*				inTextPStr,
								uint16_t				inColor,
								uint8_t					inColumn = Config::kTextInset,
								bool					inClearTillEOL = false);
	void					DrawItemValueP(
								const char*				inTextPStr,
								uint16_t				inColor);
	void					DrawItem(
								uint8_t					inLine,
								const char*				inTextStr,
								uint16_t				inColor,
								uint8_t					inColumn = Config::kTextInset,
								bool					inClearTillEOL = false);
	void					DrawPercentComplete(void);
	void					UpdateSelectionFrame(void);
	void					HideSelectionFrame(void);
	void					ShowSelectionFrame(void);
	void					InitializeSelectionRect(void);
	void					QueueMessage(
								uint8_t					inMessageLine0,
								uint8_t					inMessageLine1,
								uint8_t					inReturnMode,
								uint8_t					inReturnItem);
	bool					LoadNextHexFilename(
								bool					inIncrement);
	uint16_t				CountRootDirEntries(void);
	static char*			UInt8ToDecStr(
								uint8_t					inNum,
								char*					inBuffer);
	
	enum EMode
	{
		eMainMode,
		eSettingsMode,
		// All modes below are modal (waiting for input of some sort.)
		// The display will not go to sleep when in a modal mode.
		eSetTimeMode,
		eMessageMode,
		eForceRedraw
	};
	enum EMainItem
	{
		eSourceItem,
		eStartStopItem,
		eFilenameItem,
		eLoadStatusItem,
		eMCUNameItem
	};
	enum ESettingsItem
	{
		eTimeItem,
		eSetTimeItem,
		eEnableSleepItem,
		eISPItem,
		eClockItem
	};
	enum ESessionState
	{
		eIdle,				// Must be 0
		eWriting,			// 0b001
		eVerifying,			// 0b010
		ePassThrough	= 4	// 0b100
	};
	enum EMessageItem
	{
		eMessage0Item,
		eMessage1Item,
		eOKItemItem,
	};
	enum ETextDesc
	{
		eTextListEnd,
		// Messages
		eNoMessage,
		eOKItemDesc,
		eSDCardErrorDesc,
		eFileOpenErrorDesc,
		eInternalISPDesc,
		eSDSessionDesc,
		// SD Session errors must be in the same order as SDHexSession::EErrors
		eTimeoutErrorDesc,
		eSyncErrorDesc,
		eUnknownErrorDesc,
		eHexDataErrorDesc,
		eSignatureErrorDesc,
		eVerifyFailedDesc,
		eSuccessDesc,
	//	eYesItemDesc,
	//	eNoItemDesc,
		eErrorNumDesc
	};
};

#endif // SDHexLoader_h
