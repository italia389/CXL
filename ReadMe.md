Introduction
============
CXL is the "C eXtensions Library", a lightweight and robust package of routines that provide additional
functionality and features (or "extensions") to the Standard C Library.  The CXL library's API consists of several
functions, which are summarized in the "Features" section below.

Installation
============
Installation is very straightforward.  Change to the root of the source directory and do one of the following:

    On Linux:
        $ sudo ./cxl-1.2.0.sh

    On macOS:
        $ open cxl-1.2.0.pkg

This will install the CXL static library and C headers into `/usr/local`.

Features
========
CXL has several features, some of which are not available elsewhere.  A partial list follows.

Common Exception Handling
-------------------------
The library includes a small number of routines that provide a mechanism for exception handling, similar to and in
concert with the "errno" mechanism used by Linux and Unix systems.  This framework provides a simple way to set a custom
exception message in a function, or one associated with an errno code, and return a non-zero value to the caller.  The
exception routines are used by all the library functions and can be used by programs that use the library as well.

Switch Parsing
--------------
A switch parsing function called **getSwitch()** is available, which can be used to process command-line switches of
form *-x*, *-word*, or *-multi-word* with advanced capabilities and sophisticated error checking.  Switches can have one
or more alias names (for example, *-verbose* with alias *-v*), and may accept an argument.  Switches and arguments may
be required or optional.  Numeric switches of form *-n* and *+n* are also allowed.  Additionally, *--* is recognized as
an explicit end to switch parsing, *-* as a file (stdin) argument, and *--value* and *++value* as switch values that
begin with a dash or plus sign.

Datum Objects
-------------
The library introduces the concept of a *Datum* object, which is an entity that can hold any type of data, including
strings of any length and arrays.  A large number of routines are included which define and manipulate these objects.
Strings can be built or "fabricated" in pieces via an associated *DFab* object, in the same manner as a file; that is,
you open a *DFab* object with an associated *Datum* object, write text to it, and close it.  There is also a garbage
collection mechanism available for releasing heap memory in bulk that is used by the objects.

Dynamic Arrays
--------------
Routines are included which create and manipulate dynamic arrays of any length.  The arrays contain *Datum* objects as
elements.  Arrays can be extended on the fly, cloned, sliced, concatenated, etc., and elements can be inserted or
deleted in a manner similar to high level languages like Ruby and Python.

Hash Tables
-----------
There is a set of routines which create and manipulate hash tables.  They use strings as keys and contain *Datum*
objects as node values.  Hash tables are rebuilt automatically as nodes are added according to default or custom
"tuning" parameters.  A sort routine is also available.

Fast I/O
--------
The library includes a set of fast I/O routines which use large buffers and low level system calls to improve
performance.  One of the input functions is **ffgets()**, which can be used to read a line of any length from a file
with automatic detection of a CR, NL, CR-LF, or NUL line delimiter.  You can also "slurp" a file; that is, read an
entire file at once as a single "line".

Fast Boyer-Moore Searching
--------------------------
There are also routines that implement the Apostolico-Giancarlo fast-search algorithm for plain
text pattern matching on 8-bit character strings (variant of Boyer-Moore).  Searches can be performed forward
or backward, and optionally ignoring case.  The sequence of characters to search can also be supplied by a
user-written callback function.

Runs On macOS and Linux
-----------------------
CXL can be used on macOS and all Linux platforms.  It contains precompiled libraries for both.  It should also be
portable to other Unix platforms as well with little or no modifications.

Free
----
CXL is released under the GNU General Public License (GPLv3).  See the file `License.txt` for details.

Documentation
=============
The project includes detailed installation instructions, and a full set of man pages for every library function,
including pages for each of the major packages (datum, array, fast I/O, and hash table).

Distribution
============
The current distribution of the CXL library may be obtained at `https://github.com/italia389/CXL.git`.

Contact and Feedback
====================
User feedback is welcomed and encouraged.  If you have the time and interest, please contact Rick Marinelli
<italian389@yahoo.com> with your questions, comments, bug reports, likes, dislikes, or whatever you feel is worth
mentioning.  Questions and feature requests are welcomed as well.  You may also report a bug by opening an issue on the
GitHub page.

Notes
=====
This distribution of the CXL library is version 1.2.0.  Installer packages containing 64-bit binaries are included for
Linux platforms and macOS ver. 10.12 and later.  The sources can be compiled instead if desired; however, the build
process has not been tested on other Unix platforms and there may be some (hopefully minor) issues which will need to be
resolved.  If you are compiling the sources and encounter any problems, please contact the author with the details.

Credits
=======
The CXL library (c) Copyright 2022 Richard W. Marinelli is all original code written by
Rick Marinelli <italian389@yahoo.com>.
