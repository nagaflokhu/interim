Hello
-----

Bomberjacket OS is a radical new operating system with a focus on minimalism. It steals conceptually from Lisp machines and Plan 9.

![Bomberjacket OS Logo](http://dump.mntmn.com/bjos.png)

Architecture
------------

Layer 2: Jockey Graphical Terminal / Editor

Layer 1: Sledge Lisp JIT Compiler

Layer 0: Platform Interface. On X86-64, this is Pure64/BareMetal. The "alien hosted" variant uses SDL.

Rationale
---------

- The shell is the editor is the REPL is the language is the compiler.
- Namespacing allows sandboxing and network transparency.
- You need only one text encoding: UTF-8.
- All facilities must be navigable by keyboard.

Licenses
--------

Bomberjacket OS: GNU GPLv3 or later, (C) 2015 Lukas F. Hartmann / @mntmn

Bomberjacket OS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Pure64/BareMetal: 3-clause BSD, (C) ReturnInfinity / Ian Seyler

GNU Lightning: GNU GPL v3

newlib: GNU GPL v2