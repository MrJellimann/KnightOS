#include <circle/usb/usbkeyboard.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>
#include <pegasos/something.h>
#include <pegasos/shell.h>

#define SHELLDRIVE		"SD:/"
#define MAX_USER_INPUT	256

CKernel *pKernel = 0;
PShell *PShell::s_pThis = 0;

static const char _FromKernel[] = "kernel";

int _stringLen, _globalIndex=0, dirRed=31, dirGreen=31, dirBlue=31, userRed=31, userGreen=31, userBlue=31;//, _OffBoot=0;
char _inputByUser[MAX_USER_INPUT], _message[PMAX_INPUT_LENGTH]="Command was found!"; /////////
char _directory[PMAX_DIRECTORY_LENGTH]="SD:";
char _mainCommandName[PMAX_DIRECTORY_LENGTH];
char _commandParameterOne[PMAX_INPUT_LENGTH];
char _commandParameterTwo[PMAX_INPUT_LENGTH];
char _userName[PMAX_INPUT_LENGTH] = "GiancarloGuillen";
char _helloMessagePartOne[PMAX_INPUT_LENGTH] = "Well hello there ";
char _helloMessagePartTwo[PMAX_INPUT_LENGTH] = ", and welcome to PegasOS!";
char _helpMessage1[PMAX_INPUT_LENGTH] = "This is a list of the Commands for PegasOS:\n\tchangedir\n\tclear\n\tcreatefile\n\tcreatedir\n\tcopy\n\tcurrenttasks";
char _helpMessage2[PMAX_INPUT_LENGTH] = "\n\tdelete\n\tdeletedir\n\tdirtext\n\tdisplaytasks\n\techo\n\thead\n\thello\n\thelp\n\tlistdir\n\tmemorystats";
char _helpMessage3[PMAX_INPUT_LENGTH] = "\n\tmove\n\tpower\n\treboot\n\tsysteminfo\n\ttail\n\tterminatetask\n\tusertext\n\twriteto\n";
FIL _NewFIle, _ReadFile;
TScreenColor color;
TScreenStatus stat;

PShell::PShell(void)
{
    s_pThis = this;
}

PShell::~PShell(void)
{
    s_pThis = 0;
}

void PShell::AssignKernel(CKernel* _kernel)
{
    assert (_kernel != 0);

    pKernel = _kernel;
}

void PShell::CommandLineIn(const char* keyInput)
{	
	int _len = strlen(keyInput);
	int _len2;

	for (int i = 0; i < _len; i++)
	{
		// Debug
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "[%c, %c], [%s]", keyInput[i], (char)keyInput[i], &keyInput[i]);
		switch ((char)keyInput[i])
		{
			case '\b':
			case 127:
				_len2 = strlen(_inputByUser);
				_inputByUser[_len2-1] = '\0';
				break;
			
			case '\n':
				_stringLen = strlen(_inputByUser);
				_inputByUser[_stringLen] = '\0';
				SplitCommandLine(_inputByUser);
				CommandMatch(_mainCommandName);
				DisplayUserWithDirectory();
				strcpy(_inputByUser, "");
				break;
			
			case '\x1b': // Ignore all escapes for now
				// [A - Up
				// [B - Down
				// [C - Right
				// [D - Left

				// Skip the rest of the string
				i = _len;
				break;
			
			default:
				strcat(_inputByUser, &keyInput[i]);
				break;
		}
	}
	
	// Debug
	// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "[%s] [len:%i]", _inputByUser, strlen(_inputByUser));
}

void PShell::SplitCommandLine(const char* input)
{
	strcpy(_mainCommandName, "");
	strcpy(_commandParameterOne, "");
	strcpy(_commandParameterTwo, "");
	int  mainIndex = 0, subIndex = 0, spacebar = 0, stringLength=strlen(input);

	// Debug
	// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "inputLength[%i]", stringLength);

	for (int x = 0; x < stringLength; x++)
	{
		if (input[x] == 32)
			spacebar += 1;
	}


	if (spacebar == 0)
	{
		strcpy(_mainCommandName, input);
	}
	else if (spacebar == 1)
	{
		for (int x = 0; x < stringLength; x++)
		{
			if (input[x] == 32)
				spacebar = x;
		}
		for (int x = 0; x < spacebar; x++, mainIndex++)
		{
			_mainCommandName[mainIndex] = input[mainIndex];
		}

		_mainCommandName[mainIndex] = '\0';
		mainIndex++;

		for (; subIndex<stringLength; subIndex++, mainIndex++)
		{
			_commandParameterOne[subIndex] = input[mainIndex];
		}
		_commandParameterOne[subIndex] = '\0';
	}
	else if(spacebar>=2)
	{
		spacebar=0;
		for (int x = 0; x < stringLength; x++)
		{
			if (input[x] == 32)
			{
				spacebar = x;
				break;
			}
		}
		for (int x = 0; x < spacebar; x++, mainIndex++)
		{
			_mainCommandName[mainIndex] = input[mainIndex];
		}
		_mainCommandName[mainIndex] = '\0';
		mainIndex++;
		for (int x = mainIndex; x < stringLength; x++)
		{
			if (input[x] == 32)
			{
				spacebar = x;
				break;
			}
		}
		for (int x = 0; mainIndex < spacebar; x++, mainIndex++)
		{
			_commandParameterOne[x] = input[mainIndex];
		}
		_commandParameterOne[mainIndex] = '\0';
		mainIndex++;
		for (; subIndex<stringLength; subIndex++, mainIndex++)
		{
			_commandParameterTwo[subIndex] = input[mainIndex];
		}
		_commandParameterTwo[subIndex] = '\0';
	}

	// Debug
	// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "_mCN[%s]", _mainCommandName);
}

void PShell::CommandMatch(const char *commandName)
{
    // Change Directory
	if (strcmp("changedir", commandName) == 0)
	{
        assert(pKernel != 0);
    
    // 		pKernel->GetKernelScreenDevice()->Write(_message, strlen(_message));
    // 		pKernel->GetKernelScreenDevice()->Write("\n", 1);
    
		char _FilePath[MAX_DIRECTORY_LENGTH] = "";
		
		if (strcmp(_commandParameterOne, "") != 0)
		{
			strcat(_FilePath,_directory);
			strcat(_FilePath,"/");
			strcat(_FilePath,_commandParameterOne);
			FRESULT _Result=f_chdir(_FilePath);
			if (_Result != FR_OK)
			{
				pKernel->GetKernelScreenDevice()->Write("The file path was incorrect\n", 28);
				//pKernel->GetKernelScreenDevice()->Write(_FilePath,strlen(_FilePath));
			}
			else if(_Result == FR_OK) // was just if before
			{
				strcpy(_directory,_FilePath);
				FixWorkingDirectory();
				//pKernel->GetKernelScreenDevice()->Write(, strlen(currentLine));
			}
		}
		
	}
    // Create File
	else if (strcmp("createfile", commandName) == 0)
	{
		char fileName[] = "";
		strcat(fileName, _directory);
		strcat(fileName,"/");
		strcat(fileName, _commandParameterOne);
    
		FRESULT Result = f_open (&_NewFIle, fileName, FA_WRITE | FA_CREATE_ALWAYS);
		if (Result != FR_OK)
		{
			//pKernel->m_Screen.Write("Can not create file");
			pKernel->GetKernelLogger()->Write(_FromKernel, LogPanic, "Cannot create file %s", _NewFIle);
			//m_Logger.Write (_FromKernel, LogPanic, "Cannot create file: %s", FILENAME);
		}
		
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogPanic, "Cannot close file");
		}
	}
	// Create Directory
	else if (strcmp("createdir", commandName) == 0)
	{
		assert(pKernel != 0);
		char _FilePath[]="";
		strcat(_FilePath,_directory);
		strcat(_FilePath,"/");
		strcat(_FilePath,_commandParameterOne);
		//DIR Directory;
		FRESULT _Result=f_mkdir(_FilePath);
		if (_Result != FR_OK)
		{
			pKernel->GetKernelScreenDevice()->Write("The sub-directory wasn't able to be made.\n", 42);
		}
	}
	// Clear
	else if (strcmp("clear", commandName) == 0)
	{
		assert(pKernel != 0);
		for (int i = 0; i < 60; i++)
		{
			pKernel->GetKernelScreenDevice()->Write("\n",1);
		}

		stat = pKernel->GetKernelScreenDevice()->GetStatus();
		stat.nState = 2;
		if(pKernel->GetKernelScreenDevice()->SetStatus(stat)==false)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogDebug, "Screen stat just returned false");
		}
		pKernel->GetKernelScreenDevice()->Write("H",1);	
	}
	// Copy
	else if (strcmp("copy", commandName) == 0)
	{
		assert(pKernel != 0);
		int check;
		char mainFileName[] = "", newFileName[MAX_INPUT_LENGTH] = "", buffer[MAX_INPUT_LENGTH] = "";
		
		strcpy(mainFileName, _directory);
		strcat(mainFileName, "/");
		strcat(mainFileName, _commandParameterOne);

		strcpy(newFileName, _commandParameterTwo);
		strcat(newFileName, "/");
		strcat(newFileName, _commandParameterOne);

		// Debug
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogDebug, "The mainFileName is: |%s| The newFileName is: |%s|\n", mainFileName, newFileName);
		
		// Try to open file for reading first
		FRESULT mainResult = f_open (&_ReadFile, mainFileName, FA_READ | FA_OPEN_EXISTING);
		if (mainResult != FR_OK)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file for reading: %s", mainFileName);

			return;
		}
		else
		{
			// Success -> now make new file for writing
			FRESULT newResult = f_open (&_NewFIle, newFileName, FA_WRITE | FA_CREATE_ALWAYS);

			if (newResult != FR_OK)
			{
				pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file for writing: %s", newFileName);

				f_close(&_ReadFile);
				return;
			}
		}

		// Write to new file
		while (f_gets(buffer, MAX_INPUT_LENGTH, &_ReadFile) != nullptr)
		{
			// Debug
			// pKernel->GetKernelScreenDevice()->Write(buffer,strlen(buffer));
			check = f_puts(buffer,&_NewFIle);

			if (!check)
			{
				pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Failed to write line to file '%s'", newFileName);
			}
		}

		// Cleanup
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file for writing: '%s'", newFileName);
		}

		if (f_close(&_ReadFile) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file for reading: '%s'", mainFileName);
		}
	}
    // Current Directory
	else if (strcmp("listdir", commandName) == 0)
	{
		pKernel->GetKernelScreenDevice()->Write("Current Working Directory is: ",31);
		pKernel->GetKernelScreenDevice()->Write(_directory,strlen(_directory));
		pKernel->GetKernelScreenDevice()->Write("\n",1);
		// char buffer[MAX_INPUT_LENGTH] = "";
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result = f_findfirst (&Directory, &FileInfo, _directory, "*");
    
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				CString FileName;
				FileName.Format ("%-19s", FileInfo.fname);

				pKernel->GetKernelScreenDevice()->Write ((const char *) FileName, FileName.GetLength ());

				if (i % 4 == 3)
				{
					pKernel->GetKernelScreenDevice()->Write ("\n", 1);
				}
			}

			Result = f_findnext (&Directory, &FileInfo);
		}

		// Newline at the end of 'listdir' printouts
		pKernel->GetKernelScreenDevice()->Write ("\n", 1);
	}
    // Delete
	else if ((strcmp("delete", commandName) == 0) || (strcmp("deletedir", commandName) == 0))
	{
		assert(pKernel != 0);
		char _FilePath[]="";
		strcat(_FilePath,_directory);
		strcat(_FilePath,"/");
		strcat(_FilePath,_commandParameterOne);
		//DIR Directory;
		FRESULT _Result=f_unlink(_FilePath);
		if ((_Result != FR_OK) && (strcmp("delete", commandName) == 0))
		{
			pKernel->GetKernelScreenDevice()->Write("The file was not deleted.\n", 26);
		}
		if ((_Result != FR_OK) && (strcmp("deletedir", commandName) == 0))
		{
			pKernel->GetKernelScreenDevice()->Write("The directory was not deleted.\n", 31);
		}
	}
	// Change the Directory color
	else if(strcmp("dirtext",commandName)==0)
	{
		assert(pKernel != 0);
		char temp[PMAX_INPUT_LENGTH]="", digits[PMAX_INPUT_LENGTH]="";
		strcpy(temp,_commandParameterOne);
		int index = 0, sub = 0;
		for(int x=0;x<3;x++)
		{
			//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"Entered the for loop with %d",x);
			while (index < (int)strlen(temp))
			{
				if (temp[index] == ',')
					break;
				
				//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"Entered the while loop with temp[%d]=%c",index,temp[index]);
				digits[sub]=temp[index];
				sub++,index++;
			}

			digits[sub]='\0';
			if(x==0)
				dirRed=atoi(digits);
			if(x==1)
				dirGreen=atoi(digits);
			if(x==2)
				dirBlue=atoi(digits);
			sub=0,index++;
			strcpy(digits,"");
		}
		SetColor(dirRed,dirGreen,dirBlue);
	}
    // Echo
	else if (strcmp("echo", commandName) == 0)
	{
		strcpy(_message, "Echo ");

		if (strlen(_commandParameterOne) >= PMAX_INPUT_LENGTH - strlen(_message))
		{
			strncpy(_message, _commandParameterOne, PMAX_INPUT_LENGTH - strlen(_message));

			pKernel->GetKernelScreenDevice()->Write(_message, strlen(_message));
			pKernel->GetKernelScreenDevice()->Write("\n", 1);

			return;
		}

		strcat(_message, _commandParameterOne);

		if (strlen(_commandParameterTwo) > 0)
			strcat(_message, " ");

		if (strlen(_commandParameterTwo) >= PMAX_INPUT_LENGTH - strlen(_message))
		{
			strncpy(_message, _commandParameterTwo, PMAX_INPUT_LENGTH - strlen(_message));

			pKernel->GetKernelScreenDevice()->Write(_message, strlen(_message));
			pKernel->GetKernelScreenDevice()->Write("\n", 1);

			return;
		}

		strcat(_message, _commandParameterTwo);

		if (strlen(_message) <= PMAX_INPUT_LENGTH - 7)
			strcat(_message, " Echo!");
		
		pKernel->GetKernelScreenDevice()->Write(_message, strlen(_message));
		pKernel->GetKernelScreenDevice()->Write("\n", 1);
	}
	// Head
	else if(strcmp("head", commandName)==0)
	{
		assert(pKernel != 0);
		//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"We entered head command with |%s| |%s|\n",_commandParameterOne,_commandParameterTwo);
		int _LinesToBeRead=5, _LinesRead=0;
		if(strcmp(_commandParameterTwo,"")!=0)
		{
			_LinesToBeRead=atoi(_commandParameterTwo);
		}
		//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"_LinesToBeRead is: |%d|\n",_LinesToBeRead);
		char fileName[] = "", buffer[MAX_INPUT_LENGTH]="";
		strcpy(fileName,_directory);
		strcat(fileName,"/");
		strcat(fileName,_commandParameterOne);
		//pKernel->GetKernelScreenDevice()->Write(fileName,strlen(fileName));
		//pKernel->GetKernelScreenDevice()->Write("\n",1);
		FRESULT Result = f_open (&_NewFIle, fileName, FA_READ | FA_OPEN_EXISTING);
		if (Result != FR_OK)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file: %s", fileName);
		}
		while(f_gets(buffer,100,&_NewFIle)!=nullptr)
		{
			pKernel->GetKernelScreenDevice()->Write(buffer,strlen(buffer));
			if (_LinesRead==_LinesToBeRead)
			{
				break;
			}
			_LinesRead+=1;
		}
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file");
		}

		// Newline at end of 'head' printouts
		pKernel->GetKernelScreenDevice()->Write ("\n", 1);
	}
    // Hello
	else if (strcmp("hello", commandName) == 0)
	{
		//strcpy(message,m_shell.Hello("Datboi"));
		strcat(_helloMessagePartOne, _userName);
		strcat(_helloMessagePartOne, _helloMessagePartTwo);
		pKernel->GetKernelScreenDevice()->Write(_helloMessagePartOne, strlen(_helloMessagePartOne));
		pKernel->GetKernelScreenDevice()->Write("\n", 1);
		strcpy(_helloMessagePartOne,"Well hello there ");
	}
    // Help
	else if (strcmp("help", commandName) == 0)
	{
		pKernel->GetKernelScreenDevice()->Write(_helpMessage1, strlen(_helpMessage1));
		pKernel->GetKernelScreenDevice()->Write(_helpMessage2, strlen(_helpMessage2));
		pKernel->GetKernelScreenDevice()->Write(_helpMessage3, strlen(_helpMessage3));
	}
    // Login
	else if (strcmp("login", commandName) == 0)
	{
		// pKernel->GetKernelScreenDevice()->Write("We found login!\n", 16);
	}
	// Move
	else if (strcmp("move", commandName) == 0)
	{
		assert(pKernel != 0);
		int check;
		
		// mainFileName -> file you're moving
		// newFileName -> destination you're moving to
		char mainFileName[MAX_DIRECTORY_LENGTH] = "", newFileName[MAX_INPUT_LENGTH] = "", buffer[MAX_INPUT_LENGTH] = "";
		
		strcpy(mainFileName,_directory);
		strcat(mainFileName,"/");
		strcat(mainFileName,_commandParameterOne);

		strcpy(newFileName,_commandParameterTwo);
		strcat(newFileName,"/");
		strcat(newFileName,_commandParameterOne);

		// Debug
		// pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"The mainFileName is: |%s| The newFileName is: |%s|\n",mainFileName,newFileName);

		FRESULT mainResult = f_open (&_ReadFile, mainFileName, FA_READ | FA_OPEN_EXISTING);
		if (mainResult != FR_OK)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file for reading: %s", mainFileName);

			return;
		}
		else
		{
			FRESULT newResult = f_open (&_NewFIle, newFileName, FA_WRITE | FA_CREATE_ALWAYS);
		
			if (newResult != FR_OK)
			{
				pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file for writing: %s", newFileName);

				return;
			}
			else
			{
				while (f_gets(buffer, MAX_INPUT_LENGTH, &_ReadFile) != nullptr)
				{
					//pKernel->GetKernelScreenDevice()->Write(buffer,strlen(buffer));
					check = f_puts(buffer,&_NewFIle);

					if (!check)
					{
						pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Failed to write line to file '%s'", newFileName);
					}
				}
			}
		}
		
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file for writing: '%s'", newFileName);
		}
		if (f_close (&_ReadFile) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file for reading: '%s'", mainFileName);
		}

		FRESULT deleteResult = f_unlink(mainFileName);
		if ((deleteResult != FR_OK) && (strcmp("delete", commandName) == 0))
		{
			pKernel->GetKernelScreenDevice()->Write("The original file was not deleted.\n", 26);
		}
	}
    // Reboot
    else if (strcmp("reboot", commandName) == 0)
    {
        pKernel->SystemReboot();
    }
    // Power Off
    else if (strcmp("power", commandName) == 0)
    {
        pKernel->SystemOff();
    }
  	// Display Tasks/Scheduler Demo
	else if (strcmp("displaytasks", commandName) == 0)
  	{
		pKernel->GetKernelScreenDevice()->Write("Enter 's' and return to stop the demo.\n", 39);
		CScheduler::Get ()->CScheduler::turnPrintOn();
	}
	// Stop printing Scheduler Demo
	else if (strcmp("s", commandName) == 0)
  	{
		CScheduler::Get ()->CScheduler::turnPrintOff();
	}
	// Tail
	else if (strcmp("tail", commandName) == 0)
	{
		assert(pKernel != 0);
		//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"We entered head command with |%s| |%s|\n",_commandParameterOne,_commandParameterTwo);
		int _LinesToBeRead=5, _LinesRead=0, _LinesTotal=0;
		if(strcmp(_commandParameterTwo,"")!=0)
		{
			_LinesToBeRead=atoi(_commandParameterTwo);
		}
		//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"_LinesToBeRead is: |%d|\n",_LinesToBeRead);
		char fileName[] = "", buffer[MAX_INPUT_LENGTH]="";
		strcpy(fileName,_directory);
		strcat(fileName,"/");
		strcat(fileName,_commandParameterOne);
		//pKernel->GetKernelScreenDevice()->Write(fileName,strlen(fileName));
		//pKernel->GetKernelScreenDevice()->Write("\n",1);
		FRESULT Result = f_open (&_NewFIle, fileName, FA_READ | FA_OPEN_EXISTING);
		if (Result != FR_OK)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file: %s", fileName);
		}
		while(f_gets(buffer,100,&_NewFIle)!=nullptr)
		{
			_LinesTotal+=1;
		}
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file");
		}
		Result = f_open (&_NewFIle, fileName, FA_READ | FA_OPEN_EXISTING);
		if (Result != FR_OK)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file: %s", fileName);
		}
		while(f_gets(buffer,100,&_NewFIle)!=nullptr)
		{
			if(_LinesRead>=(_LinesTotal-_LinesToBeRead))
			{
				pKernel->GetKernelScreenDevice()->Write(buffer,strlen(buffer));
			}
			_LinesRead+=1;
		}
		//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"_LineTotal is: |%d|\n",_LinesTotal);
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file");
		}

		// Newline at end of 'tail' printouts
		pKernel->GetKernelScreenDevice()->Write ("\n", 1);
	}
	// Write to (file)
	else if (strcmp("writeto", commandName) == 0)
	{
		assert(pKernel != 0);
		int check, numberLines=0;
		// mainFileName -> file you're moving
		// newFileName -> destination you're moving to
		char mainFileName[MAX_DIRECTORY_LENGTH] = "", buffer[MAX_INPUT_LENGTH] = "";
		
		strcpy(mainFileName,_directory);
		strcat(mainFileName,"/");
		strcat(mainFileName,_commandParameterOne);

		FRESULT mainResult = f_open (&_NewFIle, mainFileName, FA_WRITE | FA_OPEN_EXISTING | FA_READ);
		if (mainResult != FR_OK)
		{
			// Attempt to create the file
			FRESULT _tempR = f_open (&_NewFIle, mainFileName, FA_WRITE | FA_READ);

			if (_tempR != FR_EXIST || _tempR == FR_NO_FILE)
			{
				// Last attempt to create the file
				_tempR = f_open(&_NewFIle, mainFileName, FA_WRITE | FA_CREATE_ALWAYS | FA_READ);
			}

			// If we still cannot open it for writing, exit
			if (_tempR != FR_OK)
			{
				pKernel->GetKernelLogger()->Write(_FromKernel, LogWarning, "Cannot open file for writing: '%s'", mainFileName);
				return;
			}
		}

		// Skip to end of file
		while (f_gets(buffer, MAX_INPUT_LENGTH, &_NewFIle) != nullptr)
		{
			numberLines += 1;
		}

		// Append string to file
		strcpy(buffer, _commandParameterTwo);
		check = f_puts(buffer, &_NewFIle); 
		check = f_puts("\n",&_NewFIle);

		if (check == 0)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot write to file '%s'", mainFileName);
		}

		// Cleanup
		if (f_close (&_NewFIle) != FR_OK)
		{
			pKernel->GetKernelLogger()->Write (_FromKernel, LogWarning, "Cannot close file for writing '%s'", mainFileName);
		}
	}
	// Change the Username color
	else if (strcmp("usertext",commandName)==0)
	{
		assert(pKernel != 0);
		char temp[PMAX_INPUT_LENGTH]="", digits[PMAX_INPUT_LENGTH]="";
		strcpy(temp,_commandParameterOne);
		int index = 0, sub = 0;
		for(int x=0;x<3;x++)
		{
			//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"Entered the for loop with %d",x);
			while (index < (int)strlen(temp))
			{
				if (temp[index] == ',')
					break;
				//pKernel->GetKernelLogger()->Write(_FromKernel,LogDebug,"Entered the while loop with temp[%d]=%c",index,temp[index]);
				digits[sub] = temp[index];
				sub++, index++;
			}

			digits[sub] = '\0';
			if(x==0)
				userRed=atoi(digits);
			if(x==1)
				userGreen=atoi(digits);
			if(x==2)
				userBlue=atoi(digits);
			sub=0,index++;
			strcpy(digits,"");
		}
		SetColor(userRed,userGreen,userBlue);
	}
	// System Info
	else if (strcmp("systeminfo", commandName) == 0)
	{
		CMachineInfo* _info = pKernel->GetKernelInfo();

		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Device: %s", _info->GetMachineName());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "SoC: %s", _info->GetSoCName());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Model #: %i", _info->GetModelMajor());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "RAM: %iGB", _info->GetRAMSize());

		unsigned int Hz = _info->GetClockRate(CLOCK_ID_ARM);
		unsigned int kHz = Hz / 1000;
		unsigned int mHz = kHz / 1000;
		unsigned int gHz = mHz / 1000;

		if (gHz >= 1)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Clock speed: %i GHz", gHz);
		}
		else if (mHz >= 1)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Clock speed: %i MHz", mHz);
		}
		else if (kHz >= 1)
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Clock speed: %i KHz", kHz);
		}
		else
		{
			pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Clock speed: %i Hz", Hz);
		}
		
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Clock speed: %i", _info->GetClockRate(CLOCK_ID_ARM));
	}
	// Terminate Task (at top of stack)
	else if (strcmp("terminatetask", commandName) == 0)
	{
		// Scrapped for now!
		// CScheduler::Get ()->CScheduler::PopTask();
	}
	// (Display) Current Tasks
	else if (strcmp("currenttasks", commandName) == 0)
	{
		// Flip this to 1 to use the logger instead
		int useLogger = 0;

		if (!useLogger)
		{
			CString temp = CScheduler::Get ()->CScheduler::listTasks();
			pKernel->GetKernelScreenDevice()->Write(temp, temp.GetLength());
		}
		else
		{
			CScheduler::Get()->ListTasks();
		}

		// Newline after 'currenttasks' command
		pKernel->GetKernelScreenDevice()->Write("\n", 1);
	}
	// Memory System Demo
	else if (strcmp("memorystats", commandName) == 0)
	{
		size_t heapLow = pKernel->GetKernelMemory()->GetHeapFreeSpace(HEAP_LOW); // Bits or Bytes?
		size_t heapHigh = pKernel->GetKernelMemory()->GetHeapFreeSpace(HEAP_HIGH); // Bits or Bytes?
		size_t heapAny = pKernel->GetKernelMemory()->GetHeapFreeSpace(HEAP_ANY); // Bits or Bytes?
		size_t memSize = pKernel->GetKernelMemory()->GetMemSize(); // Bits or Bytes?
		size_t _KB, _MB, _GB;
		int useLogger = 0;

		CString statString, _tempS;

		_KB = heapLow / 1000;
		_MB = _KB / 1000;
		_GB = _MB / 1000;
		if (useLogger)
		{
			if (_GB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %lu GB", _GB);
			else if (_MB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %lu MB", _MB);
			else if (_KB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %lu KB", _KB);
			else
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %lu B", heapLow);
		}
		else
		{
			statString.Format("Heap Free Space (Low): ");

			if (_GB >= 1)
				_tempS.Format("%lu GB\n", _GB);
			else if (_MB >= 1)
				_tempS.Format("%lu MB\n", _MB);
			else if (_KB >= 1)
				_tempS.Format("%lu KB\n", _KB);
			else
				_tempS.Format("%lu B\n", heapLow);
			
			statString.Append((const char*)_tempS);
			pKernel->GetKernelScreenDevice()->Write((const char*)statString, statString.GetLength());
		}
		

		_KB = heapHigh / 1000;
		_MB = _KB / 1000;
		_GB = _MB / 1000;
		if (useLogger)
		{
			if (_GB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (High): %lu GB", _GB);
			else if (_MB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (High): %lu MB", _MB);
			else if (_KB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (High): %lu KB", _KB);
			else
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (High): %lu B", heapHigh);
		}
		else
		{
			statString.Format("Heap Free Space (High): ");

			if (_GB >= 1)
				_tempS.Format("%lu GB\n", _GB);
			else if (_MB >= 1)
				_tempS.Format("%lu MB\n", _MB);
			else if (_KB >= 1)
				_tempS.Format("%lu KB\n", _KB);
			else
				_tempS.Format("%lu B\n", heapHigh);
			
			statString.Append((const char*)_tempS);
			pKernel->GetKernelScreenDevice()->Write((const char*)statString, statString.GetLength());
		}

		_KB = heapAny / 1000;
		_MB = _KB / 1000;
		_GB = _MB / 1000;
		if (useLogger)
		{
			if (_GB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Any): %lu GB", _GB);
			else if (_MB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Any): %lu MB", _MB);
			else if (_KB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Any): %lu KB", _KB);
			else
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Any): %lu B", heapAny);
		}
		else
		{
			statString.Format("Heap Free Space (Any): ");

			if (_GB >= 1)
				_tempS.Format("%lu GB\n", _GB);
			else if (_MB >= 1)
				_tempS.Format("%lu MB\n", _MB);
			else if (_KB >= 1)
				_tempS.Format("%lu KB\n", _KB);
			else
				_tempS.Format("%lu B\n", heapAny);
			
			statString.Append((const char*)_tempS);
			pKernel->GetKernelScreenDevice()->Write((const char*)statString, statString.GetLength());
		}

		_KB = memSize / 1000;
		_MB = _KB / 1000;
		_GB = _MB / 1000;
		if (useLogger)
		{
			if (_GB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Memory Blocks: %lu GB", _GB);
			else if (_MB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Memory Blocks: %lu MB", _MB);
			else if (_KB >= 1)
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Memory Blocks: %lu KB", _KB);
			else
				pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Memory Blocks: %lu B", memSize);
		}
		else
		{
			statString.Format("Memory Blocks: ");

			if (_GB >= 1)
				_tempS.Format("%lu GB\n", _GB);
			else if (_MB >= 1)
				_tempS.Format("%lu MB\n", _MB);
			else if (_KB >= 1)
				_tempS.Format("%lu KB\n", _KB);
			else
				_tempS.Format("%lu B\n", memSize);
			
			statString.Append((const char*)_tempS);
			pKernel->GetKernelScreenDevice()->Write((const char*)statString, statString.GetLength());
		}

		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %i", heapLow);
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Heap Free Space (Low): %i", heapLow);
	}

	// Secret Command shhhhh
	else if (strcmp("dumpaddr", commandName) == 0)
	{
		// Big Address Dump
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Kernel Addr: %x", pKernel);
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Shell Addr: %x", s_pThis);
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "ActLED Addr: %x", pKernel->GetKernelActLED());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "DeviceNameService Addr: %x", pKernel->GetKernelDNS());
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "EMMC Addr: %x", &m_EMMC);
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Event Addr: %x", pKernel->GetKernelSyncEvent());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "ExceptionHandler Addr: %x", pKernel->GetKernelExceptionHandler());
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Filesystem Addr: %x", &m_FileSystem);
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "MachineInfo Addr: %x", pKernel->GetKernelInfo());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Interrupt Addr: %x", pKernel->GetKernelInterruptSystem());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Logger Addr: %x", pKernel->GetKernelLogger());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Memory Addr: %x", pKernel->GetKernelMemory());
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Options Addr: %x", &m_Options);
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Scheduler Addr: %x", pKernel->GetKernelScheduler());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Screen Addr: %x", pKernel->GetKernelScreenDevice());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Serial Addr: %x", pKernel->GetKernelSerialDevice());
		pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "Timer Addr: %x", pKernel->GetKernelTimer());
		// pKernel->GetKernelLogger()->Write(_FromKernel, LogNotice, "USBHCI Addr: %x", &m_USBHCI);
	}

	memset(_mainCommandName, 0, sizeof(_mainCommandName));
	memset(_commandParameterOne, 0, sizeof(_commandParameterOne));
	memset(_commandParameterTwo, 0, sizeof(_commandParameterTwo));
	memset(_message, 0, sizeof(_message));
}

void PShell::DisplayUserWithDirectory()
{
	char currentLine[PMAX_DIRECTORY_LENGTH];
	strcpy(currentLine,_userName);
	SetColor(userRed,userGreen,userBlue);
	pKernel->GetKernelScreenDevice()->Write(currentLine, strlen(currentLine));
	strcpy(currentLine, "@RasberryPI:");
	strcat(currentLine,_directory);
	strcat(currentLine,"$ ");
	SetColor(dirRed,dirGreen,dirBlue);
	//strcat(currentLine,getcwd());
	pKernel->GetKernelScreenDevice()->Write(currentLine, strlen(currentLine));
	SetColor(31,31,31);
}

void PShell::EditUserName(const char *loginName)
{
	strcpy(_directory, "SD:/users/");
	strcat(_directory, loginName);
	strcat(_directory, "/desktop"); // Start in user's desktop
	strcpy(_userName,loginName);
}

void PShell::FixWorkingDirectory()
{
	int index = 0, currentAmount = 0, amountSlash = 0, length = strlen(_directory), amountDot = 0;
	char temp[MAX_INPUT_LENGTH] = "";

	// Debug
	// pKernel->GetKernelLogger()->Write(_FromKernel, LogDebug, "The directory variable was: |%s|", _directory);
	
	// Counts the number of occurences of ".."
	while (index < length)
	{
		if (_directory[index] == '.')
		{
			if (_directory[index+1] == '.')
			{
				amountDot += 1;
			}
		}
		index++;
	}
	
	index = 0;
	// Counts the number of occurences of "/"
	while (index < length)
	{
		if (_directory[index] == '/')
		{
			amountSlash += 1;
		}
		index++;
	}

	strcpy(temp, _directory);

	// If the number of slashes and dots are equal, clear to root
	if (amountSlash == amountDot)
	{
		strcpy(_directory, "SD:");
		return;
	}

	for (int i = 1; i <= amountDot; i++)
	{
		index = 0;
		currentAmount = 0;

		// Offset by 1, handles amounts of slashes greater than dot pairs
		while (currentAmount < (amountSlash - amountDot + 1) - i)
		{
			if (temp[index] == '/')
				currentAmount++;
			
			index++;
		}
		temp[index-1] = '\0';
	}

	strcpy(_directory, temp);

	// Debug
	// pKernel->GetKernelLogger()->Write(_FromKernel, LogDebug, "The directory variable is: |%s|",_directory);
}

void PShell::SetColor(int red, int green, int blue)
{
	color = COLOR16(red, green, blue); 	
	stat = pKernel->GetKernelScreenDevice()->GetStatus();
	stat.Color = color;

	if (pKernel->GetKernelScreenDevice()->SetStatus(stat) == false)
	{
		pKernel->GetKernelLogger()->Write(_FromKernel, LogDebug, "Screen stat just returned false");
	}
}