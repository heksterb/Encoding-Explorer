/*
	Encoding.h
	
	Definitions common to C and C++ methods for
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
	
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/*	Methpd
	Text-generation API methods
*/
enum Method {
	kMethodNone,
	kMethodWindowsAPI,
	kMethodPOSIX,
	kMethodCUnformatted,
	kMethodCFormatted,
	kMethodCPPUnformatted,
	kMethodCPPFormatted
	};


/*	Mode
	Text API modes
*/
enum Mode {
	kModeNone,
	kModeBinary,
	kModeText,
	kModeWide,
	kModeUnicode,
	kModeWideUnicode
	};	


/*	gSample
	Bytes sent as text data
	These are the same characters listed in gSampleWide, but encoded as Code Page 434
	
	Putting the 'static' definitions in the header file so that both the C and C++ code
	can see them; they will be duplicated in the object code.
*/
static const char gSample[] = { 0x41, 0xCE, 0xA3, 0xEB, 0x8C, 0x0A };


/*	gSampleWide
	Wide characters sent as text data (with UTF-16 interpretation)
	
	Not using u8'' here because that has Unicode semantics; we really want to force exactly
	the specific value here
*/
static const wchar_t gSampleWide[] = {
	0x0041,			// LATIN CAPITAL LETTER A
	0x256C,			// BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
	0x00FA,			// LATIN SMALL LETTER U WITH ACUTE
	0x03B4,			// GREEK SMALL LETTER DELTA
	0x00EE,			// LATIN SMALL LETTER I WITH CIRCUMFLEX
	0x000A			// LINE FEED (LF)
	};


/*	SetPOSIXModeForStandardOutput
	Retroactively apply a POSIX mode to the already-open standard output C stream
*/
extern bool SetPOSIXModeForStandardOutput(enum Mode);


/*	OpenFileWithCMode
	Open a C file stream applying the appropriate C file mode
*/
extern FILE *OpenFileWithCMode(enum Mode);


/*	gFileName
	Name of file used when not writing to standard output
*/
extern const char gFileName[];


extern bool TestWindowsAPI(bool standardOutput, enum Mode);
extern bool TestPOSIX(bool standardOutput, enum Mode, bool isWideMode);
extern bool TestC(bool standardOutput, enum Mode, bool isWideMode, bool isMethodFormatted);
extern bool TestCPlusPlusStream(bool standardOutput, enum Mode, bool isWideMode, const char *locale, bool isMethodFormatted);


#ifdef __cplusplus
	}
#endif
