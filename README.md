 Some ZDoom based legacy ports with lower system requirements for Windows 9x or later and older hardware.
 Some branches are mostly discontinued from now on (JUL 10 2018).

 - g3.3mgw branch:
 A maintenance branch of GZDoom 3.3 compiling with MinGW and running on older non SSE2 cpus while keeping the
DDRAW and D3D backends for compatibility. Old 3.4 legacy build, now LZDoom.

 - legacy branch:
 Official GZDoom 3.5 legacy build for GL 2 hardware.

 - gzdoom32 branch:
 ZDoom32 is a fork of truecolor ZDoom by dpJudas and Rachael and ZDoom 2.9pre (https://github.com/rheit/zdoom).
 It's a merge of dpJudas old truecolor branch (SEP 08 2016) and ZDoom master as of DEC 03 2016.
 Later merged with the GZDoom g1.x branch (APR 24 2016). Runs on Windows 98.
 
 - glzdoom32 branch:
 A merge of ZDoom32 with a later GZDoom master (roughly 2.2) from NOV 17 2016 with later fixes.

 - zdoom32 branch:
 Original ZDoom32 branch without the GL renderer.
 
 - gzdoomle branch:
 ZDoom LE (Legacy Edition) is a fork of the ZDoom 2.8.1 maintenance branch (https://github.com/rheit/zdoom/tree/maint)
for Windows 98 and old machines. Later merged with GZDoom as of AUG 18 2013, runs on GL 1.2 hardware.

 - zdoomle branch:
 Old ZDoom LE branch with OpenAL for win95 (released from the ZDOOM-LE repo).
 
 - zdoomcl branch
 ZDoom CLASSIC is a fork of ZDoom 2.1.4 for Windows 9x and pentium machines.
 Later merged with GZDoom 1.0.17 with later fixes and additions.


 ZDoom32 compiles with CMake 2.8.12, CodeBlocks 17.12 (TDM-GCC 5.1.0) and NASM 2.10.09.
 You'll need the following libraries: dx9mgw.zip (optional), fmodapi43623win-installer.exe and fluidsynth.7z (optional).
 Run CMake to generate a CodeBlocks makefile, you cannot link directly against dinput8.dll.
 Now Compiles with VS 2017 (v140_xp or v140 toolsets strongly recommended) and with gcc on Linux.

 Copyright © 1993-1996 id Software, 1998-2016 Randi Heit, 2002-2018 Christoph Oelckers, et al.
 Copyright © 2016-2018 Magnus Norddahl, Rachael Alexanderson and Alexey Lysiuk.

 These versions maintained by drfrag from zdoom.org. They include patches by Blzut3 and hail-to-the-ryzen.