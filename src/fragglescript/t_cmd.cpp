/*
** t_cmd.cpp
** Emulation for selected Legacy console commands
** Unfortunately Legacy allows full access of FS to the console
** so everything that gets used by some map has to be emulated...
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





#include <string.h>
#include <stdio.h>
#include "p_local.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_dispatch.h"
#include "sc_man.h"
#include "gl/gl_functions.h"

static void FS_Gimme(const char * what)
{
	char buffer[80];

	// This is intentionally limited to the few items
	// it can handle in Legacy. 
	if (!strnicmp(what, "health", 6)) what="health";
	else if (!strnicmp(what, "ammo", 4)) what="ammo";
	else if (!strnicmp(what, "armor", 5)) what="greenarmor";
	else if (!strnicmp(what, "keys", 4)) what="keys";
	else if (!strnicmp(what, "weapons", 7)) what="weapons";
	else if (!strnicmp(what, "chainsaw", 8)) what="chainsaw";
	else if (!strnicmp(what, "shotgun", 7)) what="shotgun";
	else if (!strnicmp(what, "supershotgun", 12)) what="supershotgun";
	else if (!strnicmp(what, "rocket", 6)) what="rocketlauncher";
	else if (!strnicmp(what, "plasma", 6)) what="plasmarifle";
	else if (!strnicmp(what, "bfg", 3)) what="BFG9000";
	else if (!strnicmp(what, "chaingun", 8)) what="chaingun";
	else if (!strnicmp(what, "berserk", 7)) what="Berserk";
	else if (!strnicmp(what, "map", 3)) what="Allmap";
	else if (!strnicmp(what, "fullmap", 7)) what="Allmap";
	else return;

	sprintf(buffer, "give %.72s", what);
	AddCommandString(buffer);
}


void FS_MapCmd()
{
	bool resetplayers=true;
	SC_MustGetString();
	strncpy (level.nextmap, sc_String, 8);

	while (SC_GetString())
	{
		if (SC_Compare("-skill"))
		{
			SC_MustGetNumber();
			NextSkill = sc_Number-1+sk_baby;
		}
		else if (SC_Compare("-monsters"))
		{
			SC_MustGetNumber();
			if (sc_Number) dmflags = dmflags & ~DF_NO_MONSTERS;
			else dmflags = dmflags | DF_NO_MONSTERS;
		}
		else if (SC_Compare("-noresetplayers"))
		{
			resetplayers=false;
		}
	}
	if (resetplayers)
	{
		for(int i=0;i<MAXPLAYERS;i++)
		{
			if (playeringame[i])
			{
				APlayerPawn * actor = players[i].mo;

				AInventory *inv = actor->Inventory;

				while (inv != NULL)
				{
					AInventory *next = inv->Inventory;
					if (!(inv->ItemFlags & IF_UNDROPPABLE))
					{
						inv->Destroy ();
					}
					else if (inv->GetClass() == RUNTIME_CLASS(AHexenArmor))
					{
						AHexenArmor *harmor = static_cast<AHexenArmor *> (inv);
						harmor->Slots[3] = harmor->Slots[2] = harmor->Slots[1] = harmor->Slots[0] = 0;
					}
					inv = next;
				}
				if (actor->player != NULL)
				{
					actor->player->ReadyWeapon = NULL;
					actor->player->PendingWeapon = WP_NOCHANGE;
					actor->player->psprites[ps_weapon].state = NULL;
					actor->player->psprites[ps_flash].state = NULL;
				}
				actor->GiveDefaultInventory();
			}
		}
	}
	level.flags|=LEVEL_NOINTERMISSION;
	G_ExitLevel(0, false);
}


void FS_EmulateCmd(char * string)
{
	SC_OpenMem("RUNCMD", string, (int)strlen(string));
	while (SC_GetString())
	{
		if (SC_Compare("GIMME"))
		{
			while (SC_GetString())
			{
				if (!SC_Compare(";")) FS_Gimme(sc_String);
				else break;
			}
		}
		else if (SC_Compare("ALLOWJUMP"))
		{
			SC_MustGetNumber();
			if (sc_Number) dmflags = dmflags & ~DF_NO_JUMP;
			else dmflags=dmflags | DF_NO_JUMP;
			while (SC_GetString())
			{
				if (SC_Compare(";")) break;
			}
		}
		else if (SC_Compare("gravity"))
		{
			SC_MustGetFloat();
			level.gravity=sc_Float*800;
			while (SC_GetString())
			{
				if (SC_Compare(";")) break;
			}
		}
		else if (SC_Compare("viewheight"))
		{
			SC_MustGetFloat();
			fixed_t playerviewheight = (fixed_t)(sc_Float*FRACUNIT);
			for(int i=0;i<MAXPLAYERS;i++)
			{
				// No, this is not correct. But this is the way Legacy WADs expect it to be handled!
				if (players[i].mo != NULL) players[i].mo->ViewHeight = playerviewheight;
				players[i].Uncrouch();
			}
			while (SC_GetString())
			{
				if (SC_Compare(";")) break;
			}
		}
		else if (SC_Compare("map"))
		{
			FS_MapCmd();
		}
		else if (SC_Compare("gr_fogdensity"))
		{
			SC_MustGetNumber();
			// Using this disables most MAPINFO fog options!
			gl_SetFogParams(sc_Number*70/400, 0xff000000, 0, 0);
		}
		else if (SC_Compare("gr_fogcolor"))
		{
			SC_MustGetString();
			level.fadeto = strtol(sc_String, NULL, 16);
		}

		else
		{
			// Skip unhandled commands
			while (SC_GetString())
			{
				if (SC_Compare(";")) break;
			}
		}
	}
}
