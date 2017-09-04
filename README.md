 ZDoom LE (Legacy Edition), a fork of the ZDoom 2.8.1 maintenance branch (https://github.com/rheit/zdoom/tree/maint)
for Windows 98 and old machines. Now merged with GZDoom 1.8.4a.
 It's compiled with pentium ii optimizations and low detail modes have been restored.
 Created at vogons (http://www.vogons.org) by blue-green-frog and continued by drfrag.
 Compiles with MinGW using Allegro DX9 libraries (Xinput support has been removed) and a modified OpenAL-soft.
 Can compile as well with allegro DX8 libraries but without Direct3D support.

 Compiles with CMake 2.8.12, CodeBlocks 16.01 (TDM-GCC 4.9.2) and NASM 2.10.09. You'll need the following libraries:
dx9mgw.zip, dx80_mgw.zip (optional), fmodapi42636win32-installer.exe and fluidsynth.7z (optional).
 It's better to install TDM-GCC 4.6.1 since it's faster than 4.9.2.
 
 Some notes:
 - You can link directly against the dlls but not for DX (dinput).
 - Run CMake to generate a CodeBlocks makefile and fill or change the paths to executables and libraries.
 
 You can follow the following guide:
 https://zdoom.org/wiki/Compile_ZDoom_on_Windows
 