# Encoding Explorer

Simple utility for testing the effect of different ways of generating text output.


## Usage

```
encexp method mode [options...]
```

_method_ defines the basic API category used and is one of:

* `winapi`: use Windows API methods (`OpenFile()`, `WriteFile()`, `WriteConsole()`)
* `posix`: use ‘POSIX style’ methods (`_open()`, `_write()`)
* `unformatted`: use unformatted C stream I/O (`fopen()` and `fwrite()`)
* `formatted`: use formatted C stream I/O (`fopen()` and `fprintf()`)
* `unformatted++`: use unformatted C++ stream I/O (`std::basic_ostream` and `.write()`)
* `formatted++`: use formatted C++ stream I/O (`std::basic_ostream` and `<<`)

_mode_ selects different options within the API and is one of:

* `binary`: data as non-text binary
* `text`: data as narrow-character (8-bit) text
* `wide`: data as wide-character (16-bit) text
* `unicode`: output as ‘Unicode mode’; this is something variously described
in Microsoft documentation as accepting wide-character (i.e. UTF-16) and
converted internally to UTF-8
* `wideunicode`: even less well documented, vaguely described as accepting UTF-16
and outputting UTF-16

Not every mode is supported by every method (as described in the table, below).

Supported _options_ are:

* `cp####`: set the Console Output Code Page (through a call to `SetConsoleOutputPage()`)
prior to generating output
* `l####`: set the locale prior to generating output;
the specific method used depends on the selected Method and is 
* `file`: output directly to a file named `output` as opposed to standard output


## Summary of Supported Modes and Methods

<TABLE>
	<THEAD>
		<TR>
			<TH>Method
			<TH>Binary
			<TH>Text
			<TH>Wide
			<TH>Unicode
			<TH>Wide Unicode
	</THEAD>
	<TBODY>
		<TR>
			<TD>Windows API
			<TD><TT>WriteFile()</TT>
			<TD><TT>WriteConsoleA()</TT>
			<TD><TT>WriteConsoleW()</TT>
			<TD>n/a
			<TD>n/a
		<TR>
			<TD>‘POSIX’ style
			<TD><TT>_open(_O_BINARY)</TT>
			<TD><TT>_open(_O_TEXT)</TT>
			<TD><TT>_open(_O_WTEXT)</TT>
			<TD><TT>_open(_O_U8TEXT)</TT>
			<TD><TT>_open(_O_U16TEXT)</TT>
		<TR>
			<TD>C unformatted
			<TD><TT>fopen("wb")</TT><BR><TT>fwrite()</TT>
			<TD><TT>fopen("w")</TT><BR><TT>fwrite()</TT>
			<TD><TT>fopen("w,ccs=unicode")</TT><BR><TT>fwrite()</TT>
			<TD><TT>fopen("w,ccs=utf-8")</TT><BR><TT>fwrite()</TT>
			<TD><TT>fopen("w,ccs=utf-16le")</TT><BR><TT>fwrite()</TT>
		<TR>
			<TD>C formatted
			<TD><TT>fopen("wb")</TT><BR><TT>fprintf()</TT>
			<TD><TT>fopen("w")</TT><BR><TT>fprintf()</TT>
			<TD><TT>fopen("w,ccs=unicode")</TT><BR><TT>fwprintf()</TT>
			<TD><TT>fopen("w,ccs=utf-8")</TT><BR><TT>fwprintf()</TT>
			<TD><TT>fopen("w,ccs=utf-16le")</TT><BR><TT>fwprintf()</TT>
		<TR>
			<TD>C++ unformatted
			<TD><TT>ostream(ios::binary)</TT><BR><TT>.write()</TT>
			<TD><TT>ostream()</TT><BR><TT>.write()</TT>
			<TD><TT>wostream()</TT><BR><TT>.write()</TT>
			<TD><TT>fopen("w,ccs=utf-8")</TT><BR><TT>wostream(FILE)</TT><BR><TT>.write()</TT>
			<TD><TT>fopen("w,ccs=utf-16le")</TT><BR><TT>wostream(FILE)</TT><BR><TT>codecvt_utf16</TT><BR><TT>.write()</TT>
		<TR>
			<TD>C++ formatted
			<TD><TT>ostream(ios::binary)</TT><BR><TT>&lt;&lt;</TT>
			<TD><TT>ostream()</TT><BR><TT>&lt;&lt;</TT>
			<TD><TT>wostream()</TT><BR><TT>&lt;&lt;</TT>
			<TD><TT>fopen("w,ccs=utf-8")</TT><BR><TT>wostream(FILE)</TT><BR><TT>&lt;&lt;</TT>
			<TD><TT>fopen("w,ccs=utf-16le")</TT><BR><TT>wostream(FILE)</TT><BR><TT>codecvt_utf16</TT><BR><TT>&lt;&lt;</TT>
	</TBODY>
</TABLE>


## Summary of Locale

* Windows API: n/a
* POSIX style: n/a
* C (unformatted and formatted): `setlocale(LC_ALL, ####)`
* C++ (unformatted and formatted):
	* binary, text (narrow input and output): `std::ostream::imbue(std::locale(####))`
	* wide, unicode (wide input, narrow output): `std::wostream::imbue(std::locale(####))`
	* wide unicode (wide input and output): `std::wostream::imbue(std::locale())` with `std::codecvt_utf16`
