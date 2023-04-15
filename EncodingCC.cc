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

#define _CRT_SECURE_NO_WARNINGS

/* Note that the codecvt approach demonstrated here is deprecated as of C++17. */
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <codecvt>
#include <iostream>
#include <fstream>
#include <locale>
#include <memory>

#include "Encoding.h"



/*	TestCPlusPlusNarrowStream
	Perform I/O on narrow-character stream
*/
static void TestCPlusPlusNarrowStream(
	std::ostream	&stream,
	enum Mode	mode,
	bool		isMethodFormatted,
	const char	*locale
	)
{
// imbue stream with locale for the purpose of character set conversion
if (locale)
	stream.imbue(std::locale(locale));

// perform output
if (!isMethodFormatted)
	stream.write(gSample, sizeof gSample);

else
	stream << std::string_view(gSample, sizeof gSample / sizeof *gSample);

// find whether the stream is in a 'failed' state
if (stream.fail()) throw "output stream has failed";
}


/*	TestCPlusPlusWideStream
	Perform I/O on wide-character stream
	
	
	It gets complicated here.  The only truly standard C++ mode is 'wide'; the other two require
	Windows extensions but are more useful:
	
	* wide mode
		stdout ...
		open as wofstream
		characters are converted from UTF-16 to character set implied by locale
		so no BOM, even of locale is ".65001"
	
	* unicode mode
		stdout ...
		open as FILE in Windows Unicode mode, which converts UTF-16 to UTF-8
		not-yet-understood effect of setting locale, but doesn't appear sensible
		UTF-8 BOM
	
	* wide unicode mode
		stdout ...
		open as FILE in Windows UTF-16LE mode
		applies nonconverting UTF16-to-UTF16 conversion
		UTF-16LE BOM
*/
static void TestCPlusPlusWideStream(
	std::wostream	&stream,
	enum Mode	mode,
	bool		isMethodFormatted,
	const char	*locale
	)
{
// imbue stream with locale for the purpose of character set conversion
switch (mode) {
	/* This uses standard C++ open which is absolutely, definitively, a narrow character
	   stream; and can therefire only work with wide characters with a locale
	   that is good enough to represent all the characters. */
	case kModeWide:
	
	/* This uses the Windows extension to open an underlying FILE in Windows 'Unicode mode'
	   which interprets the input as UTF-16 and converts it to UTF-8.  It is also influenced
	   by the locale setting (but not in a useful way, it seems) */
	case kModeUnicode:
		if (locale)
			/* As far as I know, there are no wide-character locales (there is the one corresponding to
			   Code Pages 1200/1201, which isn't available in native code) so setting a locale this way
			   implies some sort of wide-to-narrow conversion; so we'll use 'Unicode mode' to do that. */
			stream.imbue(std::locale(locale));
		
		else
			/* keep the default 'C' locale */
			;
		break;
	
	/* */
	case kModeWideUnicode:
		if (locale) throw "construction of 'wide unicode' mode requires no explicit locale";
		
		// Can also specify the character set conversion this way
		stream.imbue(
			std::locale(stream.getloc(),
			new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>)
			);
		break;
	}

// perform output
if (!isMethodFormatted)
	stream.write(gSampleWide, sizeof gSampleWide / sizeof *gSampleWide);

else
	stream << std::wstring_view(gSampleWide, 6 /* sizeof gSampleWide / sizeof *gSampleWide */);

// if character set convertion fails, the 'failbit' is set; unsure how to distinguish that particular error though
if (stream.fail())
	fprintf(stderr, "info: stream is in 'failed' state; likely due to error in implicit character set conversion\n");
}


/*	gNarrowIOSOpenModes
	std::ofstream() mode parameter corresponding to Mode
	Note that there are no modes to support wide-character output
*/
static const std::ios::openmode gNarrowIOSOpenModes[] = {
	/* kNone */		0,
	/* kBinary */		std::ios::binary,
	/* kText */		0
	/* kWide */
	/* kUnicode */
	/* kWideUnicode */
	};



/*	TestCPlusPlusStandardOutput
	C++ stream I/O standard output-based test cases
*/
static void TestCPlusPlusStandardOutput(
	enum Mode	mode,
	bool		isWideMode,
	const char	*locale,
	bool		isMethodFormatted
	)
{
if (mode == kModeText)
	// assume that standard output is open for narrow-character text already
	;

else
	// retroactively apply a POSIX mode to open standard output
	if (SetPOSIXModeForStandardOutput(mode)) throw "can't apply mode to standard output";
	
// narrow mode?
if (!isWideMode)
	TestCPlusPlusNarrowStream(std::cout, mode, isMethodFormatted, locale);

// wide mode?
else
	TestCPlusPlusWideStream(std::wcout, mode, isMethodFormatted, locale);
}


/*	TestCPlusPlusFile
	C++ stream I/O file-based test cases
*/
static void TestCPlusPlusFile(
	enum Mode	mode,
	bool		isWideMode,
	const char	*locale,
	bool		isMethodFormatted
	)
{
// narrow mode?	
if (!isWideMode) {
	// open using standard method
	std::ofstream stream(gFileName, gNarrowIOSOpenModes[mode]);
	if (!stream.is_open()) throw "can't open file for output";
	
	TestCPlusPlusNarrowStream(stream, mode, isMethodFormatted, locale);
	}

// wide mode?
/* We've defined this to mean the 'canonical' C++ wide stream analogous to the narrow stream;
   for purposes of demonstration */
else if (mode == kModeWide) {
	std::wofstream stream(gFileName);
	if (!stream.is_open()) throw "can't open file for output";
	
	TestCPlusPlusWideStream(stream, mode, isMethodFormatted, locale);
	}
	
// narrow or wide 'unicode mode'?
else {
	// open as C FILE stream
	/* Note that it makes no difference whether you use _fwopen(): the orientation of the C stream
	   is (or, ought to be) determined by fwide() or at least by fprintf()/fwprintf(). */
	std::unique_ptr<FILE, int (*)(FILE*)> file { OpenFileWithCMode(mode), fclose };
	if (!file) throw "can't open file for output";
	
	/* This is a nonstandard Windows extension that allows actual control over the underlying
	   POSIX I/O stream and the conversions that it applies. */
	std::wofstream stream(file.get());
	if (!stream.is_open()) throw "can't open file for output";
	
	TestCPlusPlusWideStream(stream, mode, isMethodFormatted, locale);
	}
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
bool failed = false;

try {
	// standard output?
	if (standardOutput)
		TestCPlusPlusStandardOutput(mode, isWideMode, locale, isMethodFormatted);

	// open file?
	else
		TestCPlusPlusFile(mode, isWideMode, locale, isMethodFormatted);
	}

catch (const char error[]) {
	fprintf(stderr, "error: %s\n", error);
	
	failed = true;
	}

catch (...) {
	fprintf(stderr, "error\n");
	
	failed = true;
	}

return failed;
}
