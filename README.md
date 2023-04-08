# Results

These describe observations and the best of my understanding of what is happening
in regard to text processing in the Windows console.
This is based on a much earlier, but incomplete, analysis I did previously.
The work here was done with the help of a utility program that let me quickly try
different combinations of modes and environmental settings.
All the sample runs were done under the current version of Windows 11.

The discussion looks only at _output_ from a native application to the console
and intentionally ignores the parallel discussion about input.
There are a couple of reasons for this
(not least avoiding the doubling of work that would be needed),
but focusing on one side of the equation at least greatly simplifies the discussion.
I'm operating with the assumption that any observations and statements that can be made
about the output side of I/O should apply (mutatis mutandis) to the other as well.


## Sample Data

I used the following input data for my investigations.
The interpretation of the narrow character under Code Page 437 is identical to the interpretation
of the corresponding wide character under UTF-16:

<table>
	<thead>
		<tr>
			<th>ordinal</th>
			<th>display</th>
			<th>narrow</th>
			<th>wide</th>
			<th>Unicode name</th>
		<tr>
	</thead>
	<tbody>
		<tr>
			<td>0</td>
			<td>A</td>
			<td>41</td>
			<td>0041</td>
			<td>LATIN CAPITAL LETTER A</td>
		</tr>
		<tr>
			<td>1</td>
			<td>╬</td>
			<td>CE</td>
			<td>256C</td>
			<td>BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL</td>
		</tr>
		<tr>
			<td>2</td>
			<td>ú</td>
			<td>A3</td>
			<td>00FA</td>
			<td>LATIN SMALL LETTER U WITH ACUTE</td>
		</tr>
		<tr>
			<td>3</td>
			<td>δ</td>
			<td>EB</td>
			<td>03B4</td>
			<td>GREEK SMALL LETTER DELTA</td>
		</tr>
		<tr>
			<td>4</td>
			<td>î</td>
			<td>8C</td>
			<td>00EE</td>
			<td>LATIN SMALL LETTER I WITH CIRCUMFLEX</td>
		</tr>
		<tr>
			<td>5</td>
			<td></td>
			<td>0A</td>
			<td>000A</td>
			<td>LINE FEED (LF)</td>
		</tr>
	</tbody>
</table>

Note that these characters are intentionally not uniformly representable in
ASCII, ISO-8859-1 or -15, or Code Page 1252.


## Methods

These represent the different APIs and variations that can be used to output text.

* Windows API
* ‘POSIX-style’
* Standard C I/O streams
	* unformatted
	* narrow-character formatted
	* wide-character formatted
* Standard C++ I/O streams
	* unformatted
	* narrow-character formatted
	* wide-character formatted

I further distinguish between

* terminal output (observed in the console)
* file-redirected output (console command redirecting output to a file)
* file output (through an explicitly opened file)


### Windows API

There is no fundamental difference between `WriteConsoleA()` and `WriteFile()`;
it is specifically _not_ the case, say, that `WriteConsole()` does any kind of special text-mode
processing such as CR/LF conversion.
The differences are mostly utilitarian in that the Console API as a whole is specifically
aware of the fact that the output device is a console and provides corresponding control
over it (such as color);
and that there are the two `WriteConsoleA()` and `WriteConsoleW()` versions which may
possibly be of use when writing code that needs to be able to deal with both these worlds.
`WriteConsole()` will _not_ work if standard output has been redirected to a file,
in which case the handle returned by `GetStdHandle()` is not a console but a file handle.
`WriteConsoleA()` and `WriteFile()` appear to both produce byte-for-byte what was presented
on input;
and the console itself displays these as characters as per the Console Output Code Page.

`WriteConsoleW()` however works like ‘Unicode mode’: it assumes as usual that the wide-character
input is UTF-16, and outputs UTF-8 that is interpreted by the console as UTF-8 regardless of
the Console Output Code Page setting.


### ‘POSIX style’

In this case we use `_open()` and `_write()`.
The different modes are selected by the `oflag` argument to
[`_open()`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen)
or the `flag` argument to
[`_setmode`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/setmode)
for an already-open file.
Note that these functions are provided by Microsoft in the C runtime library,
but are not actually part of Standard C.

* binary mode (`_O_BINARY`): 
input is passed as-is
* text mode (`_O_TEXT`):
CR/LF translation is applied
* wide-character text mode (`_O_WTEXT` and `_O_U16TEXT`)
* Unicode mode (`_O_U8TEXT`): as described above

Specifying nonsensical Unicode + binary mode (as `_O_BINARY | _O_U8TEXT`) does not trigger an error
and has the same effect as specifying just binary mode.

Writing directly to a file a BOM is generated for wide, unicode, and wide unicode modes
(but obviously not for text or binary).  When redirecting to a file, Command Prompt
also generates a BOM for those three modes but PowerShell does not.

**NOTE** The Windows header file `fcntl.h` claims that `_O_WTEXT` should produce a BOM and
`_O_U8TEXT` and `_O_U16TEXT` should not; but in my experimentation all three those modes
behave identically in that regard).
In fact, overall I could not find any difference whatsoever in behavior between
`_O_WTEXT` and `_O_U16TEXT`.


### Standard C

Standard C I/O is done with `FILE` stream handles represented by `stdout` or
created by `fopen()`.  C file streams assume a 'narrow' or 'wide' **orientation**
determined implicitly, by whether `fprintf()` ir `fwprintf()` is first called
on them; or theoretically, by calling `fwide()` explicitly.  Note though that
`fwide()` is documented as being unimplemented.  `fwrite()` is used for unformatted output.  

The different modes are selected by the `mode` argument to
[`fopen()`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen)
and work exactly the same as the corresponding POSIX modes.  

* binary: e.g. `"wb"`
* text: e.g. `"w"`
* wide text: e.g. `"w,ccs=unicode"`
* unicode: e.g. `w,ccs=utf-8`
* wide unicode: `w,ccs=utf-16le`

Again, specifying nonsensical Unicode + binary mode (as `"wb,ccs=utf-8"`) does not trigger an error
and has the same effect as specifying just binary mode.
All four text modes perform CR/LF conversion, and all three wide-character modes
insert a prefix BOM.

In fact, as documented, it's possible to change the mode of an already-open stream
by extracting the POSIX file descriptor and applying `_setmode()`:

```
_setmode(fileno(stdout), _O_U8TEXT)
```

All this clearly suggests that the Standard C APIs are implemented on top of the
POSIX style functions.

**NOTE** The BOM that is normally generated when writing to a file in one of the
wide-character modes is not generated when the standard output streams are set to one
of those modes with `_setmode()`.

**NOTE** Unicode mode is not supported through narrow-character `fprintf` even with
the `"%S"` wide-character
[format specifier](https://learn.microsoft.com/en-us/cpp/c-runtime-library/format-specification-syntax-printf-and-wprintf-functions?view=msvc-170#type-field-characters)
(`fwprintf` has to be used).


### Standard C++

Standard C++ uses I/O Streams based on `std::basic_ostream`.
Formatted I/O is done through the usual `<<` operators;
unformatted I/O is done through `.write()`.

An important point to make about C++ I/O is that even though wide-character streams
exist which accept wide-character text (e.g., `std::wostream`),
the 'associated character sequence' of all streams is still narrow
(unless certain explicit steps are taken).
This is counter to the behavior of the C wide-character streams which dynamically
assume a narrow or wide orientation (as described above).
This implies that wide-character data sent through a wide-character stream
is converted from the wide character set (always UTF-16) to some narrow character set:
and this is specified by means of a C++ locale.

The C++ locale can be set either globally or set on an individual stream through `.imbue()`.
The global C++ locale is set through `std::global::locale()`; this sets the C locale as well.
The converse is not generally true, i.e., `setlocale()` does not affect the C++ locale
_except for_ standard output and presumably standard input and error as well.
This might be due to the C++ and C streams having been synchronized through something
like `std::ios_base::sync_with_stdio()`, but I haven't verified this.

It is important to understand that this is an actual conversion,
not just a reinterpretation of the character stream;
and this happens whether the output is sent to the console or to a file.
If a character cannot be represented in the narrow character set, it is generally approximated.
For example, the wide character sample string is converted as follows with the given locales:

<table>
	<thead>
		<tr>
			<th>locale</th>
			<th>displayed</th>
			<th>converted</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>C (default)</td>
			<td>A</td>
			<td>41</td>
		</tr>
		<tr>
			<td>.437/.OCP</td>
			<td>A╬úδî</td>
			<td>41 ce a3 eb 8c 0d 0a</td>
		</tr>
		<tr>
			<td>.1252/.ACP</td>
			<td>A+údî</td>
			<td>41 2b fa 64 ee 0d 0a</td>
		</tr>
		<tr>
			<td>.65001/.utf8</td>
			<td>A╬úδî</td>
			<td>41 e2 95 ac c3 ba ce b4 c3 ae 0d 0a</td>
		</tr>
	</tbody>
</table>

Only the Code Page 437 and UTF-8 locales are able to represent the UTF-16 sample
string correctly; the default `C` locale gives up after the first nonrepresentable
character.

A `std::locale` object can be constructed with a `std::codecvt` object, giving
more precise and explicit control over the conversion.
In particular, this allows construction of a truly wide-character stream using
a 'non-converting' character set conversion that translates UTF-16 into UTF-16
(see [`codecvt_utf16`](https://en.cppreference.com/w/cpp/locale/codecvt_utf16)).
Note however that this classand the entire `<codecvt>` header are deprecated as of C++17.

See the [Locales](#locale) section below for a reference on valid locale strings.



## Modes

I've found that there are broadly four (nominally five) fundamental **modes**
in which character data can be interpreted by the various API methods.


### Binary Mode

This is the simplest mode in which data is not interpreted as text but as binary data
(either 'narrow' bytes or 'wide' 16-bit words) and has no character set interpretation.
In this mode we expect no conversions to be applied to the data.


### Text Mode (Narrow-Character)

All of the other modes cause data to be interpreted as characters (i.e., text).
The canonical text mode sees data as (narrow) byte-size characters.
Typically in all the text modes LF is translated to CR LF.


### Wide-Character Text Mode

In this case the input is interpreted as wide-character UTF-16 and presented on output
as UTF-16 as well.  The locale and Console Output Code Page are ignored.


### ‘Unicode Mode’

Several output methods in Windows support something that is vaguely and confusingly
referred to as ‘Unicode mode’.
This is like wide-chracter text mode in that ths input is interpreted as UTF-16;
internally however it is converted to UTF-8.
Again, the locale and Console Output Code Page are ignored.


This works with Standard C unformatted and POSIX-style output; but against
formatted Standard C I/O _only_ using `fwprintf()` or a run-time assertion is triggered.
This makes some sense given that the input is interpreted as wide characters (though
it doesn't explain why `fprintf("%S")` still doesn't work), and is exactly
[specified](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/setmode)
in the Microsoft documentation for `_setmode()`, further confirming the equivalence between
Standard C and POSIX-style ‘Unicode mode’.  Finally, applying `_setmode()` to
put standard output in Unicode mode seems to show the same behavior and displays "칁ઌ",
which visually corresponds with

* FEFF (ZERO WIDTH NO-BREAK SPACE; BYTE ORDER MARK)
* CE41 (specific Hangul syllable)
* EBA3 (Private Use Area)
* 0A8C (GUJARATI LETTER VOCALIC L)

Now convinced that `_setmode()` allows us to perform Unicode mode I/O to the console,
and given the observation that the Console Output Code Page appears to be overridden
by Unicode mode, I was wondering whether it was the presence of the Unicode BOM itself
that is recognized by the console: but that is not the case.  Just outputting the same
UTF-8 string of bytes in binary mode does _not_ trigger the console to interpret
UTF-8; and it also does not trigger the effect of ignoring the Output Code Page.

**NOTE** I actually don't understand why the BOM occurs in Unicode Mode when writing
directly to a file, but not when redirected to a file in the console.  The former seems
to suggest the BOM is added in the POSIX layer and removed by the console; but I'm
not able to confirm this removal by explicitly writing a UTF-8 BOM to the console.


### ‘Wide-Character Unicode Mode’

The output method APIs do nominally support something that might be called
'wide-character Unicode mode' (corresponding to `_O_U16TEXT` in the POSIX API);
however, in all of my testing I could not find any difference between it and
'regular' wide-character text mode.


## Summary

This table summarizes the available methods and modes.

<table>
	<thead>
		<tr>
			<th>Method</th>
			<th>Binary</th>
			<th>Text</th>
			<th>Wide</th>
			<th>Unicode</th>
			<th>Wide Unicode</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>Windows API</td>
			<td><tt>WriteFile()</tt></td>
			<td><tt>WriteConsoleA()</tt></td>
			<td><tt>WriteConsoleW()</tt></td>
			<td>n/a</td>
			<td>n/a</td>
		<tr>
		<tr>
			<td>‘POSIX’ style</td>
			<td><tt>_open(_O_BINARY)</tt></td>
			<td><tt>_open(_O_TEXT)</tt></td>
			<td><tt>_open(_O_WTEXT)</tt></td>
			<td><tt>_open(_O_U8TEXT)</tt></td>
			<td><tt>_open(_O_U16TEXT)</tt></td>
		<tr>
		<tr>
			<td>C unformatted</td>
			<td><tt>fopen("wb")</tt><br><tt>fwrite()</td>
			<td><tt>fopen("w")</tt><br><tt>fwrite()</td>
			<td><tt>fopen("w,ccs=unicode")</tt><br><tt>fwrite()</td>
			<td><tt>fopen("w,ccs=utf-8")</tt><br><tt>fwrite()</td>
			<td><tt>fopen("w,ccs=utf-16le")</tt><br><tt>fwrite()</td>
		<tr>
		<tr>
			<td>C formatted</td>
			<td><tt>fopen("wb")</tt><br><tt>fprintf()</td>
			<td><tt>fopen("w")</tt><br><tt>fprintf()</td>
			<td><tt>fopen("w,ccs=unicode")</tt><br><tt>fwprintf()</td>
			<td><tt>fopen("w,ccs=utf-8")</tt><br><tt>fwprintf()</td>
			<td><tt>fopen("w,ccs=utf-16le")</tt><br><tt>fwprintf()</td>
		<tr>
		<tr>
			<td>C++ unformatted</td>
			<td><tt>ostream(ios::binary)</tt><br><tt>.write()</tt></td>
			<td><tt>ostream()</tt><br><tt>.write()</tt></td>
			<td>n/a</td>
			<td><tt>wostream()</tt><br><tt>.write()</tt></td>
			<td>n/a</td>
		<tr>
		<tr>
			<td>C++ formatted</td>
			<td><tt>ostream(ios::binary)</tt><br><tt>&lt;&lt;</tt></td>
			<td><tt>ostream()</tt><br><tt>&lt;&lt;</tt></td>
			<td>n/a</td>
			<td><tt>wostream()</tt><br><tt>&lt;&lt;</tt></td>
			<td>n/a</td>
		<tr>
	</tbody>
</table>


## Console

How the console displays characters is influenced by three properties.


### Console Font

This used to be an issue: without a ‘good’ font being configured in the Command Prompt,
you would not get the correct characters displayed, no matter what.  In Windows 11
(at least) this does not appear to be an issue anymore, as the default font
seems to have adequate character set coverage.  On systems before Windows 11, check
to make sure something like Lucida Console is configured.


### Console Code Page

There are two different code pages that appear like they should be associated with
console output:

* The **console code page** (set through `SetConsoleCP()`) doesn't appear to have any effect on output;
in fact I can't find any effect it has at all
* The **console output code page** (set through `SetConsoleOutputCP()`) determines
the character repertoire that is _available for display_ by the console;
it doesn't affect how character data is _stored_ (say, in case output is redirected
to a file).

There is a difference in how these two are handled by Command Prompt and PowerShell
and `chcp`, explained here and summarized in the table:

* Command Prompt: `chcp` affects both CCP and COCP;
both CCP and COCP are persistent across invocations of the program within the console session
* PowerShell: `chcp` sets only the CCP;
COCP appears to be a property of the calling process and changes made by the process
do not persist across invocations but are always initially set to 437.

<table>
	<thead>
		<tr>
			<th></th>
			<td>Console Code Page</td>
			<td>Console Output Code Page</td>
		</tr>
	</thead>
	<tbody>
		<tr>
			<th>Command Prompt</th>
			<td>chcp/console</td>
			<td>chcp/console</td>
		</tr>
		<tr>
			<th>PowerShell</th>
			<td>chcp/console</td>
			<td>437/process</td>
		</tr>
	</tbody>
</table>

Here are some specific Code Pages I looked at; see
[Code Page Identifiers](https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers)
for a complete list:

* [437](https://en.wikipedia.org/wiki/Code_page_437) (default):
	The 'OEM character set' used in DOS pre-Windows.
	Note that this both includes characters not in ASCII, as well as
	does not include all characters from ASCII
* [1252](https://en.wikipedia.org/wiki/Windows-1252)
	Defined by the original Windows;
	roughly a superset of ISO-8859-1 (and therefore ASCII)
* [28591](https://en.wikipedia.org/wiki/ISO/IEC_8859-1): ISO-8859-1
* [65001](https://en.wikipedia.org/wiki/UTF-8): UTF-8 encoding of Unicode


### Locale

The Standard C 'locale' setting (through `setlocale()`) interplays with
the Console Output Code Page in that it defines the character set of the output that is set to the console.
Setting the Standard C locale in general has no effect with on direct or console-redirected file output
(although see above for C++ I/O) but affects the presentation within the console itself with the POSIX-style method in 'text' mode.
This affects the POSIX-style, C (formatted or unformatted), C++ (formatted or unformatted)
It has no effect in any other mode; presumably because input
in those modes either already has a well-defined character set interpretation
(wide, unicode, and wideunicode) or has no character interpretation at all (binary).

[Valid locale names](https://learn.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings)
even include the ability to specify code pages explicitly, so this all appears
very similar to the effect of the Console Output Code Page;
however, it is a different mechanism as can be seen by the fact that they interact.
Note that on my system, `.OCP` (OEM Code Page) invokes Code Page 437 and
`.ACP` (ANSI Code Page) invokes Code Page 1252.


### Examples

These show the display of the sample bytes under different combinations of locale
and Console Output Code Page:

<table>
	<thead>
		<tr>
			<th>Locale</th>
			<th>Console Output Code Page</th>
			<th>Output</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td rowspan="2"><tt>.437</tt>/<tt>.OCP</tt>/not set</td>
			<td>437/not set</td>
			<td>A╬úδî</td>
		</tr>
		<tr>
			<td>1252</td>
			<td>A+údî</td>
		</tr>
		<tr>
			<td rowspan="2"><tt>.1252</tt>/<tt>.ACP</tt></td>
			<td>1252</td>
			<td>AÎ£ëŒ</td>
		</tr>
		<tr>
			<td>437/not set</td>
			<td>AI£ëO</td>
		</tr>
	</tbody>
</table>

When the locale and Console Output Code Page are aligned, the displayed output is
as expected and correct for that given Code Page.
Otherwise, the characters are interpreted as _existing_ in the locale character set
but _approximated_ with characters from the Console Output Page.
For example:

* the next-to-last character with code point EB corresponds to GREEK SMALL LETTER DELTA
in Code Page 437.  When the locale is set to (`.437`) and the Console Output Code Page
is also set to 437, the character is correctly dislayed as "δ"
When the Console Output Code Page is set to 1252, which doesn't have that character
avialable, it is instead approximated by and displayed as "d".
* the final character with code point 8C corresponds to LATIN CAPITAL LIGATURE OE
in Code Page 1252.  When the locale is set to (`.1252`) and the Console Output Code Page
is also set to 1252, the character is correctly displayed as "Œ".
When the Console Output Code Page is set to 437, which doesn't have that character
available, it is instead approximated by and displayed as "O".

Note that setting the Console Output Code Page to 65001 (for UTF-8) allows
the characters to be correctly displayed according to the specified locale
in every case.  This again serves to reinforce that the COCP does not dictate
the character set _encoding_ of the data, but the available character _repertoire_.


## Console File Redirection

When the output of a command is redirected in the console, note first of all that the
console APIs such as `WriteConsole()` no longer work; `WriteFile()` must be used.  However,
the character set encoding of the file depends on a number of factors.


### Command Prompt

Supposedly depends on whether it was invoked as `CMD /U` or `CMD /A`,
but I haven't been able to find any difference:
the input is passed through to the output without any character set interpretation or any CR/LF translation.


### PowerShell

The ‘redirection operator’ `>` is a shortcut for
[`| OutFile`](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/out-file)
.  The documentation claims that its default encoding is UTF-8-no-BOM, but my observation is
that UTF-16-with-BOM is the default.
Either way, supposedly this can be overridden with the
[`-Encoding`](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/out-file?view=powershell-7.3#-encoding)
argument.
Also, CR/LF translation is applied and CR/LF is **appended** after the end of the last line if none is present.
It appears that the input byte stream is always interpreted as Code Page 437;
neither the Console Code Page nor the Console Output Code Page settings have any effect on this.

To illustrate the effect of PowerShell file redirection: our sample byte stream becomes

```
FF FE 41 00 6c 25 FA 00 B4 03 EE 00 0D 00 0A 00 0D 00 0A 00 00 00 0D 00 0A 00
```

corresponding to the UTF-16 code points

```
FEFF 0041 256C 00FA 03B4 00EE 000D 000A 000D 000A 0000 000D 000A
```

which are exactly the interpretations under Code Page 437.  For example, `CE` translates
to `256C` (BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL).


### Unicode Mode and File Redirection

Because of the multiple places where character set translations are done, you can
construct some really ludicrous situations.  For example, if you open a file in POSIX
Unicode Mode, as described above internally your input is intepreted as UTF-16
`CE41 EBA3 0A8C` and translated into UTF-8 `ec b9 81 ee ae a3 e0 aa 8c`.  Redirecting this
in PowerShell then interprets the UTF-8 bytes as Code Page 437 and translates them
into UTF-16:

* `EC` → `221E`
* `B9` → `2563`
* `81` → `00FC`
* `EE` → `03B5`
* `AE` → `00AB`
* `A3` → `00FA`
* `E0` → `03B1`
* `AA` → `00AC`
* `8C` → `00EE`

Adding a BOM and CR/LF results in the nonsensical monstrosity:

```
FF FE 1E 22 63 25 FC 00 B5 03 AB 00 FA 00 B1 03 AC 00 EE 00 0D 00 0A 00
```

which opens as "∞╣üε«úα¬î".

This is controlled by the PowerShell `$OutputEncoding` variable:

* `[Console]::OutputEncoding = [Text.ASCIIEncoding]::ASCII` (Code Page 20127; 7-bit ASCII)
* `[Console]::OutputEncoding = [Text.UTF8Encoding]::UTF8` (Code Page 65001; UTF-8)
* `[Console]::OutputEncoding = [Text.UnicodeEncoding]::Unicode` (Code Page 65001; UTF-8)

It seems they affect the Console Output Code Page that is _seen by_ the application;
however, _setting_ it through `SetConsoleOutputCP()` has no effect.
So at a minimum, this could be used to check what encoding PowerShell is expecting.

## Resources

* [UTF-8 decoder](https://software.hixie.ch/utilities/cgi/unicode-decoder/utf8-decoder): good for working in hexadecimal
* [UTF8Tools](https://onlineutf8tools.com/) lots of (too many) tools
* [CMD Reference](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/cmd)