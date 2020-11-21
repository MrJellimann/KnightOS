//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// this is the inital kernel to get the keyboard running. PegasOS main functions should go here
#include "kernel.h"
#include <circle/usb/usbkeyboard.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>
#include <pegasos/something.h>
#include <pegasos/shell.h>

#define DRIVE		"SD:"
//#define DRIVE		"USB:"

#define FILENAME	"/circle.txt"

static const char FromKernel[] = "kernel";
int _OffBoot=0, _UserNameMatch=0, _PasswordMatch=0;
char _inputUsername[MAX_INPUT_LENGTH]="", _inputPassword[MAX_INPUT_LENGTH]="", userDirectory[]="SD:/users", _userResponse[]="";

CKernel *CKernel::s_pThis = 0;
PShell *PegasosShell = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_ShutdownMode (ShutdownNone)
	//m_Shell (20)
{
	s_pThis = this;

	PegasosShell = new PShell();
	PegasosShell->AssignKernel(s_pThis);

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

// basic keyboard setup
boolean CKernel::Initialize (void)
{
	// Proving linking bullshit
	//int rando = something;
	awful_funct();

	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}
	
	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);


	// Mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	}

	if (m_FileSystem.fs_type == FS_EXFAT)
	{
		m_Logger.Write (FromKernel, LogNotice, "\t\tExFAT <ACTIVE>");
	}

	if (m_FileSystem.fs_type == FS_FAT12)
	{
		m_Logger.Write (FromKernel, LogNotice, "\t\tFAT12 <ACTIVE>");
	}

	if (m_FileSystem.fs_type == FS_FAT16)
	{
		m_Logger.Write (FromKernel, LogNotice, "\t\tFAT16 <ACTIVE>");
	}

	if (m_FileSystem.fs_type == FS_FAT32)
	{
		m_Logger.Write (FromKernel, LogNotice, "\t\tFAT32 <ACTIVE>");
	}
	// Show contents of root directory
	DIR Directory;
	FILINFO FileInfo;
	FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
	for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
	{
		if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
		{
			CString FileName;
			FileName.Format ("%-19s", FileInfo.fname);

			//m_Screen.Write ((const char *) FileName, FileName.GetLength ());

			if (i % 4 == 3)
			{
				//m_Screen.Write ("\n", 1);
			}
		}

		Result = f_findnext (&Directory, &FileInfo);
	}
	//m_Screen.Write ("\n", 1);

	
	// Create file and write to it
	FIL File;
	Result = f_open (&File, DRIVE FILENAME, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot create file: %s", FILENAME);
	}

	
	for (unsigned nLine = 1; nLine <= 5; nLine++)
	{
		CString Msg;
		Msg.Format ("Hello File! (Line %u)\n", nLine);

		unsigned nBytesWritten;
		if (   f_write (&File, (const char *) Msg, Msg.GetLength (), &nBytesWritten) != FR_OK
		    || nBytesWritten != Msg.GetLength ())
		{
			m_Logger.Write (FromKernel, LogError, "Write error");
			break;
		}
	}

	
	if (f_close (&File) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}

	// Reopen file, read it and display its contents
	Result = f_open (&File, DRIVE FILENAME, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open file: %s", FILENAME);
	}
	
	char Buffer[100];
	unsigned nBytesRead;
	while ((Result = f_read (&File, Buffer, sizeof Buffer, &nBytesRead)) == FR_OK)
	{
		if (nBytesRead > 0)
		{
			m_Screen.Write (Buffer, nBytesRead);
		}

		if (nBytesRead < sizeof Buffer)		// EOF?
		{
			break;
		}
	}

	
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogError, "Read error");
	}
	
	if (f_close (&File) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}


	// Reopen file, read it and display its contents
	Result = f_open (&File, DRIVE FILENAME, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open file: %s", FILENAME);
	}

	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");

		return ShutdownHalt;
	}
	
#if 1	// set to 0 to test raw mode
	pKeyboard->RegisterShutdownHandler (ShutdownHandler);
	pKeyboard->RegisterKeyPressedHandler (KeyPressedHandler);
#else
	pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
#endif
	//s_pThis->m_Screen.Write ("welcome to PegasOS!\n", 21);
	//m_Logger.Write (FromKernel, LogNotice, "Just type something!");
	s_pThis->m_Screen.Write("Hello, Welcome to PegasOS!\nPlease login in to continue...\nUsername: ",67);
	//commenceLogin();
	//PegasosShell->DisplayUserWithDirectory();
	
	// this is the main loop for the OS
	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		// CUSBKeyboardDevice::UpdateLEDs() must not be called in interrupt context,
		// that's why this must be done here. This does nothing in raw mode.
		pKeyboard->UpdateLEDs ();

		
		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	// Unmount file system
	if (f_mount (0, DRIVE, 0) != FR_OK)
	{
		//m_Logger.Write (FromKernel, LogPanic, "Cannot unmount drive: %s", DRIVE);
	}

	return m_ShutdownMode;
}

void CKernel::commenceLogin()
{
	s_pThis->m_Screen.Write ("We entered the commenceLogin function\n",38);
	char  _tempFileName[]="";
	char Buffer[100];
	if(_OffBoot == 0)
	{	///// This will find the user directory that matches the username given by the user. /////
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result = f_findfirst (&Directory, &FileInfo, userDirectory, "*");
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			s_pThis->m_Screen.Write ("We entered the for loop\n",24);
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				s_pThis->m_Screen.Write ("We entered the main if loop\n",28);
				CString FileName;
				FileName.Format ("%-19s", FileInfo.fname);
				strcpy(_tempFileName,FileName);
				EditFileName(_tempFileName);
				s_pThis->m_Screen.Write (_tempFileName,strlen(_tempFileName));
				s_pThis->m_Screen.Write ("\n",1);
				if(strcmp(_inputUsername,_tempFileName)==0)
				{
					_UserNameMatch=1;
				}
				s_pThis->m_Screen.Write ("Exiting the if loop now.\n",25);
			}
			s_pThis->m_Screen.Write ("We exited the main if loop and now about to repeat the while loop\n",66);
			Result = f_findnext (&Directory, &FileInfo);
		}
		if(_UserNameMatch==1)	///// This shows that we found a user and now waiting for input by the user. /////
		{
			s_pThis->m_Screen.Write ("Please enter your password:",29);
			_OffBoot=3;
		}
		else if(_UserNameMatch==0)	///// This shows that we found not user directory that matches the one given by the user. We then ask if they want to create a user or not. /////
		{
			s_pThis->m_Screen.Write ("There is no account for that Username. Would you like to create one?\n",69);
			_OffBoot=1;
		}
	}
	else if(_OffBoot == 1)	///// This will handle the case of the user answering Yes or No. /////
	{
		s_pThis->m_Screen.Write("Entered the _OffBoot==1 if loop with",37);
		s_pThis->m_Screen.Write(_userResponse,strlen(_userResponse));
		s_pThis->m_Screen.Write("\n",1);
		if(strcmp(_userResponse,"yes") == 0)	///// This means that the user wants to make a new username. /////
		{
			//
			_OffBoot=5;
		}
		if(strcmp(_userResponse,"no") == 0)	///// This means that the user does not want to make a new username and will then be prompted to enter a username. /////
		{
			s_pThis->m_Screen.Write("Please enter your username.\nUsername: ",38);
			_OffBoot=0;
		}
	}
	else if(_OffBoot == 3)
	{
		strcat(userDirectory,"/");
		strcat(userDirectory,_inputUsername);
		strcat(userDirectory,"/");
		strcat(userDirectory,_inputUsername);
		strcat(userDirectory,".txt");
		//s_pThis->m_Screen.Write(userDirectory,strlen(userDirectory));
		FIL passwordFILE;
		FRESULT Result = f_open (&passwordFILE, userDirectory, FA_READ | FA_OPEN_EXISTING);
		if (Result != FR_OK)
		{
			s_pThis->m_Screen.Write("Cannot open the file!\n",22);
		}
		
		unsigned nBytesRead;
		while ((Result = f_read (&passwordFILE, Buffer, sizeof Buffer, &nBytesRead)) == FR_OK)
		{
			if (nBytesRead > 0)
			{
				s_pThis->m_Screen.Write("|", 1);
				s_pThis->m_Screen.Write(Buffer, strlen(Buffer));
				s_pThis->m_Screen.Write("|", 1);
			}
			if (nBytesRead < sizeof Buffer)		// EOF?
			{
				break;
			}
		}
		if(strcmp(Buffer,_inputPassword)==0)
		{
			s_pThis->m_Screen.Write("Successfully logged in!\n",24);
			PegasosShell->EditUserName(_inputUsername);
			PegasosShell->DisplayUserWithDirectory();
			_OffBoot=5;
		}
		else if(strcmp(Buffer,_inputPassword)!=0)
		{
			s_pThis->m_Screen.Write("The password does not match!\n",29);
		}
		if (f_close (&passwordFILE) != FR_OK)
		{
			s_pThis->m_Screen.Write("Cannot close the file!\n",22);
		}
	}
}

void CKernel::EditFileName(char* tempFileName)
{
	int length=strlen(tempFileName), index=0;
	while(index<length)
	{
		if(tempFileName[index]==32)
		{
			tempFileName[index]='\0';
			break;
		}
		index++;
	}
}

void CKernel::KeyPressedHandler (const char *pString)
{
	assert (s_pThis != 0);
	s_pThis->m_Screen.Write (pString, strlen (pString));
	// CommandLineIn(pString);
	if(_OffBoot != 5)
	{
		LoginInput(pString);
	}
	else if(_OffBoot == 5)
	{
		PegasosShell->CommandLineIn(pString);
	}
}

void CKernel::LoginInput(const char* keyInput)
{
	int stringLength=0;
	if(_OffBoot == 0)
	{
		if (strcmp(keyInput, "\n") == 0)
		{
			stringLength = strlen(_inputUsername);
			_inputUsername[stringLength] = '\0';
			commenceLogin();
		}
		if (strcmp(keyInput,"\n") != 0)
			strcat(_inputUsername, keyInput);
	}
	else if(_OffBoot == 1)
	{
		if(strcmp(keyInput, "\n") == 0)
		{
			stringLength = strlen(_userResponse);
			_userResponse[stringLength] = '\0';
			commenceLogin();
		}
		if(strcmp(keyInput, "\n") !=0)
			strcat(_userResponse,keyInput);
	}
	else if(_OffBoot == 3)
	{
		if (strcmp(keyInput, "\n") == 0)
		{
			stringLength = strlen(_inputPassword);
			_inputPassword[stringLength] = '\0';
			commenceLogin();
		}
		if (strcmp(keyInput,"\n") != 0)
			strcat(_inputPassword, keyInput);
	}
}

void CKernel::ShutdownHandler (void)
{
	assert (s_pThis != 0);
	s_pThis->m_ShutdownMode = ShutdownReboot;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	assert (s_pThis != 0);

	CString Message;
	Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			CString KeyCode;
			KeyCode.Format (" %02X", (unsigned) RawKeys[i]);

			Message.Append (KeyCode);
		}
	}

	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}

//============================================================================//
// PegasOS Kernel Extensions for Shell Commands
void CKernel::SystemReboot()
{
	s_pThis->m_ShutdownMode = ShutdownReboot;
}

void CKernel::SystemOff()
{
	s_pThis->m_ShutdownMode = ShutdownHalt;
}

//============================================================================//
// Getter Extensions for internal kernel devices

CMemorySystem *CKernel::GetKernelMemory()
{
	return &(s_pThis->m_Memory);
}

CActLED *CKernel::GetKernelActLED()
{
	return &(s_pThis->m_ActLED);
}

CDeviceNameService *CKernel::GetKernelDNS()
{
	return &(s_pThis->m_DeviceNameService);
}

CScreenDevice *CKernel::GetKernelScreenDevice()
{
	return &(s_pThis->m_Screen);
}

CSerialDevice *CKernel::GetKernelSerialDevice()
{
	return &(s_pThis->m_Serial);
}

CExceptionHandler *CKernel::GetKernelExceptionHandler()
{
	return &(s_pThis->m_ExceptionHandler);
}

CInterruptSystem *CKernel::GetKernelInterruptSystem()
{
	return &(s_pThis->m_Interrupt);
}

CTimer *CKernel::GetKernelTimer()
{
	return &(s_pThis->m_Timer);
}

CLogger *CKernel::GetKernelLogger()
{
	return &(s_pThis->m_Logger);
}
