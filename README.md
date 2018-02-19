 ZDoom LE (Legacy Edition), a fork of the ZDoom 2.8 maintenance branch (https://github.com/rheit/zdoom/tree/maint)
for Windows 98 and old machines. Now merged with GZDoom 1.8.
 Has a lot of later official patches and low detail modes have been restored.
 Created at vogons (http://www.vogons.org) by blue-green-frog and continued by drfrag.
 Built with MinGW using Allegro DX9 libraries (no XInput support). Can compile as well with allegro DX8 libraries
 but without Direct3D support.

 Compiles with CMake 2.8.12, CodeBlocks 16.01 (TDM-GCC 4.9.2) and NASM 2.10.09. You'll need the following libraries:
dx9mgw.zip, dx80_mgw.zip (optional), fmodapi43623win-installer.exe and fluidsynth.7z (optional).
 You can still compile with TDM-GCC 4.6.1 but then you wouldn't get the hqNx MMX HQ resize modes.
 The original LE for win95 is at the ZDoom-LE repository.
 
 Some notes:
 - You can link directly against the dlls but not for DX (dinput).
 - Run CMake to generate a CodeBlocks makefile and fill or change the paths to executables and libraries.
 
 You can follow the following guide:
 https://zdoom.org/wiki/Compile_ZDoom_on_Windows
 