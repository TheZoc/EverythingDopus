#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	/**
	 * Query Windows Registry and return a new string with the path to DopusRT.exe
	 * Remember to free this after use!
	 */
	LPTSTR GetDopusRTPath();

	/**
	 * Generates a new time file in a valid temp folder.
	 * Remember to free this after use!
	 */
	TCHAR* GenerateTempFile();

	/**
	 * Attempt to query Everything version to check if it's running.
	 */
	BOOL IsEverythingRunning();

	/**
	 * Parse the command line arguments and build a search string
	 * Remember to free this after use!
	 */
	TCHAR* BuildSearchRequest(int argc, TCHAR* argv[]);

	/**
	 * Prepare everything SetSearch request.
	 * This function checks for SearchEverythingCoreCLI regex style strings, for compatibility purposes.
	 */
	void EverythingSearch(TCHAR* searchString);

	//void RetrieveEverythingResults(TCHAR* tempFilePath);

	/**
	 * Retrieve Everything results and save to target file
	 */
	void RetrieveAndSaveSearchToFile(TCHAR* outFilepath);

	/**
	 * Retrieve the file list with the full path from Everything
	 * and return a filechunk structure with the data converted to UTF-8 (when necessary)
	 */
	struct s_filechunk* RetrieveEverythingResults();

	/**
	 * Writes the filechunk data to the target file
	 */
	void WriteFilechunkToFile(TCHAR* outFilepath, struct s_filechunk* filechunk);

	/**
	 * Build dopusrt.exe command line in the format:
	 * "C:\Program Files\GPSoftware\Directory Opus\dopusrt.exe" /col import /clear /create /nocheck Everything "%temp%\tempfile"
	 */
	TCHAR* DopusPrepareCollection(LPTSTR dopusPath, TCHAR* collectionFilepath);

	/**
	 * Show collection imported to Directory Opus. Format:
	 * "C:\Program Files\GPSoftware\Directory Opus\dopusrt.exe" /cmd go path=coll://Everything/
	 */
	TCHAR* DopusShowCollection(LPTSTR dopusPath);

	/**
	 * Execute commands with a hidden console
	 * Only used to Dopusrt.exe commands
	 */
	BOOL RunSyncHiddenApp(TCHAR* commandLine);

#ifdef __cplusplus
};
#endif // __cplusplus

