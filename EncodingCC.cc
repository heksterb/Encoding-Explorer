/*
	EncodingCC
	
	Definitions particular to C++ methods for
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

#include <codecvt>
#include <iostream>
#include <fstream>
#include <locale>

#include "Encoding.h"


/*	gSample
	Bytes sent as text data (copy from Encoding.c)
	
	Not using u8'' here because that has Unicode semantics; we really want to force exactly
	the specific value here
*/
static const char gSample[] = {
	static_cast<char>(0x41),
	static_cast<char>(0xce),
	static_cast<char>(0xa3),
	static_cast<char>(0xeb),
	static_cast<char>(0x8c),
	static_cast<char>('\n')
	};


/*	gSampleWide
	Wide characters sent as text data
*/
static const wchar_t gSampleWide[] = { 0x0041, 0x256C, 0x00FA, 0x03B4, 0x00EE, 0x000A };



/*	TestCPlusPlusNarrowStream
	Perform I/O on narrow-character stream
	Return wheter call failed
*/
static bool TestCPlusPlusNarrowStream(
	bool		standardOutput,
	enum Mode	mode,
	bool		isMethodFormatted
	)
{
std::ostream *stream;
std::ofstream file;

// get stream
if (standardOutput) {
	if (mode != kModeText) {
		fprintf(stderr, "error: unable to reopen standard output in non-'text' mode\n");
		return true;
		}
	
	stream = &std::cout;
	}

else {
	std::ios::openmode iosMode;
	
	switch (mode) {
		case kModeBinary:	iosMode = std::ios::binary; break;
		case kModeText:		iosMode = 0; break;
		}
	
	file.open(gFileName, iosMode);
	if (!file.is_open()) {
		fprintf(stderr, "error: can't open file for output\n");
		return true;
		}
	
	stream = &file;
	}

// perform output
if (!isMethodFormatted)
	stream->write(gSample, sizeof gSample);

else
	*stream << std::string_view(gSample, sizeof gSample / sizeof *gSample);

// find whether the stream is in a 'failed' state
bool failed = stream->fail();
if (failed) fprintf(stderr, "error: output stream has failed\n");

return failed;
}


/*	TestCPlusPlusWideStream
	Perform I/O on wide-character stream
	Return wheter call failed
*/
static bool TestCPlusPlusWideStream(
	bool		standardOutput,
	enum Mode	mode,
	bool		isMethodFormatted,
	const char	*locale
	)
{
std::wostream *stream;
std::wofstream file;

// get stream
if (standardOutput) {
	if (mode != kModeWide) {
		fprintf(stderr, "error: unable to reopen standard output in non-'wide' mode\n");
		return true;
		}
	
	stream = &std::wcout;
	}

else {
	if (mode == kModeUnicode || mode == kModeWideUnicode) {
		fprintf(stderr, "error: C++ streams don't support 'unicode' or 'wideunicode' modes\n");
		return true;
		}
	
	file.open(gFileName, 0);
	if (!file.is_open()) {
		fprintf(stderr, "error: can't open file for output\n");
		return true;
		}
	stream = &file;
	}

// imbue stream with locale for the purpose of character set conversion
if (locale)
	#if 1
		stream->imbue(std::locale(locale));
	
	#else
		// Can also specify the character set conversion this way
		stream->imbue(std::locale(out.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
		#endif

// perform output
if (!isMethodFormatted)
	stream->write(gSampleWide, sizeof gSampleWide / sizeof *gSampleWide);

else
	*stream << std::wstring_view(gSampleWide, sizeof gSampleWide / sizeof *gSampleWide);

// if character set convertion fails, the 'failbit' is set; unsure how to distinguish that particular error though
if (stream->fail())
	fprintf(stderr, "info: stream is in 'failed' state; likely due to error in implicit character set conversion\n");

return false;
}


/*	TestCPlusPlusStream
	C++ stream I/O test cases
*/
extern "C"
bool TestCPlusPlusStream(
	bool		standardOutput,
	enum Mode	mode,
	bool		isWideMode,
	const char	*locale,
	bool		isMethodFormatted
	)
{
bool failed;

// locale can also be set globally this way
#if 0
	/* Unfortunately this does raise a run-time assertion if the locale doesn't exist. */
	(void) std::locale::global(std::locale(locale));
	#endif

if (!isWideMode)
	failed = TestCPlusPlusNarrowStream(standardOutput, mode, isMethodFormatted);

else
	failed = TestCPlusPlusWideStream(standardOutput, mode, isMethodFormatted, locale);

return failed;
}
