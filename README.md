# Welcome to the ed(1) museum!

I'm a big fan of ed(1), and I thought it would be fun to collect various
versions of the Standard Text Editor.  I found it interesting to see how both the
code of the editor and even the C language evolved between releases.  I also
noticed that GNU ed and the other modern, POSIX ed(1)s are all a lot bigger
than the original ed(1).  I thought it would be fun to see if it would be
possible to minimally update the code to allow it to be compiled with modern
ANSI C compilers.

## Files and Directories

First, all source files were copied from The Unix Archive maintained by
The Unix Heritage Society.  See: https//www.tuhs.org/  See the COPYING section
below for a longer, more entertaining discussion of sourcing.

* v6-ed.c: /usr/source/s1/ed.c from Unix Version 6, original C version of ed.
* v7-ed.c: /usr/src/cmd/ed.c from Unix Version 7
* 2.11bsd-ed.c: ed.c from that last release of BSD for the PDP-11
* 4.3bsd-ed.c: ed.c from 4.3BSD, very similar to the 2.11BSD version
* 4.4bsd-ed/: for 4.4BSD, ed(1) got a directory to itself, this source is unmodified
* ed211/: 2.11BSD version ed(1) converted to ANSI and updated to minimally handle UTF-8
* v6ed/: V6 ed(1) converted to ANSI, minimal changes, ASCII only.

There are some places in the code with the high-order bit of a character is used
as a flag when processing substitutions.  For v6ed, this code is left as-is.  I
wanted to keep this one as pure and original as possible.  It is 7-bit ASCII only.

For ed211, I decided to try and make it UTF-8 clean.  As a minimal intervention,
I switched the code to use the illegal bytes in UTF-8 where the original just set
the high-order bit.

v6ed has two other fun quirks.  First, it's written in a really old dialect of C.
All of the `&=`, `+=` and so on were _backwards_ in that version.  I also ported
the memory allocation code from v7-ed.c to v6ed, as v6-ed.c uses the brk(2) and
sbrk(2) calls.  This was a really wonderful moment, as those calls are minimally
documented on modern 32- and 64-bit systems with a warning to avoid them.  I now
understand the reason for this: they were built around the segment registers of
the PDP-11's memory management unit, and were not very portable.  And they make
no sense at all in the context of a modern kernel that completely hides the
page table MMUs of modern processors.

I did end up rearringing the code in ed211 and v6ed quite a bit, this was to keep
from having to add a huge number of function prototypes at the top.

Anyway, comparing v6-ed.c and v6ed/ed.c is quite worthwhile.

I made more extensive modifications to ed211, so I could use it as my daily-driver
ed(1).  I am using it to compose this document.

## COPYING

Strap in kids, because we get to talk about USL v. BSDi and SCO v. Novell.

USL v. BSDi was a lawsuit where UNIX Systems Laboratories attempted to assert
copyright over the code used in BSD/OS.  Essentially, USL wanted license
fees because the BSD codebase was derived from Unix Version 7.  BSDi
won, and the court decided that because AT&T had not put proper license notices
on its distributions of early Unix, that code was in the public domain.

This means that v6-ed.c and v7-ed.c are public domain, if you are in the United States.
I also remand the contents of the v6ed directory to the public domain.

But what about the Caldera license?  Well, when SCO melted down and sued everyone
they could think of, well, they sued Novell, who they had, in theory, bought the
rights to the Unix source code from.  (But didn't USL own that?  Well, after they
lost USL v. BSDi, they sold it to Novell.)  It turned out that the courts didn't
agree that SCO had bought the IP rights that they thought they had, and thus were
not legally able to issue the Caldera license, so it is null and void.

Which means that Novell, who is now owned by Micro Focus, owns the rights to
v6-ed.c and v7-ed.c outside the US.  They probably don't care, but no one should
go bundling this in a distribution of a modern POSIX operating system.

All of the BSD code, is, of course:

```
Copyright 1979, 1980, 1983, 1986, 1988, 1989, 1991, 1992, 1993
	The Regents of the University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
This product includes software developed by the University of
California, Berkeley and its contributors.
4. Neither the name of the University nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
```
