#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <Windows.h>
#include "utils.h"
#include "Everything.h"

#define MAX_PATH_UNICODE                    (MAX_PATH * 10)
#define EDC_PROCESS_COMMAND_LINE_MAX_SIZE   512

/**
 * Define the basic filechunk structure and forward declaration
 * of the functions for our extremely basic filechunk functions
 **/
struct s_filechunk
{
	char* data;
	size_t remaining_size;
	struct s_filechunk* next;
};

#define FILECHUNK_SIZE 32 * 1024
static struct s_filechunk* NewFileChunk(struct s_filechunk* previous_filechunk);
static size_t AddToFilechunk(struct s_filechunk* filechunk, char* data, size_t data_size);
static void HandleFileChunkAvailableSpace(struct s_filechunk** filechunk);
static void FreeFileChunkChain(struct s_filechunk** filechunk);


LPTSTR GetDopusRTPath()
{
#define DOPUSRT TEXT("\\dopusrt.exe")
	LPTSTR dopusPath = NULL;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\DOpus.exe"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return NULL;
	}

	DWORD dwType;
	DWORD dwDataSize;
	if (RegQueryValueEx(hKey, TEXT("Path"), NULL, &dwType, NULL, &dwDataSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return NULL;
	}

	if (dwType != REG_SZ)
	{
		RegCloseKey(hKey);
		return NULL;
	}

	const size_t dwBufSize = (size_t)dwDataSize + (_tcslen(DOPUSRT) * sizeof(TCHAR));
	dopusPath = (LPTSTR)malloc(dwBufSize);
	if (dopusPath == NULL)
	{
		RegCloseKey(hKey);
		return NULL;
	}

	if (RegQueryValueEx(hKey, TEXT("Path"), NULL, &dwType, (LPBYTE)(dopusPath), &dwDataSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		free(dopusPath);
		dopusPath = NULL;
		return NULL;
	}

	// DopusRT is already a null-terminated string, so we kill two birds with one stone
	_tcscpy_s(dopusPath + dwDataSize / sizeof(TCHAR) - 1, (dwBufSize - dwDataSize) / sizeof(TCHAR) + 1, DOPUSRT);

	RegCloseKey(hKey);
	return dopusPath;
#undef DOPUSRT
}

TCHAR* GenerateTempFile()
{
	// Get the temp file path and temp file name
	TCHAR tempFilePath[MAX_PATH_UNICODE];
	GetTempPath(MAX_PATH_UNICODE, tempFilePath);
	GetTempFileName(tempFilePath, TEXT("edc"), 0, tempFilePath);

	// Allocate a new string and copy data over
	size_t len = _tcslen(tempFilePath) + 1;
	TCHAR* newString = (TCHAR*)malloc((len) * sizeof(TCHAR));
	if (!newString) return NULL;
	_tcscpy_s(newString, len, tempFilePath);
	newString[len - 1] = '\0';

	return newString;
}

BOOL IsEverythingRunning()
{
	return Everything_GetMajorVersion() > 0 && Everything_GetMinorVersion() > 0;
}

TCHAR* BuildSearchRequest(int argc, TCHAR* argv[])
{
	// Calculate the search string buffer size
	size_t argsLength = 0;
	for (int i = 1; i < argc; ++i)
	{
		// Add 1 for the space between args, and for the final terminator.
		argsLength += _tcslen(argv[i]) + 1;
	}

	// Allocate the buffer
	TCHAR* searchString = (TCHAR*)malloc(argsLength * sizeof(TCHAR));
	if (!searchString)
		return NULL;

	// Build the string, adding a space between args and a null character at the end of the string.
	size_t pos = 0;
	for (size_t i = 1; i < (size_t)argc; ++i)
	{
		_tcscpy_s(searchString + pos, argsLength - pos, argv[i]);
		pos += _tcslen(argv[i]);
		searchString[pos++] = TEXT(' ');
	}
	searchString[pos - 1] = '\0';
	return searchString;
}

BOOL CleanRegexSearchString(TCHAR* searchString)
{
	if (!searchString)
		return FALSE;

	const size_t len = _tcslen(searchString);
	if (len > 2 && searchString[0] == '/' && searchString[len - 1] == '/')
	{
		memmove_s(searchString, (len + 1) * sizeof(TCHAR), searchString + 1, (len) * sizeof(TCHAR));
		searchString[len - 2] = searchString[len - 1] = '\0';
		Everything_SetRegex(TRUE);
		return TRUE;
	}

	return FALSE;
}

void EverythingSearch(TCHAR* searchString)
{
	Everything_SetSearch(searchString);
	Everything_Query(TRUE);
}

void RetrieveAndSaveSearchToFile(TCHAR* outFilepath)
{
	struct s_filechunk* searchResultFilechunk = RetrieveEverythingResults();
	WriteFilechunkToFile(outFilepath, searchResultFilechunk);
	FreeFileChunkChain(&searchResultFilechunk);
}

struct s_filechunk* RetrieveEverythingResults()
{
	struct s_filechunk* root = NewFileChunk(NULL);
	if (!root)
		return NULL;

	DWORD EverythingNumResults = Everything_GetNumResults();
	if (EverythingNumResults > 0)
	{
		struct s_filechunk* filechunk = root;
		TCHAR buffer[MAX_PATH_UNICODE];
		for (DWORD i = 0; i < EverythingNumResults; ++i)
		{
			HandleFileChunkAvailableSpace(&filechunk);
			Everything_GetResultFullPathName(i, buffer, MAX_PATH_UNICODE);

#ifdef UNICODE
			size_t unicodeLen = _tcslen(buffer);
			size_t utf8size = WideCharToMultiByte(CP_UTF8, 0, buffer, (int)unicodeLen, NULL, 0, NULL, NULL);
			char* utf8buffer = (char*)malloc(utf8size);
			if (utf8buffer)
			{
				WideCharToMultiByte(CP_UTF8, 0, buffer, (int)unicodeLen, utf8buffer, (int)utf8size, NULL, NULL);
				size_t written = AddToFilechunk(filechunk, utf8buffer, utf8size);
				if (written < utf8size)
				{
					assert(filechunk->remaining_size == 0);
					HandleFileChunkAvailableSpace(&filechunk);
					AddToFilechunk(filechunk, utf8buffer + written, utf8size - written);
				}

				HandleFileChunkAvailableSpace(&filechunk);
				AddToFilechunk(filechunk, "\n", 1);
				free(utf8buffer);
			}
#else
			size_t len = _tcslen(buffer);
			size_t written = AddToFileChunk(filechunk, buffer, len);
			if (written < len)
			{
				assert(filechunk->remaining_size == 0);
				HandleFileChunkAvailableSpace(&filechunk);
				size_t written2 = AddToFileChunk(filechunk, buffer + written, len - written);
				assert(len == (written + written2));
			}

			HandleFileChunkAvailableSpace(&filechunk);
			AddToFileChunk(filechunk, "\n", 1);
#endif
		}
	}
	return root;
}

void WriteFilechunkToFile(TCHAR* outFilepath, struct s_filechunk* filechunk)
{
	FILE* pFile = NULL;
	errno_t err = 0;
	if ((err = _tfopen_s(&pFile, outFilepath, TEXT("wb"))) == 0 && pFile)
	{
		struct s_filechunk* fc = filechunk;
		while (fc)
		{
			fwrite(fc->data, sizeof(char), FILECHUNK_SIZE - fc->remaining_size, pFile);
			fc = fc->next;
		}
		fclose(pFile);
	}
	else
	{
		// TODO: Move this to an outer scope
		TCHAR errorBuffer[2048];
		_sntprintf_s(errorBuffer, 2048, 2048, TEXT("EverythingDopusCLI\n================\n\nUnable to write intermediary file:\nError: %d"), (int)err);

		MessageBox(NULL, errorBuffer, TEXT("EverythingDopusCLI"), MB_ICONWARNING | MB_OK);

	}
}

TCHAR* BuildDateString()
{
	// (YYYY-MM-DD HH-MM-SS)
	const int bufferSize = 22;
	TCHAR* outDate = (TCHAR*)malloc(bufferSize * sizeof(TCHAR));
	if (!outDate)
		return NULL;

	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	_tcsftime(outDate, bufferSize, TEXT("(%Y-%m-%d %H-%M-%S)"), &timeinfo);
	return outDate;
}

TCHAR* BuildCollectionString(BOOL isRegex, TCHAR* searchString)
{
	// Build the date string
	TCHAR* dateString = BuildDateString();

	// Needed buffer size
	const size_t dateSize = _tcslen(dateString);
	const size_t regexSize = isRegex ? 8 : 0;
	const size_t searchStringSize = _tcslen(searchString);
	const size_t bufferSize = dateSize + regexSize + searchStringSize + 3; // 2 spaces + 1 null terminator

	// Allocate new string
	TCHAR* collectionName = (TCHAR*)malloc(bufferSize * sizeof(TCHAR));
	if (!collectionName)
		return NULL;

	// Copy the date string
	size_t counter = dateSize;
	_tcscpy_s(collectionName, bufferSize, dateString);
	collectionName[counter++] = TEXT(' ');

	// Copy the regex string if necessary
	if (isRegex)
	{
		_tcscpy_s(collectionName + counter, bufferSize - counter, TEXT("(regex) "));
		counter += 8;
	}

	// Copy the search string
	_tcscpy_s(collectionName + counter, bufferSize - counter, searchString);

	// Remove invalid characters from the collection name
	TCHAR* pCollectionName = collectionName;
	while (*pCollectionName != TEXT('\0'))
	{
		if (*pCollectionName == '<'
			|| *pCollectionName == '>'
			|| *pCollectionName == ':'
			|| *pCollectionName == '"'
			|| *pCollectionName == '/'
			|| *pCollectionName == '\\'
			|| *pCollectionName == '|'
			|| *pCollectionName == '?'
			|| *pCollectionName == '*')
		{
			*pCollectionName = TEXT('_');
		}
		++pCollectionName;
	}

	free(dateString);
	return collectionName;
}

TCHAR* DopusPrepareCollection(LPTSTR dopusPath, TCHAR* collectionName, TCHAR* collectionFilepath)
{
	TCHAR* commandLine = (TCHAR*)malloc(EDC_PROCESS_COMMAND_LINE_MAX_SIZE * sizeof(TCHAR));
	if (!commandLine)
		return NULL;

	ZeroMemory(commandLine, EDC_PROCESS_COMMAND_LINE_MAX_SIZE * sizeof(TCHAR));
	_sntprintf_s(commandLine, EDC_PROCESS_COMMAND_LINE_MAX_SIZE, EDC_PROCESS_COMMAND_LINE_MAX_SIZE, TEXT("\"%s\" /col import /utf8 /clear /create /nocheck \"EverythingDopus/%s\" \"%s\""), dopusPath, collectionName, collectionFilepath);
	return commandLine;
}

TCHAR* DopusShowCollection(LPTSTR dopusPath, TCHAR* collectionName)
{
	TCHAR* commandLine = (TCHAR*)malloc(EDC_PROCESS_COMMAND_LINE_MAX_SIZE * sizeof(TCHAR));
	if (!commandLine)
		return NULL;

	ZeroMemory(commandLine, EDC_PROCESS_COMMAND_LINE_MAX_SIZE * sizeof(TCHAR));
	_sntprintf_s(commandLine, EDC_PROCESS_COMMAND_LINE_MAX_SIZE, EDC_PROCESS_COMMAND_LINE_MAX_SIZE, TEXT("\"%s\" /acmd Go NEWTAB findexisting path=\"coll://EverythingDopus/%s\""), dopusPath, collectionName);

	return commandLine;
}

BOOL RunSyncHiddenApp(TCHAR* commandLine)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, commandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		return FALSE;

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return TRUE;
}

LPWSTR* CommandLineToArgvKeepQuotesW(const LPWSTR cmdline, int* numargs)
{
	int qcount, bcount;
	LPCWSTR s;
	LPWSTR* argv;
	DWORD argc;
	LPWSTR d;

	if (!numargs)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (*cmdline == 0)
	{
		/* Return the path to the executable */
		DWORD len, deslen = MAX_PATH, size;

		size = sizeof(LPWSTR) * 2 + deslen * sizeof(WCHAR);
		for (;;)
		{
			argv = LocalAlloc(LMEM_FIXED, size);
			if (!argv) return NULL;
			len = GetModuleFileNameW(0, (LPWSTR)(argv + 2), deslen);
			if (!len)
			{
				LocalFree(argv);
				return NULL;
			}
			if (len < deslen) break;
			deslen *= 2;
			size = sizeof(LPWSTR) * 2 + deslen * sizeof(WCHAR);
			LocalFree(argv);
		}
		argv[0] = (LPWSTR)(argv + 2);
		argv[1] = NULL;
		*numargs = 1;

		return argv;
	}

	/* --- First count the arguments */
	argc = 1;
	s = cmdline;
	/* The first argument, the executable path, follows special rules */
	if (*s == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s++;
		while (*s)
			if (*s++ == '"')
				break;
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*s && *s != ' ' && *s != '\t')
			s++;
	}
	/* skip to the first argument, if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s)
		argc++;

	/* Analyze the remaining arguments */
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* skip to the next argument and count it if any */
			while (*s == ' ' || *s == '\t')
				s++;
			if (*s)
				argc++;
			bcount = 0;
		}
		else if (*s == '\\')
		{
			/* '\', count them */
			bcount++;
			s++;
		}
		else if (*s == '"')
		{
			/* '"' */
			if ((bcount & 1) == 0)
				qcount++; /* unescaped '"' */
			s++;
			bcount = 0;
			/* consecutive quotes, see comment in copying code below */
			while (*s == '"')
			{
				qcount++;
				s++;
			}
			qcount = qcount % 3;
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			bcount = 0;
			s++;
		}
	}

	/* Allocate in a single lump, the string array, and the strings that go
	 * with it. This way the caller can make a single LocalFree() call to free
	 * both, as per MSDN.
	 */
	argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPWSTR) + (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
	if (!argv)
		return NULL;

	/* --- Then split and copy the arguments */
	argv[0] = d = lstrcpyW((LPWSTR)(argv + argc + 1), cmdline);
	argc = 1;
	/* The first argument, the executable path, follows special rules */
	if (*d == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s = d + 1;
		while (*s)
		{
			if (*s == '"')
			{
				s++;
				break;
			}
			*d++ = *s++;
		}
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*d && *d != ' ' && *d != '\t')
			d++;
		s = d;
		if (*s)
			s++;
	}
	/* close the executable path */
	*d++ = 0;
	/* skip to the first argument and initialize it if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (!*s)
	{
		/* There are no parameters so we are all done */
		argv[argc] = NULL;
		*numargs = argc;
		return argv;
	}

	/* Split and copy the remaining arguments */
	argv[argc++] = d;
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* close the argument */
			*d++ = 0;
			bcount = 0;

			/* skip to the next one and initialize it if any */
			do {
				s++;
			} while (*s == ' ' || *s == '\t');
			if (*s)
				argv[argc++] = d;
		}
		else if (*s == '\\')
		{
			*d++ = *s++;
			bcount++;
		}
		else if (*s == '"')
		{
			if ((bcount & 1) == 0)
			{
				/* Preceded by an even number of '\', this is half that
				 * number of '\', plus a quote which we erase.
				 */
				d -= bcount / 2;
				qcount++;
			}
			else
			{
				/* Preceded by an odd number of '\', this is half that
				 * number of '\' followed by a '"'
				 */
				d = d - bcount / 2 - 1;
				*d++ = '"';
			}
			*d++ = *s++;	// This is what makes it keep the quotes. Originally: s++
			bcount = 0;
			/* Now count the number of consecutive quotes. Note that qcount
			 * already takes into account the opening quote if any, as well as
			 * the quote that lead us here.
			 */
			while (*s == '"')
			{
				if (++qcount == 3)
				{
					*d++ = '"';
					qcount = 0;
				}
				s++;
			}
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			*d++ = *s++;
			bcount = 0;
		}
	}
	*d = '\0';
	argv[argc] = NULL;
	*numargs = argc;

	return argv;
}

LPSTR* CommandLineToArgvKeepQuotesA(const LPSTR cmdline, int* numargs)
{
	int qcount, bcount;
	LPCSTR s;
	LPSTR* argv;
	DWORD argc;
	LPSTR d;

	if (!numargs)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (*cmdline == 0)
	{
		/* Return the path to the executable */
		DWORD len, deslen = MAX_PATH, size;

		size = sizeof(LPSTR) * 2 + deslen * sizeof(WCHAR);
		for (;;)
		{
			argv = LocalAlloc(LMEM_FIXED, size);
			if (!argv) return NULL;
			len = GetModuleFileNameA(0, (LPSTR)(argv + 2), deslen);
			if (!len)
			{
				LocalFree(argv);
				return NULL;
			}
			if (len < deslen) break;
			deslen *= 2;
			size = sizeof(LPSTR) * 2 + deslen * sizeof(WCHAR);
			LocalFree(argv);
		}
		argv[0] = (LPSTR)(argv + 2);
		argv[1] = NULL;
		*numargs = 1;

		return argv;
	}

	/* --- First count the arguments */
	argc = 1;
	s = cmdline;
	/* The first argument, the executable path, follows special rules */
	if (*s == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s++;
		while (*s)
			if (*s++ == '"')
				break;
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*s && *s != ' ' && *s != '\t')
			s++;
	}
	/* skip to the first argument, if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s)
		argc++;

	/* Analyze the remaining arguments */
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* skip to the next argument and count it if any */
			while (*s == ' ' || *s == '\t')
				s++;
			if (*s)
				argc++;
			bcount = 0;
		}
		else if (*s == '\\')
		{
			/* '\', count them */
			bcount++;
			s++;
		}
		else if (*s == '"')
		{
			/* '"' */
			if ((bcount & 1) == 0)
				qcount++; /* unescaped '"' */
			s++;
			bcount = 0;
			/* consecutive quotes, see comment in copying code below */
			while (*s == '"')
			{
				qcount++;
				s++;
			}
			qcount = qcount % 3;
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			bcount = 0;
			s++;
		}
	}

	/* Allocate in a single lump, the string array, and the strings that go
	 * with it. This way the caller can make a single LocalFree() call to free
	 * both, as per MSDN.
	 */
	argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (lstrlenA(cmdline) + 1) * sizeof(WCHAR));
	if (!argv)
		return NULL;

	/* --- Then split and copy the arguments */
	argv[0] = d = lstrcpyA((LPSTR)(argv + argc + 1), cmdline);
	argc = 1;
	/* The first argument, the executable path, follows special rules */
	if (*d == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s = d + 1;
		while (*s)
		{
			if (*s == '"')
			{
				s++;
				break;
			}
			*d++ = *s++;
		}
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*d && *d != ' ' && *d != '\t')
			d++;
		s = d;
		if (*s)
			s++;
	}
	/* close the executable path */
	*d++ = 0;
	/* skip to the first argument and initialize it if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (!*s)
	{
		/* There are no parameters so we are all done */
		argv[argc] = NULL;
		*numargs = argc;
		return argv;
	}

	/* Split and copy the remaining arguments */
	argv[argc++] = d;
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* close the argument */
			*d++ = 0;
			bcount = 0;

			/* skip to the next one and initialize it if any */
			do {
				s++;
			} while (*s == ' ' || *s == '\t');
			if (*s)
				argv[argc++] = d;
		}
		else if (*s == '\\')
		{
			*d++ = *s++;
			bcount++;
		}
		else if (*s == '"')
		{
			if ((bcount & 1) == 0)
			{
				/* Preceded by an even number of '\', this is half that
				 * number of '\', plus a quote which we erase.
				 */
				d -= bcount / 2;
				qcount++;
			}
			else
			{
				/* Preceded by an odd number of '\', this is half that
				 * number of '\' followed by a '"'
				 */
				d = d - bcount / 2 - 1;
				*d++ = '"';
			}
			*d++ = *s++;	// This is what makes it keep the quotes. Originally: s++
			bcount = 0;
			/* Now count the number of consecutive quotes. Note that qcount
			 * already takes into account the opening quote if any, as well as
			 * the quote that lead us here.
			 */
			while (*s == '"')
			{
				if (++qcount == 3)
				{
					*d++ = '"';
					qcount = 0;
				}
				s++;
			}
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			*d++ = *s++;
			bcount = 0;
		}
	}
	*d = '\0';
	argv[argc] = NULL;
	*numargs = argc;

	return argv;
}

LPTSTR* CommandLineToArgvKeepQuotes(const LPTSTR cmdline, int* numargs)
{
#ifdef UNICODE
	return CommandLineToArgvKeepQuotesW(cmdline, numargs);
#else
	return CommandLineToArgvKeepQuotesA(cmdline, numargs);
#endif // UNICODE
}

///////////////////////////////////////////////////////////////////////////////
/**
 * Extremely basic file chunk system.
 * Defined a set of static functions so they are only available locally.
 */
 ///////////////////////////////////////////////////////////////////////////////

 /**
 * Creates a new file chunk.
 * If previous_filechunk is not null, it will link the previous chunk with the new chunk
 */
static struct s_filechunk* NewFileChunk(struct s_filechunk* previous_filechunk)
{
	struct s_filechunk* fc = (struct s_filechunk*)malloc(sizeof(struct s_filechunk));
	if (!fc) return NULL;
	fc->data = (char*)malloc(FILECHUNK_SIZE);
	if (!fc->data) return NULL;
	ZeroMemory(fc->data, FILECHUNK_SIZE);
	fc->next = NULL;
	fc->remaining_size = FILECHUNK_SIZE;

	if (previous_filechunk)
	{
		assert(previous_filechunk->remaining_size == 0);
		previous_filechunk->next = fc;
	}

	return fc;
}

// Returns bytes written
static size_t AddToFilechunk(struct s_filechunk* filechunk, char* data, size_t data_size)
{
	if (!filechunk)
		return 0;

	size_t copy_size = data_size;
	if (copy_size > filechunk->remaining_size)
		copy_size = filechunk->remaining_size;

	errno_t err = 0;
	err = memcpy_s(filechunk->data + (FILECHUNK_SIZE - filechunk->remaining_size), filechunk->remaining_size, data, copy_size);
	assert(err == 0);

	filechunk->remaining_size -= copy_size;
	return copy_size;
}

// Check if there's still available space in the current chunk, otherwise creates a new filechunk
static void HandleFileChunkAvailableSpace(struct s_filechunk** filechunk)
{
	assert((*filechunk)->remaining_size >= 0);
	if ((*filechunk)->remaining_size == 0)
		(*filechunk) = NewFileChunk(*filechunk);
}

// Free used memory
static void FreeFileChunkChain(struct s_filechunk** filechunk)
{
	if (!filechunk)
		return;

	struct s_filechunk* next;
	while (*filechunk)
	{
		next = (*filechunk)->next;
		free((*filechunk)->data);
		free((*filechunk));
		(*filechunk) = next;
	}
}
