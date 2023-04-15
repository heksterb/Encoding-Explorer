/*
	EncodingC
	
	Definitions particular to C methods for
	Encoding Explorer
	
	Copyright © 2023 by: Ben Hekster
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys\stat.h>

#define WIN32_LEAN_AND_MEAN

#include <WINDOWS.H>
#include <CONSOLEAPI.H>

#include "Encoding.h"



/*	TestWindowsAPI
	Windows API test cases
	Return whether the call failed
*/
bool TestWindowsAPI(
	bool		standardOutput,
	enum Mode	mode
	)
{
HANDLE handle;

// check for unsupported modes
if (mode == kModeUnicode || mode == kModeWideUnicode) {
	fprintf(stderr, "error: no specific 'unicode' modes in Windows API\n");
	return true;
	}

// get output handle
if (standardOutput)
	handle = GetStdHandle(STD_OUTPUT_HANDLE);

else
	handle = CreateFileA("output", GENERIC_WRITE, 0 /* no sharing */, NULL /* security */, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL /* template */);

if (handle == INVALID_HANDLE_VALUE) {
	fprintf(stderr, "error: couldn't get handle for API output\n");
	return true;
	}

// check if mode supported on this handle
/* Console API works only on console handles, not on file handles: it doesn't matter whether we opened
   the file ourselves, or whether the standard handle was a file handle by virtue of redirection. */
DWORD consoleMode;
const bool isConsole = GetConsoleMode(handle, &consoleMode) != 0;
if (!isConsole && (mode == kModeText || mode == kModeWide)) {
	fprintf(stderr,"error: winapi Console APIs ('text' and 'wide') not supported against file (neither explicit or by redirection)\n");
	return true;
	}

bool failed = false;

// perform output
DWORD write;
DWORD written;
BOOL succeeded;
switch (mode) {
	case kModeBinary:
		write = sizeof gSample / sizeof *gSample;
		succeeded = WriteFile(handle, gSample, write, &written, NULL /* overlapped */);
		break;
	
	case kModeText:
		write = sizeof gSample / sizeof *gSample;
		succeeded = WriteConsoleA(handle, gSample, write, &written, NULL);
		break;
	
	case kModeWide:
		write = sizeof gSampleWide / sizeof *gSampleWide;
		succeeded = WriteConsoleW(handle, gSampleWide, write, &written, NULL);
		break;
	}
if (!succeeded) { fprintf(stderr, "error: API write failed\n"); failed = true; }
else if (written != write) { fprintf(stderr, "error: unable to write entire output\n"); failed = true; }

// close
if (!standardOutput) CloseHandle(handle);

return failed;
}


/*	gPOSIXOpenModes
	_open() ‘mode’ parameter corresponding to Mode
*/
static const int gPOSIXOpenModes[] = {
	/* kNone */		0,
	/* kBinary */		_O_BINARY,
	/* kText */		_O_TEXT,
	/* kWide */		_O_WTEXT,
	/* kUnicode */		_O_U8TEXT,
	/* kWideUnicode */	_O_U16TEXT
	};


/*	gCOpenModes
	fopen() ‘mode’ parameter corresponding to Mode
*/
static const char *const gCOpenModes[] = {
	/* kNone */		"0",
	/* kBinary */		"wb",
	/* kText */		"w",
	/* kWide */		"w,ccs=unicode",
	/* kUnicode */		"w,ccs=utf-8",
	/* kUnicodeWide */	"w,ccs=utf-16le"
	};


/*	TestPOSIX
	POSIX-style API test cases
	Return whether call failed
*/
bool TestPOSIX(
	bool		standardOutput,
	enum Mode	mode,
	bool		isWideMode
	)
{
// get output file descriptor
int fd;
if (standardOutput) {
	// use standard output
	fd = _fileno(stdout);
	
	// retroactively apply the 'open()' mode
	if (_setmode(fd, gPOSIXOpenModes[mode]) == -1) {
		fprintf(stderr, "error: can't apply mode to standard output\n");
		return true;
		}
	}

else {
	if ((fd = _open(gFileName, _O_WRONLY | _O_CREAT | _O_TRUNC | gPOSIXOpenModes[mode], _S_IREAD | _S_IWRITE)) == -1) {
		fprintf(stderr, "error: can't open file for output\n");
		return true;
		}
	}

bool failed = false;

// perform output
unsigned int write;
int written;
if (!isWideMode)
	written = _write(fd, gSample, (write = sizeof gSample));

else
	written = _write(fd, gSampleWide, (write = sizeof gSampleWide));

if (written != write) { fprintf(stderr, "error: unable to write entire output\n"); failed = true; }

// close
if (!standardOutput) _close(fd);

return failed;
}


/*	TestCUnformatted
	Standard C unformatted I/O test cases
	Return if call failed
*/
static bool TestCUnformatted(
	FILE		*file,
	bool		isWideMode
	)
{
size_t write, written;
if (!isWideMode)
	written = fwrite(gSample, sizeof *gSample, (write = sizeof gSample / sizeof *gSample), file);

else
	written = fwrite(gSampleWide, sizeof *gSampleWide, (write = sizeof gSampleWide / sizeof *gSampleWide), file);

if (written != write) { fprintf(stderr, "error: unable to write entire output %d %dt\n", write, written); return true; }

return false;
}


/*	TestCFormatted
	Standard C formatted I/O test cases
*/
static bool TestCFormatted(
	FILE		*file,
	bool		isWideMode
	)
{
int write;
size_t written;
if (!isWideMode)
	/* This doesn't accept files opened in POSIX style _O_U8TEXT; use fwprintf() */
	written = fprintf(file, "%*s", (write = (int) (sizeof gSample / sizeof *gSample)), gSample);

else
	written = fwprintf(file, L"%*s", (write = (int) (sizeof gSampleWide / sizeof *gSampleWide)), gSampleWide);

if (written != write) { fprintf(stderr, "error: unable to write entire output\n"); return true; }

return false;
}


/*	SetPOSIXModeForStandardOutput
	Retroactively apply a POSIX mode to the already-open standard output C stream
*/
bool SetPOSIXModeForStandardOutput(
	enum Mode	mode
	)
{
// retroactively apply the 'open()' mode
if (_setmode(_fileno(stdout), gPOSIXOpenModes[mode]) == -1) {
	fprintf(stderr, "error: can't apply mode to standard output\n");
	return true;
	}

return false;
}


/*	OpenFileWithCMode
	Open a C file stream applying the appropriate C file mode
*/
FILE *OpenFileWithCMode(
	enum Mode	mode
	)
{
return fopen(gFileName, gCOpenModes[mode]);
}


/*	TestC
	Standard C I/O test cases
*/
bool TestC(
	bool		standardOutput,
	enum Mode	mode,
	bool		isWideMode,
	bool		isMethodFormatted
	)
{
FILE *file;
if (standardOutput) {
	file = stdout;
	
	SetPOSIXModeForStandardOutput(mode);
	}

else {
	// open
	file = OpenFileWithCMode(mode);
	if (!file) return true;
	
	/* Thought about applying fwide() here, even though it isn't really necessary; however, it's unimplemented. */
	}

bool failed = false;

if (!isMethodFormatted)
	failed = TestCUnformatted(file, isWideMode);

else
	failed = TestCFormatted(file, isWideMode);

// close
if (!standardOutput) fclose(file);

return failed;
}
