// EverythingDopusCLI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <WinUser.h>
#include "utils.h"
#include "Everything.h"

// Enable newer ComCtl32 visual styles
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef UNICODE
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wmainCRTStartup")
#else
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#define APP_NAME                TEXT("EverythingDopusCLI")
#define APP_VERSION             TEXT("v1.0")
#define APP_COPYRIGHT           TEXT("© 2023 Felipe Guedes da Silveira")
#define APP_URL                 TEXT("https://github.com/TheZoc/EverythingDopus")
#define EDC_ERROR_BUFFER_SIZE   1024
#define EV_RESULT_COUNT_WARNING	1000


int _tmain(int argc, TCHAR* argv[])
{
	TCHAR errorBuffer[EDC_ERROR_BUFFER_SIZE * sizeof(TCHAR)];
	if (!IsEverythingRunning())
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nEverything application isn't running.\nStart it and try again."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONWARNING | MB_OK);
		return -1;
	}

	LPTSTR dopusPath = GetDopusRTPath();
	if (!dopusPath)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nUnable to find Directory Opus path in Windows Registry.\nReinstalling it might fix the issue."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONWARNING | MB_OK);
		return -1;
	}

	if (argc < 2)
	{
		TCHAR* exeName = _tcsrchr(argv[0], TEXT('\\')) + 1;
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE,
			TEXT("%s %s\n")
			TEXT("%s\n")
			TEXT("=====================\n\n")
			TEXT("You need to provide a search string:\n\n")
			TEXT("%s search string\n")
			TEXT("%s /regex search string/\n\n")
			TEXT("Or setup Directory Opus with the toolbar button.\n\n")
			TEXT("Would you like to visit the website for more info?\n")
			, APP_NAME, APP_VERSION, APP_COPYRIGHT, exeName, exeName);
		if (MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
		{
			ShellExecute(NULL, TEXT("open"), APP_URL, NULL, NULL, SW_SHOWNORMAL);
		}

		return 0;
	}

	TCHAR* searchString = BuildSearchRequest(argc, argv);
	if (!searchString)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nOut of memory when building the search string."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Do the search!
	EverythingSearch(searchString);
	free(searchString);

	// Check to see if we got way too many results - and ask for user confirmation
	DWORD ResultCount = Everything_GetTotResults();
	if (ResultCount > EV_RESULT_COUNT_WARNING)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nNumber of objects found: %lu\nDo you want to continue and open this in Directory Opus?"), APP_NAME, APP_VERSION, ResultCount);
		if (MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO)
		{
			Everything_CleanUp();
			return 0;
		}
	}

	// Create a temporary to hold the list of files to be sent to directory opus
	TCHAR* tempFilePath = GenerateTempFile();
	if (!tempFilePath)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nError trying to allocate an intermediary collection file."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Save file list to a temp file
	RetrieveAndSaveSearchToFile(tempFilePath);

	// Prepare import collection dopus command line
	TCHAR* commandLine = DopusPrepareCollection(dopusPath, tempFilePath);
	if (!commandLine)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nOut of memory when attempting to import the collection."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Run Dopus prepare collection
	if (!RunSyncHiddenApp(commandLine))
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nUnable to import collection into Directory Opus."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(commandLine);

	// Prepare show collection dopus command line
	commandLine = DopusShowCollection(dopusPath);
	if (!commandLine)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nOut of memory when attempting to show the collection."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(dopusPath);

	// Run dopus show collection
	if (!RunSyncHiddenApp(commandLine))
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("%s %s\n================\n\nUnable to show collection in Directory Opus."), APP_NAME, APP_VERSION);
		MessageBox(NULL, errorBuffer, APP_NAME, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(commandLine);

	Everything_CleanUp();

	// Delete the temporary file as the last action in the program
	DeleteFile(tempFilePath);
	free(tempFilePath);

	return 0;
}
