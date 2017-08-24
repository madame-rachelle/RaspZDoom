#include "gl_pch.h"
/*
** Hires texture management
**
**---------------------------------------------------------------------------
** Copyright 2005 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of GZDoom or a GZDoom derivative, this code will be
**    covered by the terms of the GNU Lesser General Public License as published
**    by the Free Software Foundation; either version 2.1 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include "w_wad.h"
#include "m_png.h"
#include "r_draw.h"
#include "sbar.h"
#include "gi.h"
#include "cmdlib.h"
#include "sc_man.h"

#include "gl/gl_struct.h"
#include "gl/gl_texture.h"
#include "gl/gl_functions.h"

#include <il/il.h>
#include <il/ilu.h>

//==========================================================================
//
// This is just a dummy class to add hires textures to the texture manager
// If used by the software renderer it only makes sure that all the pointers
// point to valid data - but don't contain any actual texture information.
//
//==========================================================================

FHiresTexture::FHiresTexture (const char * name, int w, int h)
: Pixels(0)
{
	sprintf(Name, "%.8s", name);

	SourceLump=-1;
	bMasked = true;
	Width = w;
	Height = h;
	CalcBitSize();
	DummySpans[0].TopOffset = 0;
	DummySpans[0].Length = Height;
	DummySpans[1].TopOffset = 0;
	DummySpans[1].Length = 0;

	ScaleX = ScaleY = 8;
	UseType = TEX_Override;
}

FHiresTexture::~FHiresTexture ()
{
	Unload ();
}

void FHiresTexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

const BYTE *FHiresTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	if (spans_out != NULL)
	{
		*spans_out = DummySpans;
	}
	return GetPixels();
}

const BYTE *FHiresTexture::GetPixels ()
{
	if (Pixels == NULL)
	{
		Pixels = new BYTE[Width*Height];
		memset(Pixels, 255, Width*Height);
	}
	return Pixels;
}



static bool IsDoomPatch(const char * data, int length)
{
	const patch_t * foo = (const patch_t *)data;
	
	int height = LittleShort(foo->height);
	int width = LittleShort(foo->width);
	bool gapAtStart=true;
	
	if (height > 0 && height < 510 && width > 0 && width <= 2048 && width < length/4)
	{
		// The dimensions seem like they might be valid for a patch, so
		// check the column directory for extra security. At least one
		// column must begin exactly at the end of the column directory,
		// and none of them must point past the end of the patch.
		bool gapAtStart = true;
		int x;
	
		for (x = 0; x < width; ++x)
		{
			DWORD ofs = LittleLong(foo->columnofs[x]);
			if (ofs == (DWORD)width * 4 + 8)
			{
				gapAtStart = false;
			}
			else if (ofs >= length-1)	// Need one byte for an empty column
			{
				return false;
			}
			else
			{
				// Ensure this column does not extend beyond the end of the patch
				const BYTE *foo2 = (const BYTE *)foo;
				while (ofs < length)
				{
					if (foo2[ofs] == 255)
					{
						break;
					}
					ofs += foo2[ofs+1] + 4;
				}
				if (ofs >= length)
				{
					return false;
				}
			}
		}
		return !gapAtStart;
	}
	return false;
}
	
//==========================================================================
//
// Tries to load a lump with DevIL and creates a texture if successful
//
//==========================================================================
FHiresTexture * FHiresTexture::TryLoad(int lumpnum)
{
	static bool unstable=false;
	unsigned char *buffer = NULL;
	ILuint imgID;
	FMemLump memLump;
	char name[9];

	if (Wads.LumpLength(lumpnum)==0) return NULL;

	memLump = Wads.ReadLump(lumpnum);
	
	if (IsDoomPatch((const char*)memLump.GetMem(), Wads.LumpLength(lumpnum))) return NULL;

	//Printf("Trying to load %s\n", Wads.GetLumpFullName(lumpnum));
	if (unstable) return NULL;

	try
	{
		ilGenImages(1, &imgID);
		ilBindImage(imgID);
	}
	catch (...)
	{
		Printf("DevIL crashed unexpectedly for %s\n", Wads.GetLumpFullName(lumpnum));
		unstable=true;
		return NULL;
	}

	try
	{
		if (ilLoadL(IL_TYPE_UNKNOWN, memLump.GetMem(), Wads.LumpLength(lumpnum)))
		{
			int width = ilGetInteger(IL_IMAGE_WIDTH);
			int height = ilGetInteger(IL_IMAGE_HEIGHT);
	
	
			Wads.GetLumpName(name, lumpnum);
			FHiresTexture * tex = new FHiresTexture(name, width, height);
			if (tex)
			{
				FGLTexture * gltex = FGLTexture::ValidateTexture(tex);
				if (gltex)
				{
					gltex->HiresLump=lumpnum;
				}
				tex->UseType=FTexture::TEX_MiscPatch;
			}
			ilDeleteImages(1, &imgID);
			return tex;
		}
	}
	catch(...)
	{
		// DevIL isn't robust enough to handle every data thrown at it
		// so we have to be prepared...
		Printf("DEvIL crashed for %s\n", Wads.GetLumpFullName(lumpnum));
	}
	ilDeleteImages(1, &imgID);
	return NULL;
}

//==========================================================================
//
// Checks for the presence of a hires texture replacement
//
//==========================================================================
bool FGLTexture::CheckExternalFile()
{
	static const char * doom1texpath[]= {
		"%stextures/doom/doom1/%s.%s", "%stextures/doom/doom1/%s-ck.%s", 
			"%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * doom2texpath[]= {
		"%stextures/doom/doom2/%s.%s", "%stextures/doom/doom2/%s-ck.%s", 
			"%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * pluttexpath[]= {
		"%stextures/doom/plut/%s.%s", "%stextures/doom/plut/%s-ck.%s", 
		"%stextures/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/%s-ck.%s", 
			"%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * tnttexpath[]= {
		"%stextures/doom/tnt/%s.%s", "%stextures/doom/tnt/%s-ck.%s", 
		"%stextures/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/%s-ck.%s", 
			"%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * heretictexpath[]= {
		"%stextures/heretic/%s.%s", "%stextures/heretic/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * hexentexpath[]= {
		"%stextures/hexen/%s.%s", "%stextures/hexen/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * strifetexpath[]= {
		"%stextures/strife/%s.%s", "%stextures/strife/%s-ck.%s", "%stextures/%s.%s", "%stextures/%s-ck.%s", NULL
	};

	static const char * doom1flatpath[]= {
		"%sflats/doom/doom1/%s.%s", "%stextures/doom/doom1/flat-%s.%s", 
			"%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * doom2flatpath[]= {
		"%sflats/doom/doom2/%s.%s", "%stextures/doom/doom2/flat-%s.%s", 
			"%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * plutflatpath[]= {
		"%sflats/doom/plut/%s.%s", "%stextures/doom/plut/flat-%s.%s", 
		"%sflats/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/flat-%s.%s", 
			"%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * tntflatpath[]= {
		"%sflats/doom/tnt/%s.%s", "%stextures/doom/tnt/flat-%s.%s", 
		"%sflats/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/flat-%s.%s", 
			"%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * hereticflatpath[]= {
		"%sflats/heretic/%s.%s", "%stextures/heretic/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * hexenflatpath[]= {
		"%sflats/hexen/%s.%s", "%stextures/hexen/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * strifeflatpath[]= {
		"%sflats/strife/%s.%s", "%stextures/strife/flat-%s.%s", "%sflats/%s.%s", "%stextures/flat-%s.%s", NULL
	};

	static const char * doom1patchpath[]= {
		"%spatches/doom/doom1/%s.%s", "%spatches/doom/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * doom2patchpath[]= {
		"%spatches/doom/doom2/%s.%s", "%spatches/doom/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * plutpatchpath[]= {
		"%spatches/doom/plut/%s.%s", "%spatches/doom/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * tntpatchpath[]= {
		"%spatches/doom/tnt/%s.%s", "%spatches/doom/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * hereticpatchpath[]= {
		"%spatches/heretic/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * hexenpatchpath[]= {
		"%spatches/hexen/%s.%s", "%spatches/%s.%s", NULL
	};

	static const char * strifepatchpath[]= {
		"%spatches/strife/%s.%s", "%spatches/%s.%s", NULL
	};

	char checkName[250];
	const char ** checklist;
	BYTE useType=tex->UseType;

	if (useType==FTexture::TEX_SkinSprite || useType==FTexture::TEX_Decal || useType==FTexture::TEX_FontChar)
	{
		return false;
	}

	bool ispatch = (useType==FTexture::TEX_MiscPatch || useType==FTexture::TEX_Sprite) ;

	// for patches this doesn't work yet
	if (ispatch) return false;

	switch (gameinfo.gametype)
	{
	case GAME_Doom:
		switch (gamemission)
		{
		case doom:
			checklist = ispatch ? doom1patchpath : useType==FTexture::TEX_Flat? doom1flatpath : doom1texpath;
			break;
		case doom2:
			checklist = ispatch ? doom2patchpath : useType==FTexture::TEX_Flat? doom2flatpath : doom2texpath;
			break;
		case pack_tnt:
			checklist = ispatch ? tntpatchpath : useType==FTexture::TEX_Flat? tntflatpath : tnttexpath;
			break;
		case pack_plut:
			checklist = ispatch ? plutpatchpath : useType==FTexture::TEX_Flat? plutflatpath : pluttexpath;
			break;
		default:
			return false;
		}
		break;

	case GAME_Heretic:
		checklist = ispatch ? hereticpatchpath : useType==FTexture::TEX_Flat? hereticflatpath : heretictexpath;
		break;
	case GAME_Hexen:
		checklist = ispatch ? hexenpatchpath : useType==FTexture::TEX_Flat? hexenflatpath : hexentexpath;
		break;
	case GAME_Strife:
		checklist = ispatch ?strifepatchpath : useType==FTexture::TEX_Flat? strifeflatpath : strifetexpath;
		break;
	default:
		return false;
	}

	while (*checklist)
	{
		static const char * extensions[] = { "PNG", "JPG", "TGA", "PCX", NULL };

		for (const char ** extp=extensions; *extp; extp++)
		{
			sprintf(checkName, *checklist, progdir, tex->Name, *extp);
			if (_access(checkName, 0) == 0) 
			{
				hirespath=copystring(checkName);
				return true;
			}
		}
		checklist++;
	}
	return false;
}


//==========================================================================
//
// Checks for alpha channel information
//
//==========================================================================

void FGLTexture::CheckForAlpha(const unsigned char * buffer)
{
	if (!tex->bAlphaChannel)
	{
		for(int i=0; i< Width*Height; i++)
		{
			DWORD pixel = ((const DWORD*)buffer)[i]&0xff000000;
			if (pixel!=0 && pixel!=0xff000000)
			{
				tex->bAlphaChannel = true;
				break;
			}
		}
	}
}

//==========================================================================
//
// Loads a hires texture
//
//==========================================================================
unsigned char *FGLTexture::LoadFile(const char *fileName, int *width, int *height, int cm)
{
	unsigned char *buffer = NULL;
	byte *data;
	ILuint imgID;
	int imgSize;

	ilGenImages(1, &imgID);
	ilBindImage(imgID);
	ilLoad(IL_TYPE_UNKNOWN, (const ILstring)fileName);
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	*width = ilGetInteger(IL_IMAGE_WIDTH);
	*height = ilGetInteger(IL_IMAGE_HEIGHT);
	imgSize = *width * *height;
	buffer = new unsigned char[4*imgSize];
	data = ilGetData();

	if (strstr(fileName, "-ck."))
	{
		// This is a crappy Doomsday color keyed image
		// We have to remove the key manually. :(
		DWORD * dwdata=(DWORD*)data;
		for(int i=0;i<imgSize;i++)
		{
			if (dwdata[i]==0xffffff00 || dwdata[i]==0xffff00ff) dwdata[i]=0;
		}
	}

	// Since I have to copy the image anyway I'll do the 
	// palette manipulation in the same step.
	ModifyPalette((PalEntry*)buffer, (PalEntry*)data, cm, imgSize, true);

	ilDeleteImages(1, &imgID);
	CheckForAlpha(buffer);
	return buffer;
}


//==========================================================================
//
// Loads a hires texture from a lump
//
//==========================================================================
unsigned char * FGLTexture::LoadFromLump(int lumpNum, int *width, int *height, int cm)
{
	unsigned char *buffer = NULL;
	byte *data;
	ILuint imgID;
	int imgSize;
	FMemLump memLump;

	memLump = Wads.ReadLump(lumpNum);

	ilGenImages(1, &imgID);
	ilBindImage(imgID);
	ilLoadL(IL_TYPE_UNKNOWN, memLump.GetMem(), Wads.LumpLength(lumpNum));

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	*width = ilGetInteger(IL_IMAGE_WIDTH);
	*height = ilGetInteger(IL_IMAGE_HEIGHT);
	imgSize = *width * *height;
	buffer = new unsigned char[imgSize*4];
	data = ilGetData();

	// Since I have to copy the image anyway I'll do the 
	// palette manipulation in the same step.
	ModifyPalette((PalEntry*)buffer, (PalEntry*)data, cm, imgSize, true);

	ilDeleteImages(1, &imgID);

	CheckForAlpha(buffer);
	return buffer;
}

//==========================================================================
//
// Checks for the presence of a hires texture replacement and loads it
//
//==========================================================================
unsigned char *FGLTexture::LoadHiresTexture(int *width, int *height,int cm)
{
	if (HiresLump>=0)
	{
		// This is an internally defined hires texture.
		return LoadFromLump(HiresLump, width, height, cm);
	}

	if (HiresLump==-1)
	{
		HiresLump = CheckExternalFile()? -2:-3;
	}
	if (hirespath != NULL)
	{
		unsigned char * buffer = LoadFile(hirespath, width, height, cm);
		if (buffer) return buffer;

		// don't try again
		HiresLump=-3;
		delete [] hirespath;
		hirespath=NULL;
	}
	return NULL;
}


//==========================================================================
//
// Adds all hires texture definitions.
//
//==========================================================================

void FGLTexture::LoadHiresTextures()
{
	int remapLump, lastLump;
	char src[9];
	bool is32bit;
	int width, height;
	int type,mode;

	lastLump = 0;
	src[8] = '\0';

	while ((remapLump = Wads.FindLump("HIRESTEX", &lastLump)) != -1)
	{
		SC_OpenLumpNum(remapLump, "HIRESTEX");
		while (SC_GetString())
		{
			if (SC_Compare("remap")) // remap an existing texture
			{
				SC_MustGetString();
				
				// allow selection by type
				if (SC_Compare("wall")) type=FTexture::TEX_Wall, mode=FTextureManager::TEXMAN_Overridable;
				else if (SC_Compare("flat")) type=FTexture::TEX_Flat, mode=FTextureManager::TEXMAN_Overridable;
				else if (SC_Compare("sprite")) type=FTexture::TEX_Sprite, mode=0;
				else type = FTexture::TEX_Any;
				
				sc_String[8]=0;

				int tex = TexMan.CheckForTexture(sc_String, type, mode);

				SC_MustGetString();

				if (tex>0) 
				{
					FTexture * texx = TexMan[tex];
					FGLTexture * gtex = FGLTexture::ValidateTexture(texx);
					if (gtex) 
					{
						gtex->HiresLump = Wads.CheckNumForFullName(sc_String);
						if (gtex->HiresLump < 0) gtex->HiresLump = Wads.CheckNumForName(sc_String, ns_graphics);
						if (!gtex->HiresLump) Printf("Unable to set hires texture for '%s': %s not found\n", texx->Name, sc_String);
					}
				}
			}
			else if (SC_Compare("define")) // define a new "fake" texture
			{
				SC_GetString();
				memcpy(src, sc_String, 8);

				int lumpnum = Wads.CheckNumForFullName(sc_String);
				if (lumpnum < 0) lumpnum = Wads.CheckNumForName(sc_String, ns_graphics);

				SC_GetString();
				is32bit = !!SC_Compare("force32bit");
				if (!is32bit) SC_UnGet();

				SC_GetNumber();
				width = sc_Number;
				SC_GetNumber();
				height = sc_Number;

				if (lumpnum>=0)
				{
					int oldtex = TexMan.CheckForTexture(src, FTexture::TEX_Override);
					FTexture * tex = new FHiresTexture(src, width, height);

					if (oldtex>=0) TexMan.ReplaceTexture(oldtex, tex, true);
					else TexMan.AddTexture(tex);

					FGLTexture * gtex = FGLTexture::ValidateTexture(tex);
					if (gtex) 
					{
						gtex->HiresLump = lumpnum;
					}
				}				
				//else Printf("Unable to define hires texture '%s'\n", tex->Name);
			}
		}
		SC_Close();
	}
}
	