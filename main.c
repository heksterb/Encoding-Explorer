/*
	main
	
	Command-line entry point for Encoding Explorer:
	A utility program to investigate text encoding behavior
	in Windows console applications
	
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
	
	
	Usage:
		encexp method mode [cp####] [l####] [file]
	
	where “method” determines the API used to generate output:
	
		winapi		Windows API (WriteFile, WriteConsole)
		posix		‘POSIX-style’ API (_open, _write)
		unformatted	Unformatted C I/O (fopen with fwrite)
		formatted	Formatted C I/O (fopen with fprintf/fwprintf)
		unformatted++	Unformatted C++ I/O (ostream/wostream with .write())
		formatted++	Formatted C++ I/O (ostream/wostream with operator<<())
	
	and “mode” is one of
	
		binary		Binary
		text		Narrow-character text
		wide		Wide-character text
		unicode		Narrow-character ‘Unicode mode’
		wideunicode	Wide-character ‘Unicode mode’
	
	“cp####” causes the Console Output Code Page to be set to #### (e.g. “cp1252”)
	
	“l####” causes the locale to be set to #### (e.g. “lC”)
	
	“file” causes output to a file (named “output”) that is read back and printed
	as hexadecimal bytes; if not specified, data is written to standard output
*/

#define _CRT_SECURE_NO_WARNINGS

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN

#include <WINDOWS.H>
#include <CONSOLEAPI.H>

#include "Encoding.h"


/*	gMethodNames
	Command-line argument names for API Methods
*/
static const char *const gMethodNames[] = {
	"",
	"winapi",
	"posix",
	"unformatted",
	"formatted",
	"unformatted++",
	"formatted++"
	};


/*	gMethodUsesCLocale
	Whether the given API method respects the Standard C locale
*/
static const bool gMethodUsesCLocale[] = {
	false,
	false,
	false,
	true,
	true,
	false,
	false
	};


/*	ParseMethod
	Parse the given Method name string into its corresponding enumerator
*/
static enum Method ParseMethod(
	const char	arg[]
	)
{
enum Method result = kMethodNone;

// find its offset in the list
for (
	const char *const
		*methodNamep = gMethodNames,
		*const methodNamepe = gMethodNames + sizeof gMethodNames / sizeof *gMethodNames;
	methodNamep < methodNamepe;
	methodNamep++
	)
	if (strcmp(arg, *methodNamep) == 0) {
		result = methodNamep - gMethodNames;
		break;
		}

return result;
}


/*	gModeNames
	Command-line argument names for text modes
*/
static const char *const gModeNames[] = {
	"",
	"binary",
	"text",
	"wide",
	"unicode",
	"wideunicode"
	};


/*	gModeIsWide
	Whether the corresponding mode requires a wide-character API call
*/
static const bool gModeIsWide[] = {
	false,
	false,
	false,
	true,
	true,
	true
	};


/*	ParseMode
	Parse the given Mode name string into its corresponding enumerator
*/
static enum Mode ParseMode(
	const char	arg[]
	)
{
enum Mode result = kModeNone;

// find its offset in the list
for (
	const char *const
		*modeNamep = gModeNames,
		*const modeNamepe = gModeNames + sizeof gModeNames / sizeof *gModeNames;
	modeNamep < modeNamepe;
	modeNamep++
	)
	if (strcmp(arg, *modeNamep) == 0) {
		result = modeNamep - gModeNames;
		break;
		}

return result;
}


/*	gFileName
	Name of file used when not writing to standard output
*/
const char gFileName[] = "output";



/*	Test
	Test specified configuration
	Return whether the function failed
*/
static bool Test(
	bool		standardOutput,
	enum Method	method,
	enum Mode	mode,
	UINT		codePage,
	const char	*locale
	)
{
// print console code page status before setting it
DWORD ccp, cocp;
if ((ccp = GetConsoleCP()) != 437)
	fprintf(stderr, "info: console code page was not 437 but %d\n", ccp);
if ((cocp = GetConsoleOutputCP()) != 437)
	/* It appears that in PowerShell, chcp only affects the Console Code Page and not the
	   Console Output Code Page; whereas in Command Prompt it affects both.
	   Still have no idea what effect the Console Code Page has. */
	fprintf(stderr, "info: console output code page was not 437 but %d\n", cocp);

// set console output code page
if (codePage) {
	if (mode == kModeUnicode)
		fprintf(stderr, "info: 'unicode' appears to override the code page setting\n");
	
	SetConsoleOutputCP(codePage);
	}

// set C locale globally (C++ locale is set on the specific stream object)
if (locale && gMethodUsesCLocale[method])
	if (!setlocale(LC_ALL, locale)) {
		fprintf(stderr, "error: unable to apply C locale \"%s\"\n", locale);
		return true;
		}

// is wide-character input mode?
const bool isWideMode = gModeIsWide[mode];

// run test based on requested method
bool result;
switch (method) {
	case kMethodWindowsAPI:		result = TestWindowsAPI(standardOutput, mode); break;
	case kMethodPOSIX:		result = TestPOSIX(standardOutput, mode, isWideMode); break;
	case kMethodCUnformatted:	result = TestC(standardOutput, mode, isWideMode, false); break;
	case kMethodCFormatted:		result = TestC(standardOutput, mode, isWideMode, true); break;
	case kMethodCPPUnformatted:	result = TestCPlusPlusStream(standardOutput, mode, isWideMode, locale, false); break;
	case kMethodCPPFormatted:	result = TestCPlusPlusStream(standardOutput, mode, isWideMode, locale, true); break;
	} if (result) return true;

// print console code page status after setting it
if ((ccp = GetConsoleCP()) != 437)
	fprintf(stderr, "info: console code page is now %d\n", ccp);
if ((cocp = GetConsoleOutputCP()) != 437)
	/* It *appears* that in PowerShell, chcp only affects the Console Code Page and not the
	   Console Output Code Page; whereas in Command Prompt it affects both.
	   Still have no idea what effect the Console Code Page has. */
	fprintf(stderr, "info: console output code page is now %d\n", cocp);

return false;
}


/*	main
	Command-line entry point
*/
int main(
	int		argc,
	const char	*argv[]
	)
{
bool standardOutput = true;
UINT codePage = 0;
const char *locale = NULL;

// first argument is method
const enum Method method = argc > 1 ? ParseMethod((--argc, *++argv)) : kMethodNone;
if (method == kMethodNone) {
	fprintf(stderr, "error: first argument must be one of: winapi, posix, unformatted, formatted, unformatted++, formatted++\n");
	return -1;
	}

// second argument is format
const enum Mode mode = argc > 1 ? ParseMode((--argc, *++argv)) : kModeNone;
if (mode == kModeNone) {
	fprintf(stderr, "error: second argument must be one of: binary, text, wide, unicode, wideunicode\n");
	return -1;
	}

// other configuration arguments
for (const char *arg; arg = *(--argc, ++argv); )
	// code page?
	if (strncmp(arg, "cp", 2) == 0) {
		if ((codePage = strtoul(arg + 2, NULL, 10)) == 0)
			fprintf(stderr, "warning: option cp#### needs code page number\n");
		}
	
	// locale?
	else if (strncmp(arg, "l", 1) == 0)
		locale = arg + 1;
	
	// output into file?
	else if (strcmp(arg, "file") == 0)
		standardOutput = false;
	
	else
		fprintf(stderr, "warning: unexpected option: \"%s\"\n", arg);

// run test
if (Test(standardOutput, method, mode, codePage, locale)) return -1;

// test output to file?
if (!standardOutput) {
	/* Remember to open in 'binary' mode or we get CR/LF conversion.
	   We're _assuming_ nothing else funky is happening in binary mode;
	   to be completely sure we could memory-map the file in. */
	FILE *const file = fopen(gFileName, "rb");
	if (!file) {
		fprintf(stderr, "can't open output file for reading\n");
		return -1;
		}
	
	// print each byte in the file as hexadecimal
	for (int c; (c = fgetc(file)) != EOF;) printf("%02x ", c);
	fputc('\n', stdout);
	
	fclose(file);
	}

return 0;
}
