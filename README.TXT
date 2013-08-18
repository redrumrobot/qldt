===========================
= Quake Live - Demo Tools =
===========================

Quake Live Demo Tools is a small set of tools for managing, editing and viewing Quake Live 
and Quake 3 Arena demos.


- Supported platforms -
-----------------------
1. Binary packages:

* Windows XP/Vista/7
* Ubuntu 10.04 / Debian Squeeze 32 and 64-bit
* GNU/Linux 32 and 64-bit distro with Qt 4.6.0 or later installed

2. Source code:

* Windows XP/Vista/7 32-bit
* GNU/Linux 32 and 64-bit, gcc 4.2

Building the sources requires Qt 4.6.0 or later.


- Supported games -
-------------------
* Quake Live - dm_73
* Quake 3 Arena (VQ3/OSP/CPMA) - dm_68


- Build and install -
---------------------
Linux:

qmake-qt4 -spec linux-g++-32 (or linux-g++-64)
make qmake
make
make install

Windows (VC++):

qmake
nmake qmake
nmake


- Site and Email -
------------------
http://qldt.sf.net
edxbow@gmail.com


QLDT is available under the GPLv3 license, see the COPYING for details.
Copyright (c) 2010-2011 Ed Bow
