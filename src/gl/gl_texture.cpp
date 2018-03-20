#include "gl_pch.h"
/*
** gltexture.cpp
** The texture classes for hardware rendering
** (Even though they are named 'gl' there is nothing hardware dependent
**  in this file. That is all encapsulated in the GLTexture class.)
**
**---------------------------------------------------------------------------
** Copyright 2004-2005 Christoph Oelckers
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

#include "gl/gl_struct.h"
#include "gl/gl_texture.h"
#include "gl/gl_functions.h"
#include "gl/gl_hqresize.h"

#include <il/il.h>
#include <il/ilu.h>

CUSTOM_CVAR(Bool, gl_warp_shader, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (!self) gl_SetShaderForWarp(0,0);
}

CUSTOM_CVAR(Bool, gl_texture_usehires, true, CVAR_ARCHIVE|CVAR_NOINITCALL)
{
	FGLTexture::FlushAll();
}

CVAR(Bool, gl_precache, false, CVAR_ARCHIVE)

// internal parameters for ModifyPalette
enum SpecialModifications
{
	CM_ICE=-2,					// The bluish ice translation for frozen corpses
	CM_GRAY=-3,					// a simple grayscale map for colorizing blood splats
};


//===========================================================================
// 
// Creates one of the special palette translations for the given palette
//
//===========================================================================
void ModifyPalette(PalEntry * pout, PalEntry * pin, int cm, int count, bool bgra)
{
	int i;
	int fac;

	static const BYTE IcePalette[16][3] =
	{
		{  10,  8, 18 },
		{  15, 15, 26 },
		{  20, 16, 36 },
		{  30, 26, 46 },
		{  40, 36, 57 },
		{  50, 46, 67 },
		{  59, 57, 78 },
		{  69, 67, 88 },
		{  79, 77, 99 },
		{  89, 87,109 },
		{  99, 97,120 },
		{ 109,107,130 },
		{ 118,118,141 },
		{ 128,128,151 },
		{ 138,138,162 },
		{ 148,148,172 }
	};

	switch(cm)
	{
	case CM_DEFAULT:
		if (pin != pout)
		{
			memcpy(pout, pin, count * sizeof(PalEntry));
		}
		break;

	case CM_INVERT:
		// Doom's inverted invulnerability map
		for(i=0;i<count;i++)
		{
			int gray = 255-((pin[i].r*77 + pin[i].g*143 + pin[i].b*37) >> 8);
			pout[i].r = pout[i].g = pout[i].b = clamp<int>(gray, 0, 255);
			pout[i].a = pin[i].a;
		}
		break;

	case CM_GOLDMAP:
		// Heretic's golden invulnerability map
		if (!bgra)
		{
			for(i=0;i<count;i++)
			{
				int gray = (pin[i].r*77 + pin[i].g*143 + pin[i].b*37)>>8;
				pout[i].r = clamp<int>(gray+(gray>>1), 0, 255);
				pout[i].g = clamp<int>(gray, 0, 255);
				pout[i].b = 0;
				pout[i].a = pin[i].a;
			}
		}
		else	// Raw image data is stored differently
		{
			for(i=0;i<count;i++)
			{
				int gray = (pin[i].r*77 + pin[i].g*143 + pin[i].b*37)>>8;
				pout[i].b = clamp<int>(gray+(gray>>1), 0, 255);
				pout[i].g = clamp<int>(gray, 0, 255);
				pout[i].r = 0;
				pout[i].a = pin[i].a;
			}
		}
		break;

	case CM_GRAY:
		// this is used for colorization of blood.
		// To get the best results the brightness is taken from 
		// the most intense component and not averaged because that would be too dark.
		for(i=0;i<count;i++)
		{
			pout[i].r = pout[i].g = pout[i].b = max(max(pin[i].r, pin[i].g), pin[i].b);
			pout[i].a = pin[i].a;
		}
		break;

	case CM_ICE:
		// Create the ice translation table, based on Hexen's.
		// Since this is done in True Color the purplish tint is fully preserved - even in Doom!
		if (!bgra)
		{
			for(i=0;i<count;i++)
			{
				int gray=(pin[i].r*77 + pin[i].g*143 + pin[i].b*37)>>12;

				pout[i].r = IcePalette[gray][0];
				pout[i].g = IcePalette[gray][1];
				pout[i].b = IcePalette[gray][2];
				pout[i].a = pin[i].a;
			}
		}
		else
		{
			for(i=0;i<count;i++)
			{
				int gray=(pin[i].r*77 + pin[i].g*143 + pin[i].b*37)>>12;

				pout[i].b = IcePalette[gray][0];
				pout[i].g = IcePalette[gray][1];
				pout[i].r = IcePalette[gray][2];
				pout[i].a = pin[i].a;
			}
		}
		break;
	
	default:
		// Boom colormaps.
		if (cm>=CM_FIRSTCOLORMAP && cm<CM_FIRSTCOLORMAP+numfakecmaps)
		{
			if (count<=256)	// This does not work for raw image data because it assumes
							// the use of the base palette!
			{
				// CreateTexBuffer has already taken care of needed palette mapping so this
				// buffer is guaranteed to be in the base palette.
				byte * cmapp = &realcolormaps [NUMCOLORMAPS*256*(cm - CM_FIRSTCOLORMAP)];

				for(i=0;i<count;i++)
				{
					pout[i].r = GPalette.BaseColors[*cmapp].r;
					pout[i].g = GPalette.BaseColors[*cmapp].g;
					pout[i].b = GPalette.BaseColors[*cmapp].b;
					pout[i].a = pin[i].a;
					cmapp++;
				}
			}
			else if (pin != pout)
			{
				// Boom colormaps cannot be applied to hires texture replacements.
				// For those you have to set the colormap usage to  'blend'.
				memcpy(pout, pin, count * sizeof(PalEntry));
			}
		}
		else if (cm<=CM_DESAT31)
		{
			// Desaturated light settings.
			fac=cm-CM_DESAT0;
			for(i=0;i<count;i++)
			{
				int gray=(pin[i].r*77 + pin[i].g*143 + pin[i].b*36)>>8;

				pout[i].r = (pin[i].r*(31-fac) + gray*fac)/31;
				pout[i].g = (pin[i].g*(31-fac) + gray*fac)/31;
				pout[i].b = (pin[i].b*(31-fac) + gray*fac)/31;
				pout[i].a = pin[i].a;
			}
		}
		else if (pin!=pout)
		{
			memcpy(pout, pin, count * sizeof(PalEntry));
		}
		break;
	}
}


//===========================================================================
//
// True Color texture copy function
//
//===========================================================================
static void CopyPixelData(BYTE * buffer, int texwidth, int texheight, int originx, int originy,
						  const BYTE * patch, int pix_width, int pix_height, 
						  int step_x, int step_y,
						  intptr_t cm, int translation, PalEntry * palette)
{
	PalEntry penew[256];
	int srcwidth=pix_width;
	int srcheight=pix_height;
	int pitch=texwidth;

	int x,y,pos,i;
	
	// clip source rectangle to destination
	if (originx<0)
	{
		srcwidth+=originx;
		patch-=originx*step_x;
		originx=0;
		if (srcwidth<=0) return;
	}
	if (originx+srcwidth>texwidth)
	{
		srcwidth=texwidth-originx;
		if (srcwidth<=0) return;
	}
		
	if (originy<0)
	{
		srcheight+=originy;
		patch-=originy*step_y;
		originy=0;
		if (srcheight<=0) return;
	}
	if (originy+srcheight>texheight)
	{
		srcheight=texheight-originy;
		if (srcheight<=0) return;
	}
	buffer+=4*originx + 4*pitch*originy;


	if (translation!=DIRECT_PALETTE)
	{
		// CM_SHADE is an alpha map with 0==transparent and 1==opaque
		if (cm==CM_SHADE) 
		{
			for(int i=0;i<256;i++) 
			{
				if (palette[i].a!=255)
					penew[i]=PalEntry(255-i,255,255,255);
				else
					penew[i]=0xffffffff;	// If the palette contains transparent colors keep them.
			}
		}
		else
		{
			// apply any translation. 
			if (translation)
			{
				// The ice and blood color translations are done in true color
				// because that yields better results.
				switch(translation)
				{
				case (TRANSLATION_Standard<<8) | 8:
					ModifyPalette(penew, palette, CM_GRAY, 256);
					break;

				case (TRANSLATION_Standard<<8) | 7:
					ModifyPalette(penew, palette, CM_ICE, 256);
					break;

				default:
					const unsigned char * tbl = &translationtables[translation>>8][256*(translation&0xff)];

					for(i = 0; i < 256; i++)
					{
						penew[i] = (palette[tbl[i]]&0xffffff) | (palette[i]&0xff000000);
					}
				}
			}
			else
			{
				memcpy(penew, palette, 256*sizeof(PalEntry));
			}
			if (cm!=0)
			{
				// Apply color modifications like invulnerability, desaturation and Boom colormaps
				ModifyPalette(penew, penew, cm, 256);
			}
		}
	}
	else
	{
		// fonts create their own translation palettes
		BYTE * translationp=(BYTE*)cm;
		for(int i=0;i<256;i++)
		{
			penew[i].r=*translationp++;
			penew[i].g=*translationp++;
			penew[i].b=*translationp++;
			penew[i].a=0;
		}
		penew[0].a=255;
	}
	// Now penew contains the actual palette that is to be used for creating the image.

	// convert the image according to the translated palette.
	// Please note that the alpha of the passed palette is inverted. This is
	// so that the base palette can be used without constantly changing it.
	// This can also handle full PNG translucency.
	for (y=0;y<srcheight;y++)
	{
		pos=4*(y*pitch);
		for (x=0;x<srcwidth;x++,pos+=4)
		{
			int v=(unsigned char)patch[y*step_y+x*step_x];
			if (penew[v].a==0)
			{
				buffer[pos]=penew[v].r;
				buffer[pos+1]=penew[v].g;
				buffer[pos+2]=penew[v].b;
				buffer[pos+3]=255-penew[v].a;
			}
			else if (penew[v].a!=255)
			{
				buffer[pos  ] = (buffer[pos  ] * penew[v].a + penew[v].r * (1-penew[v].a)) / 255;
				buffer[pos+1] = (buffer[pos+1] * penew[v].a + penew[v].g * (1-penew[v].a)) / 255;
				buffer[pos+2] = (buffer[pos+2] * penew[v].a + penew[v].b * (1-penew[v].a)) / 255;
				buffer[pos+3] = clamp<int>(buffer[pos+3] + (( 255-buffer[pos+3]) * (255-penew[v].a))/255, 0, 255);
			}
		}
	}
}

//===========================================================================
//
// FTexture::CopyTrueColorPixels 
//
// this is the generic case that can handle
// any properly implemented texture for software rendering.
// Its drawback is that it is limited to the base palette which is
// why all classes that handle different palettes should subclass this
// method
//
//===========================================================================

void FTexture::CopyTrueColorPixels(BYTE * buffer, int buf_width, int buf_height, int x, int y, 
									 intptr_t cm, int translation)
{
	PalEntry * palette = screen->GetPalette();
	palette[0].a=255;
	CopyPixelData(buffer, buf_width, buf_height, x, y,
				  GetPixels(), Width, Height, Height, 1, 
				  cm, translation, palette);

	Unload();
	palette[0].a=0;
}

//===========================================================================
//
// FMultipatchTexture::CopyTrueColorPixels
//
// This needs to be subclassed so it can handle textures that use
// patches with different palettes
//
//===========================================================================

void FMultiPatchTexture::CopyTrueColorPixels(BYTE * buffer, int buf_width, int buf_height, int x, int y, 
									 intptr_t cm, int translation)
{
	for(int i=0;i<NumParts;i++)
	{
		Parts[i].Texture->GetWidth();
		Parts[i].Texture->CopyTrueColorPixels(buffer, buf_width, buf_height, 
											  x+Parts[i].OriginX, y+Parts[i].OriginY, cm, translation);
	}
}

//===========================================================================
//
// FPNGTexture::CopyTrueColorPixels
//
// Preserves the full PNG palette (unlike software mode)
//
//===========================================================================

void FPNGTexture::CopyTrueColorPixels(BYTE * buffer, int buf_width, int buf_height, int x, int y, 
									 intptr_t cm, int translation)
{
	// Parse pre-IDAT chunks. I skip the CRCs. Is that bad?
	PalEntry pe[256];
	DWORD len, id;
	FWadLump lump = Wads.OpenLumpNum (SourceLump);

	lump.Seek (33, SEEK_SET);
	for(int i=0;i<256;i++) pe[i]=PalEntry(0,i,i,i);	// default to a gray map

	lump >> len >> id;
	while (id != MAKE_ID('I','D','A','T') && id != MAKE_ID('I','E','N','D'))
	{
		len = BigLong((unsigned int)len);
		switch (id)
		{
		default:
			lump.Seek (len, SEEK_CUR);
			break;

		case MAKE_ID('P','L','T','E'):
			for(int i=0;i<PaletteSize;i++)
				lump >> pe[i].r >> pe[i].g >> pe[i].b;
			break;

		case MAKE_ID('t','R','N','S'):
			for(int i=0;i<len;i++)
			{
				lump >> pe[i].a;
				pe[i].a=255-pe[i].a;	// use inverse alpha so the default palette can be used unchanged
			}
			break;
		}
		lump >> len >> len;	// Skip CRC
		id = MAKE_ID('I','E','N','D');
		lump >> id;
	}

	if (ColorType == 0 || ColorType == 3)	/* Grayscale and paletted */
	{
		BYTE * Pixels = new BYTE[Width*Height];

		lump.Seek (StartOfIDAT, SEEK_SET);
		lump >> len >> id;
		M_ReadIDAT (&lump, Pixels, Width, Height, Width, BitDepth, ColorType, Interlace, BigLong((unsigned int)len));
		
		CopyPixelData(buffer, buf_width, buf_height, x, y,
					  Pixels, Width, Height, 1, Width,
					  cm, translation, pe);

		delete[] Pixels;
	}
}

//===========================================================================
//
// FWarpTexture::CopyTrueColorPixels
//
// Since the base texture can be anything the warping must be done in
// true color
//
//===========================================================================

void FWarpTexture::CopyTrueColorPixels(BYTE * buffer, int buf_width, int buf_height, int xx, int yy, 
									 intptr_t cm, int translation)
{
	if (gl_warp_shader)
	{
		this->SourcePic->CopyTrueColorPixels(buffer, buf_width, buf_height, xx, yy, cm, translation);
		return;
	}

	unsigned long * in=new unsigned long[Width*Height];
	unsigned long * out;
	bool direct;
	
	if (Width == buf_width && Height == buf_height && xx==0 && yy==0)
	{
		out = (unsigned long*)buffer;
		direct=true;
	}
	else
	{
		out = new unsigned long[Width*Height];
		direct=false;
	}

	GenTime = r_FrameTime;
	if (SourcePic->bMasked) memset(in, 0, Width*Height*sizeof(long));
	SourcePic->CopyTrueColorPixels((BYTE*)in, Width, Height, 0, 0, cm, translation);

	static unsigned long linebuffer[4096];	// that's the maximum texture size for most graphics cards!
	int timebase = r_FrameTime*23/28;
	int xsize = Width;
	int ysize = Height;
	int xmask = xsize - 1;
	int ymask = ysize - 1;
	int ds_xbits;
	int i,x;

	for(ds_xbits=-1,i=Width; i; i>>=1, ds_xbits++);

	for (x = xsize-1; x >= 0; x--)
	{
		int yt, yf = (finesine[(timebase+(x+17)*128)&FINEMASK]>>13) & ymask;
		const unsigned long *source = in + x;
		unsigned long *dest = out + x;
		for (yt = ysize; yt; yt--, yf = (yf+1)&ymask, dest += xsize)
		{
			*dest = *(source+(yf<<ds_xbits));
		}
	}
	timebase = r_FrameTime*32/28;
	int y;
	for (y = ysize-1; y >= 0; y--)
	{
		int xt, xf = (finesine[(timebase+y*128)&FINEMASK]>>13) & xmask;
		unsigned long *source = out + (y<<ds_xbits);
		unsigned long *dest = linebuffer;
		for (xt = xsize; xt; xt--, xf = (xf+1)&xmask)
		{
			*dest++ = *(source+xf);
		}
		memcpy (out+y*xsize, linebuffer, xsize*sizeof(unsigned long));
	}

	if (!direct)
	{
		// Negative offsets cannot occur here.
		if (xx<0) xx=0;
		if (yy<0) yy=0;

		unsigned long * targ = ((unsigned long*)buffer) + xx + yy*buf_width;
		int linelen=MIN<int>(Width, buf_width-xx);
		int linecount=MIN<int>(Height, buf_height-yy);

		for(i=0;i<linecount;i++)
		{
			memcpy(targ, &out[Width*i], linelen*sizeof(unsigned long));
			targ+=buf_width;
		}
		delete [] out;
	}
	delete [] in;
	GenTime=r_FrameTime;
}

//===========================================================================
//
// FWarpTexture::CopyTrueColorPixels
//
// Since the base texture can be anything the warping must be done in
// true color
//
//===========================================================================

void FWarp2Texture::CopyTrueColorPixels(BYTE * buffer, int buf_width, int buf_height, int xx, int yy, 
									 intptr_t cm, int translation)
{
	if (gl_warp_shader)
	{
		this->SourcePic->CopyTrueColorPixels(buffer, buf_width, buf_height, xx, yy, cm, translation);
		return;
	}

	unsigned long * in=new unsigned long[Width*Height];
	unsigned long * out;
	bool direct;
	
	if (Width == buf_width && Height == buf_height && xx==0 && yy==0)
	{
		out = (unsigned long*)buffer;
		direct=true;
	}
	else
	{
		out = new unsigned long[Width*Height];
		direct=false;
	}

	GenTime = r_FrameTime;
	if (SourcePic->bMasked) memset(in, 0, Width*Height*sizeof(long));
	SourcePic->CopyTrueColorPixels((BYTE*)in, Width, Height, 0, 0, cm, translation);

	int xsize = Width;
	int ysize = Height;
	int xmask = xsize - 1;
	int ymask = ysize - 1;
	int ybits;
	int x, y;
	int i;

	for(ybits=-1,i=ysize; i; i>>=1, ybits++);

	DWORD timebase = r_FrameTime * 40 / 28;
	for (x = xsize-1; x >= 0; x--)
	{
		for (y = ysize-1; y >= 0; y--)
		{
			int xt = (x + 128
				+ ((finesine[(y*128 + timebase*5 + 900) & 8191]*2)>>FRACBITS)
				+ ((finesine[(x*256 + timebase*4 + 300) & 8191]*2)>>FRACBITS)) & xmask;
			int yt = (y + 128
				+ ((finesine[(y*128 + timebase*3 + 700) & 8191]*2)>>FRACBITS)
				+ ((finesine[(x*256 + timebase*4 + 1200) & 8191]*2)>>FRACBITS)) & ymask;
			const unsigned long *source = in + (xt << ybits) + yt;
			unsigned long *dest = out + (x << ybits) + y;
			*dest = *source;
		}
	}

	if (!direct)
	{
		// Negative offsets cannot occur here.
		if (xx<0) xx=0;
		if (yy<0) yy=0;

		unsigned long * targ = ((unsigned long*)buffer) + xx + yy*buf_width;
		int linelen=MIN<int>(Width, buf_width-xx);
		int linecount=MIN<int>(Height, buf_height-yy);

		for(i=0;i<linecount;i++)
		{
			memcpy(targ, &out[Width*i], linelen*sizeof(unsigned long));
			targ+=buf_width;
		}
		delete [] out;
	}
	delete [] in;
	GenTime=r_FrameTime;
}


//===========================================================================
//
// The GL texture maintenance class
//
//===========================================================================
TArray<FGLTexture *> * FGLTexture::gltextures;

//===========================================================================
//
// Constructor
//
//===========================================================================
FGLTexture::FGLTexture(FTexture * tx)
{
	tex = tx;

	glpatch=NULL;
	gltexture=NULL;

	HiresLump=-1;
	hirespath=NULL;

	areacount = 0;
	areas = NULL;

	bSkybox=false;

	Width = tex->GetWidth();
	Height = tex->GetHeight();
	LeftOffset = tex->LeftOffset;
	TopOffset = tex->TopOffset;

	scalex = tex->ScaleX ? tex->ScaleX/8.f:1.f;
	scaley = tex->ScaleY ? tex->ScaleY/8.f:1.f;

	RenderWidth=(short)(Width/scalex);
	RenderHeight=(short)(Height/scaley);

	// a little adjustment to make sprites look better with texture filtering:
	// create a 1 pixel wide empty frame around them.
	if (tex->UseType == FTexture::TEX_Sprite || 
		tex->UseType == FTexture::TEX_SkinSprite || 
		tex->UseType == FTexture::TEX_Decal)
	{
		RenderWidth+=2;
		RenderHeight+=2;
		Width+=2;
		Height+=2;
		LeftOffset+=1;
		TopOffset+=1;
	}
	if (!gltextures) gltextures = new TArray<FGLTexture *>;
	index = gltextures->Push(this);

	if (tex->bHasCanvas) scaley=-scaley;

}

//===========================================================================
//
// Destructor
//
//===========================================================================

FGLTexture::~FGLTexture()
{
	Clean(true);
	if (areas) delete [] areas;
	if (hirespath) delete [] hirespath;

	for(int i=0;i<gltextures->Size();i++)
	{
		if ((*gltextures)[i]==this) 
		{
			gltextures->Delete(i);
			break;
		}
	}
}

//===========================================================================
//
// GetRect
//
//===========================================================================
void FGLTexture::GetRect(GL_RECT * r) const
{
	r->left=-(float)LeftOffset;
	r->top=-(float)TopOffset;
	r->width=(float)RenderWidth;
	r->height=(float)RenderHeight;
}

//===========================================================================
// 
//	Finds gaps in the texture which can be skipped by the renderer
//  This was mainly added to speed up one area in E4M6 of 007LTSD
//
//===========================================================================
bool FGLTexture::FindHoles(const unsigned char * buffer, int w, int h)
{
	const unsigned char * li;
	int y,x;
	int startdraw,lendraw;
	int gaps[5][2];
	int gapc=0;


	// already done!
	if (areacount) return false;
	if (tex->UseType==FTexture::TEX_Flat) return false;	// flats don't have transparent parts
	areacount=-1;	//whatever happens next, it shouldn't be done twice!

	// large textures are excluded for performance reasons
	if (h>256) return false;	

	startdraw=-1;
	lendraw=0;
	for(y=0;y<h;y++)
	{
		li=buffer+w*y*4+3;

		for(x=0;x<w;x++,li+=4)
		{
			if (*li!=0) break;
		}

		if (x!=w)
		{
			// non - transparent
			if (startdraw==-1) 
			{
				startdraw=y;
				// merge transparent gaps of less than 16 pixels into the last drawing block
				if (gapc && y<=gaps[gapc-1][0]+gaps[gapc-1][1]+16)
				{
					gapc--;
					startdraw=gaps[gapc][0];
					lendraw=y-startdraw;
				}
				if (gapc==4) return false;	// too many splits - this isn't worth it
			}
			lendraw++;
		}
		else if (startdraw!=-1)
		{
			if (lendraw==1) lendraw=2;
			gaps[gapc][0]=startdraw;
			gaps[gapc][1]=lendraw;
			gapc++;

			startdraw=-1;
			lendraw=0;
		}
	}
	if (startdraw!=-1)
	{
		gaps[gapc][0]=startdraw;
		gaps[gapc][1]=lendraw;
		gapc++;
	}
	if (startdraw==0 && lendraw==h) return false;	// nothing saved so don't create a split list

	GL_RECT * rcs=new GL_RECT[gapc];

	for(x=0;x<gapc;x++)
	{
		// gaps are stored as texture (u/v) coordinates
		rcs[x].width=rcs[x].left=-1.0f;
		rcs[x].top=(float)gaps[x][0]/(float)h;
		rcs[x].height=(float)gaps[x][1]/(float)h;
	}
	areas=rcs;
	areacount=gapc;

	return false;
}


//===========================================================================
// 
// smooth the edges of transparent fields in the texture
// returns false when nothing is manipulated to save the work on further
// levels

// 28/10/2003: major optimization: This function was far too pedantic.
// taking the value of one of the neighboring pixels is fully sufficient
//
//===========================================================================
#define CHKPIX(ofs) \
	(l1[(ofs)*4+3]==255 ? (( ((long*)l1)[0] = ((long*)l1)[ofs]&0xffffff), trans=true ) : false)

bool FGLTexture::SmoothEdges(unsigned char * buffer,int w, int h, bool clampsides)
{
	int x,y;
	bool trans=buffer[3]==0; // If I set this to false here the code won't detect textures 
							 // that only contain transparent pixels.
	unsigned char * l1;

	if (h<=1 || w<=1) return false;	// makes (a) no sense and (b) doesn't work with this code!

	l1=buffer;


	if (l1[3]==0 && !CHKPIX(1)) CHKPIX(w);
	l1+=4;
	for(x=1;x<w-1;x++, l1+=4)
	{
		if (l1[3]==0 &&	!CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
	}
	if (l1[3]==0 && !CHKPIX(-1)) CHKPIX(w);
	l1+=4;

	for(y=1;y<h-1;y++)
	{
		if (l1[3]==0 && !CHKPIX(-w) && !CHKPIX(1)) CHKPIX(w);
		l1+=4;
		for(x=1;x<w-1;x++, l1+=4)
		{
			if (l1[3]==0 &&	!CHKPIX(-w) && !CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
		}
		if (l1[3]==0 && !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(w);
		l1+=4;
	}

	if (l1[3]==0 && !CHKPIX(-w)) CHKPIX(1);
	l1+=4;
	for(x=1;x<w-1;x++, l1+=4)
	{
		if (l1[3]==0 &&	!CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(1);
	}
	if (l1[3]==0 && !CHKPIX(-w)) CHKPIX(-1);

	return trans;
}


//===========================================================================
// 
// Post-process the texture data after the buffer has been created
//
//===========================================================================
void FGLTexture::ProcessData(unsigned char * buffer, int w, int h, int cm, bool ispatch)
{
	if (tex->bMasked) 
	{
		tex->bMasked=SmoothEdges(buffer, w, h, ispatch);
		if (tex->bMasked && !ispatch) FindHoles(buffer, w, h);
	}
}



//===========================================================================
// 
//	Deletes all allocated resources
//
//===========================================================================

void FGLTexture::Clean(bool all)
{
	WorldTextureInfo::Clean(all);
	PatchTextureInfo::Clean(all);
	if (all)
	{
		if (HiresLump==-2)
		{
			if (hirespath) delete [] hirespath;
			hirespath=NULL;
		}
		if (HiresLump<0) HiresLump=-1;
	}
}

//===========================================================================
// 
//	Initializes the buffer for the texture data
//
//===========================================================================

unsigned char * FGLTexture::CreateTexBuffer(int _cm, int translation, const byte * translationtable, int & w, int & h)
{
	unsigned char * buffer;
	intptr_t cm = _cm;

	if (HiresLump>=0)
	{
		buffer = LoadFromLump(HiresLump, &w, &h, cm<CM_FIRSTCOLORMAP? cm : CM_DEFAULT);
		if (buffer)
		{
			return buffer;
		}
	}

	// Textures that are already scaled in the texture lump will not get replaced
	// by hires textures
	if (gl_texture_usehires && scalex==1.f && scaley==1.f)
	{
		// Hires textures will not be subject to translations or Boom colormaps!
		buffer = LoadHiresTexture (&w, &h, cm<CM_FIRSTCOLORMAP? cm : CM_DEFAULT);
		if (buffer)
		{
			return buffer;
		}
	}

	w=Width;
	h=Height;

	buffer=new unsigned char[Width*(Height+1)*4];
	memset(buffer, 0, Width * (Height+1) * 4);

	if (translationtable)
	{
		// todo: check whether it is one of the standard tables
		cm=(intptr_t)translationtable;
		translation=DIRECT_PALETTE;
	}

	// For hires textures remapping to the base palette is far too expensive so I am skipping it.
	if (cm<CM_FIRSTCOLORMAP || translation==DIRECT_PALETTE)
	{
		tex->CopyTrueColorPixels(buffer, Width, Height, LeftOffset - tex->LeftOffset, TopOffset - tex->TopOffset,
								cm, translation);
	}
	else
	{
		// For Boom colormaps it is easiest to pass a buffer that has been mapped to the base palette
		// Since FTexture's method is doing exactly that by calling GetPixels let's use that here
		// to do all the dirty work for us. ;)
		tex->FTexture::CopyTrueColorPixels(buffer, Width, Height, LeftOffset - tex->LeftOffset, 
											TopOffset - tex->TopOffset,	cm, translation);
	}

	// [BB] Potentially upsample the buffer.
	buffer = gl_CreateUpsampledTextureBuffer ( this, buffer, Width, Height, w, h );

	return buffer;
}

//===========================================================================
// 
//	Gets texture coordinate info for world (wall/flat) textures
//  The wrapper class is there to provide a set of coordinate
//  functions to access the texture
//
//===========================================================================
const WorldTextureInfo * FGLTexture::GetWorldTextureInfo()
{

	if (tex->UseType==FTexture::TEX_Null) return NULL;		// Cannot register a NULL texture!
	if (!gltexture) gltexture=new GLTexture(Width, Height, true, true);
	if (gltexture) return (WorldTextureInfo*)this; 	
	return NULL;
}

//===========================================================================
// 
//	Gets texture coordinate info for sprites
//  The wrapper class is there to provide a set of coordinate
//  functions to access the texture
//
//===========================================================================
const PatchTextureInfo * FGLTexture::GetPatchTextureInfo()
{
	if (tex->UseType==FTexture::TEX_Null) return NULL;		// Cannot register a NULL texture!
	if (!glpatch) 
	{
		glpatch=new GLTexture(Width, Height, false, false);

		if (!(gl.flags&RFL_NPOT_TEXTURE) && CheckExternalFile()) 
		{
			int w,h;

			// Special handling required for hires sprites:
			// To get the real texture size I have to create the actual texture here!
			unsigned char * buffer = CreateTexBuffer(CM_DEFAULT, 0, NULL, w, h);
			ProcessData(buffer, w, h, CM_DEFAULT, false);
			glpatch->CreateTexture(buffer, w, h, true, CM_DEFAULT);
			delete buffer;
		}
	}
	if (glpatch) return (PatchTextureInfo*)this; 	
	return NULL;
}

//===========================================================================
// 
//	Binds a texture to the renderer
//
//===========================================================================

const WorldTextureInfo * FGLTexture::Bind(int cm)
{
	if (gl_warp_shader)
	{
		if (tex->bWarped!=0)
		{
			FGLTexture * parent = ValidateTexture(static_cast<FWarpTexture*>(tex)->SourcePic);
			const WorldTextureInfo * wti = parent->Bind(cm);

			if (gl_SetShaderForWarp(tex->bWarped, I_MSTime()/1000.f))
			{
				return wti;
			}
			// The shader failed - revert to software warping.
			gl_warp_shader=false;
		}
		else gl_SetShaderForWarp(0,0);
	}

	if (GetWorldTextureInfo())
	{
		if (cm >= CM_FIRSTCOLORMAP)
		{
			gl_SetColorMode(CM_DEFAULT);
		}
		else
		{
			if (gl_SetColorMode(cm, tex->bHasCanvas)) cm=CM_DEFAULT;
		}

		// If this is a warped texture that needs updating
		// delete all system textures created for this but only
		// if it is not a hires replacement.
		if (tex->CheckModified() && !tex->bHasCanvas && HiresLump<0 && HiresLump!=-2)
		{
			gltexture->Clean(true);
		}

		// Bind it to the system. For multitexturing this
		// should be the only thing that needs adjusting
		if (!gltexture->Bind(cm))
		{
			int w,h;

			// Create this texture
			unsigned char * buffer = CreateTexBuffer(cm, 0, NULL, w, h);
			ProcessData(buffer, w, h, cm, false);
			if (!gltexture->CreateTexture(buffer, w, h, true, cm)) 
			{
				// could not create texture
				delete buffer;
				return NULL;
			}
			delete buffer;
		}
		if (tex->bHasCanvas) static_cast<FCanvasTexture*>(tex)->NeedUpdate();
		return (WorldTextureInfo*)this; 
	}
	return NULL;
}

//===========================================================================
// 
//	Binds a sprite to the renderer
//
//===========================================================================
const PatchTextureInfo * FGLTexture::BindPatch(int cm, int translation, const byte * translationtable)
{
	if (tex->bHasCanvas) return NULL;	// no canvas textures outside of a level!

	if (gl_warp_shader)
	{
		if (tex->bWarped!=0)
		{
			FGLTexture * parent = ValidateTexture(static_cast<FWarpTexture*>(tex)->SourcePic);
			const PatchTextureInfo * wti = parent->BindPatch(cm, translation, translationtable);

			if (gl_SetShaderForWarp(tex->bWarped, I_MSTime()/1000.f))
			{
				return wti;
			}
			// The shader failed - revert to software warping.
			gl_warp_shader=false;
		}
		else gl_SetShaderForWarp(0,0);
	}


	if (GetPatchTextureInfo())
	{
		// CM_LITE is a special case and must use remapped textures in any case!
		// It is covered by this 'if'!
		if (cm >= CM_SHADE)
		{
			gl_SetColorMode(CM_DEFAULT);
			if (cm==CM_LITE) cm=CM_INVERT;
		}
		else
		{
			if (gl_SetColorMode(cm, tex->bHasCanvas)) cm=CM_DEFAULT;
		}

		// If this is a warped texture that needs updating
		// delete all system textures created for this
		if (tex->CheckModified() && !tex->bHasCanvas && HiresLump<0 && HiresLump!=-2)
		{
			glpatch->Clean(true);
		}

		// Bind it to the system. For multitexturing this
		// should be the only thing that needs adjusting
		if (!glpatch->Bind(cm, translation))
		{
			int w, h;
			// Create this texture
			unsigned char * buffer = CreateTexBuffer(cm, translation, translationtable, w, h);
			ProcessData(buffer, w, h, cm, true);
			if (!glpatch->CreateTexture(buffer, w, h, false, cm, translation, translationtable)) 
			{
				// could not create texture
				delete buffer;
				return NULL;
			}
			delete buffer;
		}
		return (PatchTextureInfo*)this; 	
	}
	return NULL;
}



//==========================================================================
//
// Flushes all hardware dependent data
//
//==========================================================================

void FGLTexture::FlushAll()
{
	if (gltextures)
	{
		for(int i=gltextures->Size()-1;i>=0;i--)
		{
			(*gltextures)[i]->Clean(true);
		}
	}

	// This is the only means to properly reinitialize the status bar from Doom.
	if (StatusBar && screen) StatusBar->NewGame();
}

//==========================================================================
//
// Gets a texture from the texture manager and checks its validity for
// GL rendering. 
//
//==========================================================================

FGLTexture * FGLTexture::ValidateTexture(FTexture * tex)
{
	if (tex	&& tex->UseType!=FTexture::TEX_Null)
	{
		if (!tex->gltex) tex->gltex = new FGLTexture(tex);
		return tex->gltex;
	}
	return NULL;
}

FGLTexture * FGLTexture::ValidateTexture(int no, bool translate)
{
	return FGLTexture::ValidateTexture(translate? TexMan(no) : TexMan[no]);
}

