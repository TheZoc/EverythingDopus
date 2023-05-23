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

#define APP_NAME					TEXT("EverythingDopusCLI")
#define APP_VERSION					TEXT("v2.0 beta4")
#define APP_TITLE					APP_NAME TEXT(" ") APP_VERSION
#define APP_COPYRIGHT				TEXT("© 2023 Felipe Guedes da Silveira")
#define APP_URL						TEXT("https://github.com/TheZoc/EverythingDopus")
#define EDC_ERROR_BUFFER_SIZE		1024
#define EV_RESULT_COUNT_WARNING		1000
#define MAX_COLLECTION_NAME_SIZE	200


int _tmain(int argc, TCHAR* argv[])
{
	TCHAR errorBuffer[EDC_ERROR_BUFFER_SIZE];
	if (!IsEverythingRunning())
	{
		MessageBox(NULL, TEXT("Everything application isn't running.\nStart it and try again."), APP_TITLE, MB_ICONWARNING | MB_OK);
		return -1;
	}

	LPTSTR dopusPath = GetDopusRTPath();
	if (!dopusPath)
	{
		MessageBox(NULL, TEXT("Unable to find Directory Opus path in Windows Registry.\nReinstalling it might fix the issue."), APP_TITLE, MB_ICONWARNING | MB_OK);
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
		if (MessageBox(NULL, errorBuffer, APP_TITLE, MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
		{
			ShellExecute(NULL, TEXT("open"), APP_URL, NULL, NULL, SW_SHOWNORMAL);
		}

		return 0;
	}

	// Use custom version of CommandLineToArgv, to keep the quotes from command line
	int argc_quoted;
	LPWSTR* argv_quoted = CommandLineToArgvKeepQuotes(GetCommandLine(), &argc_quoted);
	TCHAR* searchString = BuildSearchRequest(argc_quoted, argv_quoted);
	LocalFree(argv_quoted);
	if (!searchString)
	{
		MessageBox(NULL, TEXT("Out of memory when building the search string."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Check if it's a regex search, and remove the slashes at the start and end of the string
	BOOL regexSearch = CleanRegexSearchString(searchString);

	// Do the search!
	EverythingSearch(searchString);

	// Check to see if we got way too many results - and ask for user confirmation
	DWORD ResultCount = Everything_GetTotResults();
	if (!ResultCount)
	{
		MessageBox(NULL, TEXT("No results were found."), APP_TITLE, MB_ICONINFORMATION | MB_OK);
		Everything_CleanUp();
		return 0;
	}

	if (ResultCount > EV_RESULT_COUNT_WARNING)
	{
		_sntprintf_s(errorBuffer, EDC_ERROR_BUFFER_SIZE, EDC_ERROR_BUFFER_SIZE, TEXT("Number of objects found: %lu\nDo you want to continue and open this in Directory Opus?"), ResultCount);
		if (MessageBox(NULL, errorBuffer, APP_TITLE, MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO)
		{
			Everything_CleanUp();
			return 0;
		}
	}

	// Create a temporary to hold the list of files to be sent to directory opus
	TCHAR* tempFilePath = GenerateTempFile();
	if (!tempFilePath)
	{
		MessageBox(NULL, TEXT("Error trying to allocate an intermediary collection file."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Save file list to a temp file
	RetrieveAndSaveSearchToFile(tempFilePath);

	// Build Collection name: YYYY-MM-DD HH-MM-SS (regex) search string:
	TCHAR* collectionName = BuildCollectionString(regexSearch, searchString);
	if (!collectionName)
	{
		MessageBox(NULL, TEXT("Out of memory when attempting to create the collection name."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(searchString);

	// Prepare import collection dopus command line
	TCHAR* commandLine = DopusPrepareCollection(dopusPath, collectionName, tempFilePath);
	if (!commandLine)
	{
		MessageBox(NULL, TEXT("Out of memory when attempting to import the collection."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}

	// Run Dopus prepare collection
	if (!RunSyncHiddenApp(commandLine))
	{
		MessageBox(NULL, TEXT("Unable to import collection into Directory Opus."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(commandLine);

	// Prepare show collection dopus command line
	commandLine = DopusShowCollection(dopusPath, collectionName);
	if (!commandLine)
	{
		MessageBox(NULL, TEXT("Out of memory when attempting to show the collection."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(dopusPath);

	// Run dopus show collection
	if (!RunSyncHiddenApp(commandLine))
	{
		MessageBox(NULL, TEXT("Unable to show collection in Directory Opus."), APP_TITLE, MB_ICONERROR | MB_OK);
		return -1;
	}
	free(commandLine);
	free(collectionName);

	Everything_CleanUp();

	// Delete the temporary file as the last action in the program
	DeleteFile(tempFilePath);
	free(tempFilePath);

	return 0;
}
