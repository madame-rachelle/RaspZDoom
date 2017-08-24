// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2005 Christoph Oelckers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/*
FraggleScript is from SMMU which is under the GPL. Technically, therefore, 
combining the FraggleScript code with the non-free ZDoom code is a violation of the GPL.

As this may be a problem for you, I hereby grant an exception to my copyright on the 
SMMU source (including FraggleScript). You may use any code from SMMU in GZDoom, provided that:

    * For any binary release of the port, the source code is also made available.
    * The copyright notice is kept on any file containing my code.
*/

#include "templates.h"
#include "p_local.h"
#include "t_script.h"
#include "s_sound.h"
#include "p_lnspec.h"
#include "m_random.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "a_doomglobal.h"
#include "w_wad.h"
#include "gi.h"
#include "zstring.h"

#include "gl/gl_data.h"

static FRandom pr_script("FScript");

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);

TArray<void *> levelpointers;

#define FIXED_TO_FLOAT(f) ((f)/(float)FRACUNIT)

#define CenterSpot(sec) (vertex_t*)&(sec)->soundorg[0]

// Disables Legacy-incompatible bug fixes.
CVAR(Bool, fs_forcecompatible, false, CVAR_ARCHIVE|CVAR_SERVERINFO)

// functions. SF_ means Script Function not, well.. heh, me

/////////// actually running a function /////////////

//==========================================================================
//
// The Doom actors in their original order
//
//==========================================================================

static const char * const ActorNames_init[]=
{
	"DoomPlayer",
	"ZombieMan",
	"ShotgunGuy",
	"Archvile",
	"ArchvileFire",
	"Revenant",
	"RevenantTracer",
	"RevenantTracerSmoke",
	"Fatso",
	"FatShot",
	"ChaingunGuy",
	"DoomImp",
	"Demon",
	"Spectre",
	"Cacodemon",
	"BaronOfHell",
	"BaronBall",
	"HellKnight",
	"LostSoul",
	"SpiderMastermind",
	"Arachnotron",
	"Cyberdemon",
	"PainElemental",
	"WolfensteinSS",
	"CommanderKeen",
	"BossBrain",
	"BossEye",
	"BossTarget",
	"SpawnShot",
	"SpawnFire",
	"ExplosiveBarrel",
	"DoomImpBall",
	"CacodemonBall",
	"Rocket",
	"PlasmaBall",
	"BFGBall",
	"ArachnotronPlasma",
	"BulletPuff",
	"Blood",
	"TeleportFog",
	"ItemFog",
	"TeleportDest",
	"BFGExtra",
	"GreenArmor",
	"BlueArmor",
	"HealthBonus",
	"ArmorBonus",
	"BlueCard",
	"RedCard",
	"YellowCard",
	"YellowSkull",
	"RedSkull",
	"BlueSkull",
	"Stimpack",
	"Medikit",
	"Soulsphere",
	"InvulnerabilitySphere",
	"Berserk",
	"BlurSphere",
	"RadSuit",
	"Allmap",
	"Infrared",
	"Megasphere",
	"Clip",
	"ClipBox",
	"RocketAmmo",
	"RocketBox",
	"Cell",
	"CellBox",
	"Shell",
	"ShellBox",
	"Backpack",
	"BFG9000",
	"Chaingun",
	"Chainsaw",
	"RocketLauncher",
	"PlasmaRifle",
	"Shotgun",
	"SuperShotgun",
	"TechLamp",
	"TechLamp2",
	"Column",
	"TallGreenColumn",
	"ShortGreenColumn",
	"TallRedColumn",
	"ShortRedColumn",
	"SkullColumn",
	"HeartColumn",
	"EvilEye",
	"FloatingSkull",
	"TorchTree",
	"BlueTorch",
	"GreenTorch",
	"RedTorch",
	"ShortBlueTorch",
	"ShortGreenTorch",
	"ShortRedTorch",
	"Slalagtite",
	"TechPillar",
	"CandleStick",
	"Candelabra",
	"BloodyTwitch",
	"Meat2",
	"Meat3",
	"Meat4",
	"Meat5",
	"NonsolidMeat2",
	"NonsolidMeat4",
	"NonsolidMeat3",
	"NonsolidMeat5",
	"NonsolidTwitch",
	"DeadCacodemon",
	"DeadMarine",
	"DeadZombieMan",
	"DeadDemon",
	"DeadLostSoul",
	"DeadDoomImp",
	"DeadShotgunGuy",
	"GibbedMarine",
	"GibbedMarineExtra",
	"HeadsOnAStick",
	"Gibs",
	"HeadOnAStick",
	"HeadCandles",
	"DeadStick",
	"LiveStick",
	"BigTree",
	"BurningBarrel",
	"HangNoGuts",
	"HangBNoBrain",
	"HangTLookingDown",
	"HangTSkull",
	"HangTLookingUp",
	"HangTNoBrain",
	"ColonGibs",
	"SmallBloodPool",
	"BrainStem",
	"PointPusher",
	"PointPuller",
};

static const PClass * ActorTypes[countof(ActorNames_init)];

//==========================================================================
//
// Some functions that take care of the major differences between the class
// based actor system from ZDoom and Doom's index based one
//
//==========================================================================

//==========================================================================
//
// Gets an actor class
// Input can be either a class name, an actor variable or a Doom index
// Doom index is only supported for the original things up to MBF
//
//==========================================================================
const PClass * T_GetMobjType(svalue_t arg)
{
	const PClass * PClass=NULL;
	
	if (arg.type==svt_string)
	{
		PClass=PClass::FindClass(arg.value.s);

		// invalid object to spawn
		if(!PClass) script_error("unknown object type: %s\n", arg.value.s); 
	}
	else if (arg.type==svt_mobj)
	{
		AActor * mo = MobjForSvalue(arg);
		if (mo) PClass = mo->GetClass();
	}
	else
	{
		int objtype = intvalue(arg);
		if (objtype>=0 && objtype<countof(ActorTypes)) PClass=ActorTypes[objtype];
		else PClass=NULL;

		// invalid object to spawn
		if(!PClass) script_error("unknown object type: %i\n", objtype); 
	}
	return PClass;
}

//==========================================================================
//
// Gets a player index
// Input can be either an actor variable or an index value
//
//==========================================================================
int T_GetPlayerNum(svalue_t arg)
{
	int playernum;
	if(arg.type == svt_mobj)
	{
		if(!MobjForSvalue(arg) || !arg.value.mobj->player)
		{
			// I prefer this not to make an error.
			// This way a player function used for a non-player
			// object will just do nothing
			//script_error("mobj not a player!\n");
			return -1;
		}
		playernum = arg.value.mobj->player - players;
	}
	else
		playernum = intvalue(arg);
	
	if(playernum < 0 || playernum > MAXPLAYERS)
	{
		return -1;
	}
	if(!playeringame[playernum]) // no error, just return -1
	{
		t_return.type = svt_int;
		t_return.value.i = -1;
		return -1;
	}
	return playernum;
}

//==========================================================================
//
// Finds a sector from a tag. This has been extended to allow looking for
// sectors directly by passing a negative value
//
//==========================================================================
int T_FindSectorFromTag(int tagnum,int startsector)
{
	if (tagnum<=0)
	{
		if (startsector<0)
		{
			if (tagnum==-32768) return 0;
			if (-tagnum<numsectors) return -tagnum;
		}
		return -1;
	}
	return P_FindSectorFromTag(tagnum,startsector);
}


//==========================================================================
//
// Get an ammo type
// Input can be either a class name or a Doom index
// Doom index is only supported for the 4 original ammo types
//
//==========================================================================
static const PClass * T_GetAmmo(svalue_t t)
{
	const char * p;

	if (t.type==svt_string)
	{
		p=stringvalue(t);
	}
	else	
	{
		// backwards compatibility with Legacy.
		// allow only Doom's standard types here!
		static const char * DefAmmo[]={"Clip","Shell","Cell","RocketAmmo"};
		int ammonum = intvalue(t);
		if(ammonum < 0 || ammonum >= 4)	
		{
			script_error("ammo number out of range: %i", ammonum);
			return NULL;
		}
		p=DefAmmo[ammonum];
	}
	const PClass * am=PClass::FindClass(p);
	if (!am->IsDescendantOf(RUNTIME_CLASS(AAmmo)))
	{
		script_error("unknown ammo type : %s", p);
		return NULL;
	}
	return am;

}

//==========================================================================
//
// Finds a sound in the sound table and adds a new entry if it isn't defined
// It's too bad that this is necessary but FS doesn't know about this kind
// of sound management.
//
//==========================================================================
static int T_FindSound(const char * name)
{
	char buffer[40];
	int so=S_FindSound(name);

	if (so>0) return so;

	// Now it gets dirty!

	if (gameinfo.gametype & GAME_DoomStrife)
	{
		sprintf(buffer, "DS%.35s", name);
		if (Wads.CheckNumForName(buffer, ns_sounds)<0) strcpy(buffer, name);
	}
	else
	{
		strcpy(buffer, name);
		if (Wads.CheckNumForName(buffer, ns_sounds)<0) sprintf(buffer, "DS%.35s", name);
	}
	
	so=S_AddSound(name, buffer);
	S_HashSounds();
	return so;
}


//==========================================================================
//
// Creates a string out of a print argument list. This version does not
// have any length restrictions like the original FS versions had.
//
//==========================================================================
static FString T_GetFormatString(int startarg)
{
	FString fmt="";
	for(int i=startarg; i<t_argc; i++) fmt += stringvalue(t_argv[i]);
	return fmt;
}

static bool T_CheckArgs(int cnt)
{
	if (t_argc<cnt)
	{
		script_error("Insufficient parameters for '%s'\n", t_func);
		return false;
	}
	return true;
}
//==========================================================================

//FUNCTIONS

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'
//
//==========================================================================

//==========================================================================
//
// prints some text to the console and the notify buffer
//
//==========================================================================
void SF_Print(void)
{
	Printf(PRINT_HIGH, "%s\n", T_GetFormatString(0).GetChars());
}


//==========================================================================
//
// return a random number from 0 to 255
//
//==========================================================================
void SF_Rnd(void)
{
	t_return.type = svt_int;
	t_return.value.i = pr_script();
}

//==========================================================================
//
// looping section. using the rover, find the highest level
// loop we are currently in and return the section_t for it.
//
//==========================================================================

section_t *looping_section(void)
{
	section_t *best = NULL;         // highest level loop we're in
	// that has been found so far
	int n;
	
	// check thru all the hashchains
	
	for(n=0; n<SECTIONSLOTS; n++)
    {
		section_t *current = current_script->sections[n];
		
		// check all the sections in this hashchain
		while(current)
		{
			// a loop?
			
			if(current->type == st_loop)
				// check to see if it's a loop that we're inside
				if(rover >= current->start && rover <= current->end)
				{
					// a higher nesting level than the best one so far?
					if(!best || (current->start > best->start))
						best = current;     // save it
				}
				current = current->next;
		}
    }
	
	return best;    // return the best one found
}

//==========================================================================
//
// "continue;" in FraggleScript is a function
//
//==========================================================================

void SF_Continue(void)
{
	section_t *section;
	
	if(!(section = looping_section()) )       // no loop found
    {
		script_error("continue() not in loop\n");
		return;
    }
	
	rover = section->end;      // jump to the closing brace
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Break(void)
{
	section_t *section;
	
	if(!(section = looping_section()) )
    {
		script_error("break() not in loop\n");
		return;
    }
	
	rover = section->end+1;   // jump out of the loop
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Goto(void)
{
	if (T_CheckArgs(1))
	{
		// check argument is a labelptr
		
		if(t_argv[0].type != svt_label)
		{
			script_error("goto argument not a label\n");
			return;
		}
		
		// go there then if everythings fine
		rover = t_argv[0].value.labelptr;
	}	
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Return(void)
{
	killscript = true;      // kill the script
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Include(void)
{
	char tempstr[12];
	
	if (T_CheckArgs(1))
	{
		if(t_argv[0].type == svt_string)
			strncpy(tempstr, t_argv[0].value.s, 8);
		else
			sprintf(tempstr, "%i", (int)t_argv[0].value.i);
		
		parse_include(tempstr);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Input(void)
{
	Printf(PRINT_BOLD,"input() function not available in doom\n");
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Beep(void)
{
	S_Sound(CHAN_AUTO, "misc/chat", 1.0f, ATTN_IDLE);
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Clock(void)
{
	t_return.type = svt_int;
	t_return.value.i = (gametic*100)/TICRATE;
}

/**************** doom stuff ****************/

//==========================================================================
//
//
//
//==========================================================================

void SF_ExitLevel(void)
{
	G_ExitLevel(0, false);
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Tip(void)
{
	if (t_argc>0 && current_script->trigger &&
		current_script->trigger->CheckLocalView(consoleplayer)) 
	{
		C_MidPrint(T_GetFormatString(0).GetChars());
	}
}

//==========================================================================
//
// SF_TimedTip
//
// Implements: void timedtip(int clocks, ...)
//
//==========================================================================

EXTERN_CVAR(Float, con_midtime)

void SF_TimedTip(void)
{
	if (T_CheckArgs(2))
	{
		float saved = con_midtime;
		con_midtime = intvalue(t_argv[0])/100.0f;
		C_MidPrint(T_GetFormatString(1).GetChars());
		con_midtime=saved;
	}
}


//==========================================================================
//
// tip to a particular player
//
//==========================================================================

void SF_PlayerTip(void)
{
	if (T_CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);
		if (plnum!=-1 && players[plnum].mo->CheckLocalView(consoleplayer)) 
		{
			C_MidPrint(T_GetFormatString(1).GetChars());
		}
	}
}

//==========================================================================
//
// message player
//
//==========================================================================

void SF_Message(void)
{
	if (t_argc>0 && current_script->trigger &&
		current_script->trigger->CheckLocalView(consoleplayer))
	{
		Printf(PRINT_HIGH, "%s\n", T_GetFormatString(0).GetChars());
	}
}

//==========================================================================
//
// message to a particular player
//
//==========================================================================

void SF_PlayerMsg(void)
{
	if (T_CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);
		if (plnum!=-1 && players[plnum].mo->CheckLocalView(consoleplayer)) 
		{
			Printf(PRINT_HIGH, "%s\n", T_GetFormatString(1).GetChars());
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerInGame(void)
{
	if (T_CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);

		if (plnum!=-1)
		{
			t_return.type = svt_int;
			t_return.value.i = playeringame[plnum];
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerName(void)
{
	int plnum;
	
	if(!t_argc)
    {
		player_t *pl=NULL;
		if (current_script->trigger) pl = current_script->trigger->player;
		if(pl) plnum = pl - players;
		else plnum=-1;
    }
	else
		plnum = T_GetPlayerNum(t_argv[0]);
	
	if(plnum !=-1)
	{
		t_return.type = svt_string;
		t_return.value.s = players[plnum].userinfo.netname;
	}
	else
	{
		script_error("script not started by player\n");
	}
}

//==========================================================================
//
// object being controlled by player
//
//==========================================================================

void SF_PlayerObj(void)
{
	int plnum;

	if(!t_argc)
	{
		player_t *pl=NULL;
		if (current_script->trigger) pl = current_script->trigger->player;
		if(pl) plnum = pl - players;
		else plnum=-1;
	}
	else
		plnum = T_GetPlayerNum(t_argv[0]);

	if(plnum !=-1)
	{
		t_return.type = svt_mobj;
		t_return.value.mobj = players[plnum].mo;
	}
	else
	{
		script_error("script not started by player\n");
	}
}

extern void SF_StartScript();      // in t_script.c
extern void SF_ScriptRunning();
extern void SF_Wait();
extern void SF_TagWait();
extern void SF_ScriptWait();
extern void SF_ScriptWaitPre();    // haleyjd: new wait types

/*********** Mobj code ***************/

//==========================================================================
//
//
//
//==========================================================================

void SF_Player(void)
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	
	if(mo && mo->player) // haleyjd: added mo->player
	{
		t_return.value.i = (int)(mo->player - players);
	}
	else
	{
		t_return.value.i = -1;
	}
}

//==========================================================================
//
// SF_Spawn
// 
// Implements: mobj spawn(int type, int x, int y, [int angle], [int z], [bool zrel])
//
//==========================================================================

void SF_Spawn(void)
{
	int x, y, z;
	const PClass *PClass;
	angle_t angle = 0;
	
	if (T_CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		x = fixedvalue(t_argv[1]);
		y = fixedvalue(t_argv[2]);

		if(t_argc >= 5)
		{
			z = fixedvalue(t_argv[4]);
			// [Graf Zahl] added option of spawning with a relative z coordinate
			if(t_argc > 5)
			{
				if (intvalue(t_argv[5])) z+=R_PointInSubsector(x, y)->sector->floorplane.ZatPoint(x,y);
			}
		}
		else
		{
			// Legacy compatibility is more important than correctness.
			z = ONFLOORZ;// (GetDefaultByType(PClass)->flags & MF_SPAWNCEILING) ? ONCEILINGZ : ONFLOORZ;
		}
		
		if(t_argc >= 4)
		{
			angle = intvalue(t_argv[3]) * (SQWORD)ANG45 / 45;
		}
		
		t_return.type = svt_mobj;
		t_return.value.mobj = Spawn(PClass, x, y, z, ALLOW_REPLACE);

		if (t_return.value.mobj)		
		{
			t_return.value.mobj->angle = angle;

			if (!fs_forcecompatible)
			{
				if (!P_TestMobjLocation(t_return.value.mobj))
				{
					if (t_return.value.mobj->flags&MF_COUNTKILL) level.total_monsters--;
					if (t_return.value.mobj->flags&MF_COUNTITEM) level.total_items--;
					t_return.value.mobj->Destroy();
					t_return.value.mobj = NULL;
				}
			}
		}
	}	
}

//==========================================================================
//
//
//
//==========================================================================

void SF_RemoveObj(void)
{
	if (T_CheckArgs(1))
	{
		AActor * mo = MobjForSvalue(t_argv[0]);
		if(mo)  // nullptr check
		{
			if (mo->flags&MF_COUNTKILL && mo->health>0) level.total_monsters--;
			if (mo->flags&MF_COUNTITEM) level.total_items--;
			mo->Destroy();
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_KillObj(void)
{
	AActor *mo;
	
	if(t_argc) mo = MobjForSvalue(t_argv[0]);
	else mo = current_script->trigger;  // default to trigger object
	
	if(mo) 
	{
		// ensure the thing can be killed
		mo->flags|=MF_SHOOTABLE;
		mo->flags2&=~(MF2_INVULNERABLE|MF2_DORMANT);
		// [GrafZahl] This called P_KillMobj directly 
		// which is a very bad thing to do!
		P_DamageMobj(mo, NULL, NULL, mo->health, MOD_MASSACRE);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_ObjX(void)
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;           // haleyjd: SoM's fixed-point fix
	t_return.value.f = mo ? mo->x : 0;   // null ptr check
}

//==========================================================================
//
//
//
//==========================================================================

void SF_ObjY(void)
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;         // haleyjd
	t_return.value.f = mo ? mo->y : 0; // null ptr check
}

//==========================================================================
//
//
//
//==========================================================================

void SF_ObjZ(void)
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;         // haleyjd
	t_return.value.f = mo ? mo->z : 0; // null ptr check
}


//==========================================================================
//
//
//
//==========================================================================

void SF_ObjAngle(void)
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed; // haleyjd: fixed-point -- SoM again :)
	t_return.value.f = mo ? (fixed_t)AngleToFixed(mo->angle) : 0;   // null ptr check
}


//==========================================================================
//
//
//
//==========================================================================

// teleport: object, sector_tag
void SF_Teleport(void)
{
	int tag;
	AActor *mo;
	
	if (T_CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: sector tag
		{
			mo = current_script->trigger;   // default to trigger
			tag = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // teleport a given object
			mo = MobjForSvalue(t_argv[0]);
			tag = intvalue(t_argv[1]);
		}
		
		if(mo)
			EV_Teleport(0, tag, NULL, 0, mo, true, true, false);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SilentTeleport(void)
{
	int tag;
	AActor *mo;
	
	if (T_CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: sector tag
		{
			mo = current_script->trigger;   // default to trigger
			tag = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // teleport a given object
			mo = MobjForSvalue(t_argv[0]);
			tag = intvalue(t_argv[1]);
		}
		
		if(mo)
			EV_Teleport(0, tag, NULL, 0, mo, false, false, true);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_DamageObj(void)
{
	AActor *mo;
	int damageamount;
	
	if (T_CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: damage trigger by amount
		{
			mo = current_script->trigger;   // default to trigger
			damageamount = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // damage a given object
			mo = MobjForSvalue(t_argv[0]);
			damageamount = intvalue(t_argv[1]);
		}
		
		if(mo)
			P_DamageMobj(mo, NULL, current_script->trigger, damageamount, MOD_UNKNOWN);
	}
}

//==========================================================================
//
//
//
//==========================================================================

// the tag number of the sector the thing is in
void SF_ObjSector(void)
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	t_return.value.i = mo ? mo->Sector->tag : 0; // nullptr check
}

//==========================================================================
//
//
//
//==========================================================================

// the health number of an object
void SF_ObjHealth(void)
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	t_return.value.i = mo ? mo->health : 0;
}

//==========================================================================
//
//
//
//==========================================================================

void SF_ObjFlag(void)
{
	AActor *mo;
	int flagnum;
	
	if (T_CheckArgs(1))
	{
		if(t_argc == 1)         // use trigger, 1st is flag
		{
			// use trigger:
			mo = current_script->trigger;
			flagnum = intvalue(t_argv[0]);
		}
		else if(t_argc == 2)	// specified object
		{
			mo = MobjForSvalue(t_argv[0]);
			flagnum = intvalue(t_argv[1]);
		}
		else                     // >= 3 : SET flags
		{
			mo = MobjForSvalue(t_argv[0]);
			flagnum = intvalue(t_argv[1]);
			
			if(mo && flagnum<26)          // nullptr check
			{
				// remove old bit
				mo->flags &= ~(1 << flagnum);
				
				// make the new flag
				mo->flags |= (!!intvalue(t_argv[2])) << flagnum;
			}     
		}
		t_return.type = svt_int;  
		if (mo && flagnum<26)
		{
			t_return.value.i = !!(mo->flags & (1 << flagnum));
		}
		else t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

// apply momentum to a thing
void SF_PushThing(void)
{
	if (T_CheckArgs(3))
	{
		AActor * mo = MobjForSvalue(t_argv[0]);
		if(!mo) return;
	
		angle_t angle = (angle_t)FixedToAngle(fixedvalue(t_argv[1]));
		fixed_t force = fixedvalue(t_argv[2]);
	
		P_ThrustMobj(mo, angle, force);
	}
}

//==========================================================================
//
//  SF_ReactionTime -- useful for freezing things
//
//==========================================================================


void SF_ReactionTime(void)
{
	if (T_CheckArgs(1))
	{
		AActor *mo = MobjForSvalue(t_argv[0]);
	
		if(t_argc > 1)
		{
			if(mo) mo->reactiontime = (intvalue(t_argv[1]) * TICRATE) / 100;
		}
	
		t_return.type = svt_int;
		t_return.value.i = mo ? mo->reactiontime : 0;
	}
}

//==========================================================================
//
//  SF_MobjTarget   -- sets a thing's target field
//
//==========================================================================

// Sets a mobj's Target! >:)
void SF_MobjTarget(void)
{
	AActor*  mo;
	AActor*  target;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			target = MobjForSvalue(t_argv[1]);
			if(mo && target && mo->SeeState) // haleyjd: added target check -- no NULL allowed
			{
				mo->target=target;
				mo->SetState(mo->SeeState);
				mo->flags|=MF_JUSTHIT;
			}
		}
		
		t_return.type = svt_mobj;
		t_return.value.mobj = mo ? mo->target : NULL;
	}
}

//==========================================================================
//
//  SF_MobjMomx, MobjMomy, MobjMomz -- momentum functions
//
//==========================================================================

void SF_MobjMomx(void)
{
	AActor*   mo;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->momx = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momx : 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_MobjMomy(void)
{
	AActor*   mo;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo)
				mo->momy = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momy : 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_MobjMomz(void)
{
	AActor*   mo;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo)
				mo->momz = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momz : 0;
	}
}


//==========================================================================
//
//
//
//==========================================================================

/****************** Trig *********************/

void SF_PointToAngle(void)
{
	if (T_CheckArgs(4))
	{
		fixed_t x1 = fixedvalue(t_argv[0]);
		fixed_t y1 = fixedvalue(t_argv[1]);
		fixed_t x2 = fixedvalue(t_argv[2]);
		fixed_t y2 = fixedvalue(t_argv[3]);
		
		angle_t angle = R_PointToAngle2(x1, y1, x2, y2);
		
		t_return.type = svt_fixed;
		t_return.value.f = (fixed_t)AngleToFixed(angle);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_PointToDist(void)
{
	if (T_CheckArgs(4))
	{
		// Doing this in floating point is actually faster with modern computers!
		float x = floatvalue(t_argv[2]) - floatvalue(t_argv[0]);
		float y = floatvalue(t_argv[3]) - floatvalue(t_argv[1]);
	    
		t_return.type = svt_fixed;
		t_return.value.f = (fixed_t)(sqrtf(x*x+y*y)*65536.f);
	}
}


//==========================================================================
//
// setcamera(obj, [angle], [height], [pitch])
//
// [GrafZahl] This is a complete rewrite.
// Although both Eternity and Legacy implement this function
// they are mutually incompatible with each other and with ZDoom...
//
//==========================================================================

void SF_SetCamera(void)
{
	angle_t angle;
	player_t * player;
	AActor * newcamera;
	
	if (T_CheckArgs(1))
	{
		player=current_script->trigger->player;
		if (!player) player=&players[0];
		
		newcamera = MobjForSvalue(t_argv[0]);
		if(!newcamera)
		{
			script_error("invalid location object for camera\n");
			return;         // nullptr check
		}
		
		angle = t_argc < 2 ? newcamera->angle : (fixed_t)FixedToAngle(fixedvalue(t_argv[1]));

		newcamera->special1=newcamera->angle;
		newcamera->special2=newcamera->z;
		newcamera->z = t_argc < 3 ? (newcamera->z + (41 << FRACBITS)) : (intvalue(t_argv[2]) << FRACBITS);
		newcamera->angle = angle;
		if(t_argc < 4) newcamera->pitch = 0;
		else
		{
			fixed_t pitch = fixedvalue(t_argv[3]);
			if(pitch < -50*FRACUNIT) pitch = -50*FRACUNIT;
			if(pitch > 50*FRACUNIT)  pitch =  50*FRACUNIT;
			newcamera->pitch=(angle_t)((pitch/65536.0f)*(ANGLE_45/45.0f)*(20.0f/32.0f));
		}
		player->camera=newcamera;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_ClearCamera(void)
{
	player_t * player;
	player=current_script->trigger->player;
	if (!player) player=&players[0];

	AActor * cam=player->camera;
	if (cam)
	{
		player->camera=player->mo;
		cam->angle=cam->special1;
		cam->z=cam->special2;
	}

}



/*********** sounds ******************/

//==========================================================================
//
//
//
//==========================================================================

// start sound from thing
void SF_StartSound(void)
{
	AActor *mo;
	
	if (T_CheckArgs(2))
	{
		mo = MobjForSvalue(t_argv[0]);
		
		if (mo)
		{
			S_SoundID(mo, CHAN_BODY, T_FindSound(stringvalue(t_argv[1])), 1, ATTN_NORM);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

// start sound from sector
void SF_StartSectorSound(void)
{
	sector_t *sector;
	int tagnum;
	
	if (T_CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		
		int i=-1;
		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sector = &sectors[i];
			S_SoundID(sector->soundorg, CHAN_BODY, T_FindSound(stringvalue(t_argv[1])), 1.0f, ATTN_NORM);
		}
	}
}

/************* Sector functions ***************/

//DMover::EResult P_MoveFloor (sector_t * m_Sector, fixed_t speed, fixed_t dest, int crush, int direction, int flags=0);
//DMover::EResult P_MoveCeiling (sector_t * m_Sector, fixed_t speed, fixed_t dest, int crush, int direction, int flags=0);

class DFloorChanger : public DFloor
{
public:
	DFloorChanger(sector_t * sec)
		: DFloor(sec) {}

	bool Move(fixed_t speed, fixed_t dest, int crush, int direction)
	{
		bool res = DMover::crushed != MoveFloor(speed, dest, crush, direction);
		Destroy();
		m_Sector->floordata=NULL;
		stopinterpolation (INTERP_SectorFloor, m_Sector);
		m_Sector=NULL;
		return res;
	}
};


//==========================================================================
//
//
//
//==========================================================================

// floor height of sector
void SF_FloorHeight(void)
{
	int tagnum;
	int secnum;
	fixed_t dest;
	int returnval = 1; // haleyjd: SoM's fixes
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		if(t_argc > 1)          // > 1: set floor height
		{
			int i;
			int crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;
			
			i = -1;
			dest = fixedvalue(t_argv[1]);
			
			// set all sectors with tag
			
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				if (sectors[i].floordata) continue;	// don't move floors that are active!

				DFloorChanger * f = new DFloorChanger(&sectors[i]);
				if (!f->Move(
					abs(dest - sectors[i].CenterFloor()), 
					sectors[i].floorplane.PointToDist (CenterSpot(&sectors[i]), dest), 
					crush? 10:-1, 
					(dest > sectors[i].CenterFloor()) ? 1 : -1))
				{
					returnval = 0;
				}
			}
		}
		else
		{
			secnum = T_FindSectorFromTag(tagnum, -1);
			if(secnum < 0)
			{ 
				script_error("sector not found with tagnum %i\n", tagnum); 
				return;
			}
			returnval = sectors[secnum].CenterFloor() >> FRACBITS;
		}
		
		// return floor height
		
		t_return.type = svt_int;
		t_return.value.i = returnval;
	}
}


//=============================================================================
//
//
//=============================================================================
class DMoveFloor : public DFloor
{
public:
	DMoveFloor(sector_t * sec,int moveheight,int _m_Direction,int crush,int movespeed)
	: DFloor(sec)
	{
		m_Type = floorRaiseByValue;
		m_Crush = crush;
		m_Speed=movespeed;
		m_Direction = _m_Direction;
		m_FloorDestDist = moveheight;
		StartFloorSound();
	}
};



//==========================================================================
//
//
//
//==========================================================================

void SF_MoveFloor(void)
{
	int secnum = -1;
	sector_t *sec;
	int tagnum, platspeed = 1, destheight, crush;
	
	if (T_CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		destheight = intvalue(t_argv[1]) * FRACUNIT;
		platspeed = t_argc > 2 ? fixedvalue(t_argv[2]) : FRACUNIT;
		crush = (t_argc > 3 ? intvalue(t_argv[3]) : -1);
		
		// move all sectors with tag
		
		while ((secnum = T_FindSectorFromTag(tagnum, secnum)) >= 0)
		{
			sec = &sectors[secnum];
			// Don't start a second thinker on the same floor
			if (sec->floordata) continue;
			
			new DMoveFloor(sec,sec->floorplane.PointToDist(CenterSpot(sec),destheight),
				destheight < sec->CenterFloor() ? -1:1,crush,platspeed);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

class DCeilingChanger : public DCeiling
{
public:
	DCeilingChanger(sector_t * sec)
		: DCeiling(sec) {}

	bool Move(fixed_t speed, fixed_t dest, int crush, int direction)
	{
		bool res = DMover::crushed != MoveCeiling(speed, dest, crush, direction);
		Destroy();
		m_Sector->ceilingdata=NULL;
		stopinterpolation (INTERP_SectorCeiling, m_Sector);
		m_Sector=NULL;
		return res;
	}
};

//==========================================================================
//
//
//
//==========================================================================

// ceiling height of sector
void SF_CeilingHeight(void)
{
	fixed_t dest;
	int secnum;
	int tagnum;
	int returnval = 1;
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		if(t_argc > 1)          // > 1: set ceilheight
		{
			int i;
			int crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;
			
			i = -1;
			dest = fixedvalue(t_argv[1]);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				if (sectors[i].ceilingdata) continue;	// don't move ceilings that are active!

				DCeilingChanger * c = new DCeilingChanger(&sectors[i]);
				if (!c->Move(
					abs(dest - sectors[i].CenterCeiling()), 
					sectors[i].ceilingplane.PointToDist (CenterSpot(&sectors[i]), dest), 
					crush? 10:-1,
					(dest > sectors[i].CenterCeiling()) ? 1 : -1))
				{
					returnval = 0;
				}
			}
		}
		else
		{
			secnum = T_FindSectorFromTag(tagnum, -1);
			if(secnum < 0)
			{ 
				script_error("sector not found with tagnum %i\n", tagnum); 
				return;
			}
			returnval = sectors[secnum].CenterCeiling() >> FRACBITS;
		}
		
		// return ceiling height
		t_return.type = svt_int;
		t_return.value.i = returnval;
	}
}


//==========================================================================
//
//
//
//==========================================================================

class DMoveCeiling : public DCeiling
{
public:

	DMoveCeiling(sector_t * sec,int tag,fixed_t destheight,fixed_t speed,int silent,int crush)
		: DCeiling(sec)
	{
		m_Crush = crush;
		m_Speed2 = m_Speed = m_Speed1 = speed;
		m_Silent = silent;
		m_Type = DCeiling::ceilLowerByValue;	// doesn't really matter as long as it's no special value
		m_Tag=tag;			
		vertex_t * spot=CenterSpot(sec);
		m_TopHeight=m_BottomHeight=sec->ceilingplane.PointToDist(spot,destheight);
		m_Direction=destheight>sec->ceilingtexz? 1:-1;

		// Do not interpolate instant movement ceilings.
		fixed_t movedist = abs(sec->ceilingplane.d - m_BottomHeight);
		if (m_Speed >= movedist)
		{
			stopinterpolation (INTERP_SectorCeiling, sec);
			m_Silent=2;
		}
		PlayCeilingSound();
	}
};


//==========================================================================
//
//
//
//==========================================================================

void SF_MoveCeiling(void)
{
	int secnum = -1;
	sector_t *sec;
	int tagnum, platspeed = 1, destheight;
	int crush;
	int silent;
	
	if (T_CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		destheight = intvalue(t_argv[1]) * FRACUNIT;
		platspeed = /*FLOORSPEED **/ (t_argc > 2 ? fixedvalue(t_argv[2]) : FRACUNIT);
		crush=t_argc>3 ? intvalue(t_argv[3]):-1;
		silent=t_argc>4 ? intvalue(t_argv[4]):1;
		
		// move all sectors with tag
		while ((secnum = T_FindSectorFromTag(tagnum, secnum)) >= 0)
		{
			sec = &sectors[secnum];
			
			// Don't start a second thinker on the same floor
			if (sec->ceilingdata) continue;
			new DMoveCeiling(sec, tagnum, destheight, platspeed, silent, crush);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_LightLevel(void)
{
	sector_t *sector;
	int secnum;
	int tagnum;
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ 
			return;
		}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)          // > 1: set light level
		{
			int i = -1;
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].lightlevel = (short)intvalue(t_argv[1]);
			}
		}
		
		// return lightlevel
		t_return.type = svt_int;
		t_return.value.i = sector->lightlevel;
	}
}



//==========================================================================
//
// Simple light fade - locks lightingdata. For SF_FadeLight
//
//==========================================================================
class DLightLevel : public DLighting
{
	DECLARE_CLASS (DLightLevel, DLighting)

	unsigned char destlevel;
	unsigned char speed;

	DLightLevel() {}

public:

	DLightLevel(sector_t * s,int destlevel,int speed);
	void	Serialize (FArchive &arc);
	void		Tick ();
	void		Destroy() { Super::Destroy(); m_Sector->lightingdata=NULL; }
};



IMPLEMENT_CLASS (DLightLevel)

void DLightLevel::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << destlevel << speed;
	if (arc.IsLoading()) m_Sector->lightingdata=this;
}


//==========================================================================
// sf 13/10/99:
//
// T_LightFade()
//
// Just fade the light level in a sector to a new level
//
//==========================================================================

void DLightLevel::Tick()
{
	Super::Tick();
	if(m_Sector->lightlevel < destlevel)
	{
		// increase the lightlevel
		if(m_Sector->lightlevel + speed >= destlevel)
		{
			// stop changing light level
			m_Sector->lightlevel = destlevel;    // set to dest lightlevel
			Destroy();
		}
		else
		{
			m_Sector->lightlevel = m_Sector->lightlevel+speed;
		}
	}
	else
	{
        // decrease lightlevel
		if(m_Sector->lightlevel - speed <= destlevel)
		{
			// stop changing light level
			m_Sector->lightlevel = destlevel;    // set to dest lightlevel
			Destroy();
		}
		else
		{
			m_Sector->lightlevel = m_Sector->lightlevel-speed;
		}
	}
}

//==========================================================================
//
//==========================================================================
DLightLevel::DLightLevel(sector_t * s,int _destlevel,int _speed) : DLighting(s)
{
	destlevel=_destlevel;
	speed=_speed;
	s->lightingdata=this;
}

//==========================================================================
// sf 13/10/99:
//
// P_FadeLight()
//
// Fade all the lights in sectors with a particular tag to a new value
//
//==========================================================================
void SF_FadeLight(void)
{
	int sectag, destlevel, speed = 1;
	int i;
	
	if (T_CheckArgs(2))
	{
		sectag = intvalue(t_argv[0]);
		destlevel = intvalue(t_argv[1]);
		speed = t_argc>2 ? intvalue(t_argv[2]) : 1;
		
		for (i = -1; (i = P_FindSectorFromTag(sectag,i)) >= 0;) 
		{
			if (!sectors[i].lightingdata) new DLightLevel(&sectors[i],destlevel,speed);
		}
	}
}

void SF_FloorTexture(void)
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Flat, FTextureManager::TEXMAN_Overridable);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].floorpic=picnum;
				sectors[i].AdjustFloorClip();
			}
		}
		
		t_return.type = svt_string;
		FTexture * tex = TexMan[sector->floorpic];
		t_return.value.s = tex? tex->Name : NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SectorColormap(void)
{
	// This doesn't work properly and it never will.
	// Whatever was done here originally, it is incompatible 
	// with Boom and ZDoom and doesn't work properly in Legacy either.
	
	// Making it no-op is probably the best thing one can do in this case.
	
	/*
	int tagnum, secnum;
	sector_t *sector;
	int c=2;
	int i = -1;

	if(t_argc<2)
    { script_error("insufficient arguments to function\n"); return; }
	
	tagnum = intvalue(t_argv[0]);
	
	// argv is sector tag
	secnum = T_FindSectorFromTag(tagnum, -1);
	
	if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
	
	sector = &sectors[secnum];

	if (t_argv[1].type==svt_string)
	{
		DWORD cm = R_ColormapNumForName(t_argv[1].value.s);

		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sectors[i].midmap=cm;
			sectors[i].heightsec=&sectors[i];
		}
	}
	*/	
}


//==========================================================================
//
//
//
//==========================================================================

void SF_CeilingTexture(void)
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Flat, FTextureManager::TEXMAN_Overridable);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].ceilingpic=picnum;
			}
		}
		
		t_return.type = svt_string;
		FTexture * tex = TexMan[sector->ceilingpic];
		t_return.value.s = tex? tex->Name : NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_ChangeHubLevel(void)
{
	I_Error("FS hub system permanently disabled\n");
}

// for start map: start new game on a particular skill
void SF_StartSkill(void)
{
	I_Error("startskill is not supported by this implementation!\n");
}

//////////////////////////////////////////////////////////////////////////
//
// Doors
//

// opendoor(sectag, [delay], [speed])

void SF_OpenDoor(void)
{
	int speed, wait_time;
	int sectag;
	
	if (T_CheckArgs(1))
	{
		// got sector tag
		sectag = intvalue(t_argv[0]);
		if (sectag==0) return;	// tag 0 not allowed
		
		// door wait time
		if(t_argc > 1) wait_time = (intvalue(t_argv[1]) * TICRATE) / 100;
		else wait_time = 0;  // 0= stay open
		
		// door speed
		if(t_argc > 2) speed = intvalue(t_argv[2]);
		else speed = 1;    // 1= normal speed

		EV_DoDoor(wait_time? DDoor::doorRaise:DDoor::doorOpen,NULL,NULL,sectag,2*FRACUNIT*clamp(speed,1,127),wait_time,0,0);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_CloseDoor(void)
{
	int speed;
	int sectag;
	
	if (T_CheckArgs(1))
	{
		// got sector tag
		sectag = intvalue(t_argv[0]);
		if (sectag==0) return;	// tag 0 not allowed
		
		// door speed
		if(t_argc > 1) speed = intvalue(t_argv[1]);
		else speed = 1;    // 1= normal speed
		
		EV_DoDoor(DDoor::doorClose,NULL,NULL,sectag,2*FRACUNIT*clamp(speed,1,127),0,0,0);
	}
}

//==========================================================================
//
//
//
//==========================================================================

// run console cmd
void SF_RunCommand(void)
{
	FS_EmulateCmd(T_GetFormatString(0).LockBuffer());
}

//==========================================================================
//
//
//
//==========================================================================

// any linedef type
extern void P_TranslateLineDef (line_t *ld, maplinedef_t *mld);

void SF_LineTrigger()
{
	if (T_CheckArgs(1))
	{
		line_t line;
		maplinedef_t mld;
		mld.special=intvalue(t_argv[0]);
		mld.tag=t_argc > 1 ? intvalue(t_argv[1]) : 0;
		P_TranslateLineDef(&line, &mld);
		LineSpecials[line.special](NULL, current_script->trigger, false, 
			line.args[0],line.args[1],line.args[2],line.args[3],line.args[4]); 
	}
}

//==========================================================================
//
//
//
//==========================================================================
bool FS_ChangeMusic(const char * string)
{
	char buffer[40];

	if (Wads.CheckNumForName(string, ns_music)<0 || !S_ChangeMusic(string,true))
	{
		// Retry with O_ prepended to the music name, then with D_
		sprintf(buffer, "O_%s", string);
		if (Wads.CheckNumForName(buffer, ns_music)<0 || !S_ChangeMusic(buffer,true))
		{
			sprintf(buffer, "D_%s", string);
			if (Wads.CheckNumForName(buffer, ns_music)<0) 
			{
				S_ChangeMusic(NULL, 0);
				return false;
			}
			else S_ChangeMusic(buffer,true);
		}
	}
	return true;
}

void SF_ChangeMusic(void)
{
	if (T_CheckArgs(1))
	{
		FS_ChangeMusic(stringvalue(t_argv[0]));
	}
}


//==========================================================================
//
//
//
//==========================================================================

inline line_t * P_FindLine(int tag,int * searchPosition)
{
	*searchPosition=P_FindLineFromID(tag,*searchPosition);
	return *searchPosition>=0? &lines[*searchPosition]:NULL;
}

/*
SF_SetLineBlocking()

  Sets a line blocking or unblocking
  
	setlineblocking(tag, [1|0]);
*/
void SF_SetLineBlocking(void)
{
	line_t *line;
	int blocking;
	int searcher = -1;
	int tag;
	static unsigned short blocks[]={0,ML_BLOCKING,ML_BLOCKEVERYTHING};
	
	if (T_CheckArgs(2))
	{
		blocking=intvalue(t_argv[1]);
		if (blocking>=0 && blocking<=2) 
		{
			blocking=blocks[blocking];
			tag=intvalue(t_argv[0]);
			while((line = P_FindLine(tag, &searcher)) != NULL)
			{
				line->flags = (line->flags & ~(ML_BLOCKING|ML_BLOCKEVERYTHING)) | blocking;
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

// similar, but monster blocking

void SF_SetLineMonsterBlocking(void)
{
	line_t *line;
	int blocking;
	int searcher = -1;
	int tag;
	
	if (T_CheckArgs(2))
	{
		blocking = intvalue(t_argv[1]) ? ML_BLOCKMONSTERS : 0;
		
		tag=intvalue(t_argv[0]);
		while((line = P_FindLine(tag, &searcher)) != NULL)
		{
			line->flags = (line->flags & ~ML_BLOCKMONSTERS) | blocking;
		}
	}
}

/*
SF_SetLineTexture

  #2 in a not-so-long line of ACS-inspired functions
  This one is *much* needed, IMO
  
	Eternity: setlinetexture(tag, side, position, texture)
	Legacy:	  setlinetexture(tag, texture, side, sections)

*/


void SF_SetLineTexture(void)
{
	line_t *line;
	int tag;
	int side;
	int position;
	const char *texture;
	int texturenum;
	int searcher;
	
	if (T_CheckArgs(4))
	{
		tag = intvalue(t_argv[0]);

		// the eternity version
		if (t_argv[3].type==svt_string)
		{
			side = intvalue(t_argv[1]);   
			if(side < 0 || side > 1)
			{
				script_error("invalid side number for texture change\n");
				return;
			}
			
			position = intvalue(t_argv[2]);
			if(position < 1 || position > 3)
			{
				script_error("invalid position for texture change\n");
				return;
			}
			position=3-position;
			
			texture = stringvalue(t_argv[3]);
			texturenum = TexMan.GetTexture(texture, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable);
			
			searcher = -1;
			
			while((line = P_FindLine(tag, &searcher)) != NULL)
			{
				// bad sidedef, Hexen just SEGV'd here!
				if(line->sidenum[side]!=NO_SIDE)
				{
					side_t * sided=&sides[line->sidenum[side]];
					switch(position)
					{
					case 0:
						sided->toptexture=texturenum;
						break;
					case 1:
						sided->midtexture=texturenum;
						break;
					case 2:
						sided->bottomtexture=texturenum;
						break;
					}
				}
			}
		}
		else // and an improved legacy version
		{ 
			int i = -1; 
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable); 
			side = !!intvalue(t_argv[2]); 
			int sections = intvalue(t_argv[3]); 
			
			// set all sectors with tag 
			while ((i = P_FindLineFromID(tag, i)) >= 0) 
			{ 
				if(lines[i].sidenum[side]!=NO_SIDE)
				{ 
					side_t * sided=&sides[lines[i].sidenum[side]];

					if(sections & 1) sided->toptexture = picnum;
					if(sections & 2) sided->midtexture = picnum;
					if(sections & 4) sided->bottomtexture = picnum;
				} 
			} 
		} 
	}
}


//==========================================================================
//
//
//
//==========================================================================

// SoM: Max, Min, Abs math functions.
void SF_Max(void)
{
	fixed_t n1, n2;
	
	if (T_CheckArgs(2))
	{
		n1 = fixedvalue(t_argv[0]);
		n2 = fixedvalue(t_argv[1]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 > n2) ? n1 : n2;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_Min(void)
{
	fixed_t   n1, n2;
	
	if (T_CheckArgs(1))
	{
		n1 = fixedvalue(t_argv[0]);
		n2 = fixedvalue(t_argv[1]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 < n2) ? n1 : n2;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_Abs(void)
{
	fixed_t   n1;
	
	if (T_CheckArgs(1))
	{
		n1 = fixedvalue(t_argv[0]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 < 0) ? -n1 : n1;
	}
}

/* 
SF_Gameskill, SF_Gamemode

  Access functions are more elegant for these than variables, 
  especially for the game mode, which doesn't exist as a numeric 
  variable already.
*/

void SF_Gameskill(void)
{
	t_return.type = svt_int;
	t_return.value.i = gameskill + 1;  // +1 for the user skill value
}

//==========================================================================
//
//
//
//==========================================================================

void SF_Gamemode(void)
{
	t_return.type = svt_int;   
	if(!multiplayer)
	{
		t_return.value.i = 0; // single-player
	}
	else if(!deathmatch)
	{
		t_return.value.i = 1; // cooperative
	}
	else
		t_return.value.i = 2; // deathmatch
}

/*
SF_IsPlayerObj()

  A function suggested by SoM to help the script coder prevent
  exceptions related to calling player functions on non-player
  objects.
*/
void SF_IsPlayerObj(void)
{
	AActor *mo;
	
	if(!t_argc)
	{
		mo = current_script->trigger;
	}
	else
		mo = MobjForSvalue(t_argv[0]);
	
	t_return.type = svt_int;
	t_return.value.i = (mo && mo->player) ? 1 : 0;
}

//============================================================================
//
// Since FraggleScript is rather hard coded to the original inventory
// handling of Doom this is rather messy.
//
//============================================================================


//============================================================================
//
// DoGiveInv
//
// Gives an item to a single actor.
//
//============================================================================

static void FS_GiveInventory (AActor *actor, const char * type, int amount)
{
	if (amount <= 0)
	{
		return;
	}
	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmorPickup";
	}
	const PClass * info = PClass::FindClass (type);
	if (info == NULL || !info->IsDescendantOf (RUNTIME_CLASS(AInventory)))
	{
		Printf ("Unknown inventory item: %s\n", type);
		return;
	}

	AWeapon *savedPendingWeap = actor->player != NULL? actor->player->PendingWeapon : NULL;
	bool hadweap = actor->player != NULL ? actor->player->ReadyWeapon != NULL : true;

	AInventory *item = static_cast<AInventory *>(Spawn (info, 0,0,0, NO_REPLACE));

	// This shouldn't count for the item statistics!
	if (item->flags&MF_COUNTITEM) 
	{
		level.total_items--;
		item->flags&=~MF_COUNTITEM;
	}
	if (info->IsDescendantOf (RUNTIME_CLASS(ABasicArmorPickup)) ||
		info->IsDescendantOf (RUNTIME_CLASS(ABasicArmorBonus)))
	{
		static_cast<ABasicArmorPickup*>(item)->SaveAmount *= amount;
	}
	else
	{
		item->Amount = amount;
	}
	if (!item->TryPickup (actor))
	{
		item->Destroy ();
	}
	// If the item was a weapon, don't bring it up automatically
	// unless the player was not already using a weapon.
	if (savedPendingWeap != NULL && hadweap)
	{
		actor->player->PendingWeapon = savedPendingWeap;
	}
}

//============================================================================
//
// DoTakeInv
//
// Takes an item from a single actor.
//
//============================================================================

static void FS_TakeInventory (AActor *actor, const char * type, int amount)
{
	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmor";
	}
	if (amount <= 0)
	{
		return;
	}
	const PClass * info = PClass::FindClass (type);
	if (info == NULL)
	{
		return;
	}

	AInventory *item = actor->FindInventory (info);
	if (item != NULL)
	{
		item->Amount -= amount;
		if (item->Amount <= 0)
		{
			// If it's not ammo, destroy it. Ammo needs to stick around, even
			// when it's zero for the benefit of the weapons that use it and 
			// to maintain the maximum ammo amounts a backpack might have given.
			if (item->GetClass()->ParentClass != RUNTIME_CLASS(AAmmo))
			{
				item->Destroy ();
			}
			else
			{
				item->Amount = 0;
			}
		}
	}
}

//============================================================================
//
// CheckInventory
//
// Returns how much of a particular item an actor has.
//
//============================================================================

static int FS_CheckInventory (AActor *activator, const char *type)
{
	if (activator == NULL)
		return 0;

	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmor";
	}
	else if (strcmp (type, "Health") == 0)
	{
		return activator->health;
	}

	const PClass *info = PClass::FindClass (type);
	AInventory *item = activator->FindInventory (info);
	return item ? item->Amount : 0;
}


//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerKeys(void)
{
	// This function is just kept for backwards compatibility and intentionally limited to thr standard keys!
	// Use Give/Take/CheckInventory instead!
	static const char * const DoomKeys[]={"BlueCard", "YellowCard", "RedCard", "BlueSkull", "YellowSkull", "RedSkull"};
	int  playernum, keynum, givetake;
	const char * keyname;
	
	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;
		
		keynum = intvalue(t_argv[1]);
		if(keynum < 0 || keynum >= 6)
		{
			script_error("key number out of range: %i\n", keynum);
			return;
		}
		keyname=DoomKeys[keynum];
		
		if(t_argc == 2)
		{
			t_return.type = svt_int;
			t_return.value.i = FS_CheckInventory(players[playernum].mo, keyname);
			return;
		}
		else
		{
			givetake = intvalue(t_argv[2]);
			if(givetake) FS_GiveInventory(players[playernum].mo, keyname, 1);
			else FS_TakeInventory(players[playernum].mo, keyname, 1);
			t_return.type = svt_int;
			t_return.value.i = 0;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerAmmo(void)
{
	// This function is just kept for backwards compatibility and intentionally limited!
	// Use Give/Take/CheckInventory instead!
	int playernum, amount;
	const PClass * ammotype;
	
	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		ammotype=T_GetAmmo(t_argv[1]);
		if (!ammotype) return;

		if(t_argc >= 3)
		{
			AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
			amount = intvalue(t_argv[2]);
			if(amount < 0) amount = 0;
			if (iammo) iammo->Amount = amount;
			else players[playernum].mo->GiveAmmo(ammotype, amount);
		}

		t_return.type = svt_int;
		AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
		if (iammo) t_return.value.i = iammo->Amount;
		else t_return.value.i = 0;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_MaxPlayerAmmo()
{
	int playernum, amount;
	const PClass * ammotype;

	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		ammotype=T_GetAmmo(t_argv[1]);
		if (!ammotype) return;

		if(t_argc == 2)
		{
		}
		else if(t_argc >= 3)
		{
			AAmmo * iammo = (AAmmo*)players[playernum].mo->FindInventory(ammotype);
			amount = intvalue(t_argv[2]);
			if(amount < 0) amount = 0;
			if (!iammo) 
			{
				iammo = static_cast<AAmmo *>(Spawn (ammotype, 0, 0, 0, NO_REPLACE));
				iammo->Amount = 0;
				iammo->AttachToOwner (players[playernum].mo);
			}
			iammo->MaxAmount = amount;
			if (players[playernum].mo->FindInventory<ABackpack>()==NULL)
			{
				if (t_argc>=4) amount = intvalue(t_argv[3]);
				else amount*=2;
			}
			iammo->BackpackMaxAmount=amount;
		}

		t_return.type = svt_int;
		AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
		if (iammo) t_return.value.i = iammo->MaxAmount;
		else t_return.value.i = ((AAmmo*)GetDefaultByType(ammotype))->MaxAmount;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerWeapon()
{
	static const char * const WeaponNames[]={
		"Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher", 
		"PlasmaRifle", "BFG9000", "Chainsaw", "SuperShotgun" };


	// This function is just kept for backwards compatibility and intentionally limited to the standard weapons!
	// Use Give/Take/CheckInventory instead!
    int playernum;
    int weaponnum;
    int newweapon;
	
	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		weaponnum = intvalue(t_argv[1]);
		if (playernum==-1) return;
		if (weaponnum<0 || weaponnum>9)
		{
			script_error("weaponnum out of range! %s\n", weaponnum);
			return;
		}
		const PClass * ti = PClass::FindClass(WeaponNames[weaponnum]);
		if (!ti)
		{
			script_error("incompatibility in playerweapon\n", weaponnum);
			return;
		}
		
		if (t_argc == 2)
		{
			AActor * wp = players[playernum].mo->FindInventory(ti);
			t_return.type = svt_int;
			t_return.value.i = wp!=NULL;;
			return;
		}
		else
		{
			AActor * wp = players[playernum].mo->FindInventory(ti);

			newweapon = !!intvalue(t_argv[2]);
			if (!newweapon)
			{
				if (wp) 
				{
					wp->Destroy();
					// If the weapon is active pick a replacement. Legacy didn't do this!
					if (players[playernum].PendingWeapon==wp) players[playernum].PendingWeapon=WP_NOCHANGE;
					if (players[playernum].ReadyWeapon==wp) 
					{
						players[playernum].ReadyWeapon=NULL;
						players[playernum].mo->PickNewWeapon(NULL);
					}
				}
			}
			else 
			{
				if (!wp) 
				{
					AWeapon * pw=players[playernum].PendingWeapon;
					players[playernum].mo->GiveInventoryType(ti);
					players[playernum].PendingWeapon=pw;
				}
			}
			
			t_return.type = svt_int;
			t_return.value.i = !!newweapon;
			return;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerSelectedWeapon()
{
	int playernum;
	int weaponnum;

	// This function is just kept for backwards compatibility and intentionally limited to the standard weapons!

	static const char * const WeaponNames[]={
		"Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher", 
		"PlasmaRifle", "BFG9000", "Chainsaw", "SuperShotgun" };


	if (T_CheckArgs(1))
	{
		playernum=T_GetPlayerNum(t_argv[0]);

		if(t_argc == 2)
		{
			weaponnum = intvalue(t_argv[1]);

			if (weaponnum<0 || weaponnum>=9)
			{
				script_error("weaponnum out of range! %s\n", weaponnum);
				return;
			}
			const PClass * ti = PClass::FindClass(WeaponNames[weaponnum]);
			if (!ti)
			{
				script_error("incompatibility in playerweapon\n", weaponnum);
				return;
			}

			players[playernum].PendingWeapon = (AWeapon*)players[playernum].mo->FindInventory(ti);

		} 
		t_return.type = svt_int;
		for(int i=0;i<9;i++)
		{
			if (players[playernum].ReadyWeapon->GetClass ()->TypeName == FName(WeaponNames[i]))
			{
				t_return.value.i=i;
				break;
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_GiveInventory(void)
{
	int  playernum, count;
	
	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		if(t_argc == 2) count=1;
		else count=intvalue(t_argv[2]);
		FS_GiveInventory(players[playernum].mo, stringvalue(t_argv[1]), count);
		t_return.type = svt_int;
		t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_TakeInventory(void)
{
	int  playernum, count;

	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		if(t_argc == 2) count=32767;
		else count=intvalue(t_argv[2]);
		FS_TakeInventory(players[playernum].mo, stringvalue(t_argv[1]), count);
		t_return.type = svt_int;
		t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_CheckInventory(void)
{
	int  playernum;
	
	if (T_CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) 
		{
			t_return.value.i = 0;
			return;
		}
		t_return.type = svt_int;
		t_return.value.i = FS_CheckInventory(players[playernum].mo, stringvalue(t_argv[1]));
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SetWeapon()
{
	if (T_CheckArgs(2))
	{
		int playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum!=-1) 
		{
			AInventory *item = players[playernum].mo->FindInventory (PClass::FindClass (stringvalue(t_argv[1])));

			if (item == NULL || !item->IsKindOf (RUNTIME_CLASS(AWeapon)))
			{
			}
			else if (players[playernum].ReadyWeapon == item)
			{
				// The weapon is already selected, so setweapon succeeds by default,
				// but make sure the player isn't switching away from it.
				players[playernum].PendingWeapon = WP_NOCHANGE;
				t_return.value.i = 1;
			}
			else
			{
				AWeapon *weap = static_cast<AWeapon *> (item);

				if (weap->CheckAmmo (AWeapon::EitherFire, false))
				{
					// There's enough ammo, so switch to it.
					t_return.value.i = 1;
					players[playernum].PendingWeapon = weap;
				}
			}
		}
		t_return.value.i = 0;
	}
}

// removed SF_PlayerMaxAmmo



//
// movecamera(camera, targetobj, targetheight, movespeed, targetangle, anglespeed)
//

void SF_MoveCamera(void)
{
	fixed_t    x, y, z;  
	fixed_t    xdist, ydist, zdist, xydist, movespeed;
	fixed_t    xstep, ystep, zstep, targetheight;
	angle_t    anglespeed, anglestep, angledist, targetangle, 
		mobjangle, bigangle, smallangle;
	
	// I have to use floats for the math where angles are divided 
	// by fixed values.  
	double     fangledist, fanglestep, fmovestep;
	int	     angledir;  
	AActor*    target;
	int        moved;
	int        quad1, quad2;
	AActor		* cam;
	
	angledir = moved = 0;

	if (T_CheckArgs(6))
	{
		cam = MobjForSvalue(t_argv[0]);

		target = MobjForSvalue(t_argv[1]);
		if(!cam || !target) 
		{ 
			script_error("invalid target for camera\n"); return; 
		}
		
		targetheight = fixedvalue(t_argv[2]);
		movespeed    = fixedvalue(t_argv[3]);
		targetangle  = (angle_t)FixedToAngle(fixedvalue(t_argv[4]));
		anglespeed   = (angle_t)FixedToAngle(fixedvalue(t_argv[5]));
		
		// figure out how big one step will be
		xdist = target->x - cam->x;
		ydist = target->y - cam->y;
		zdist = targetheight - cam->z;
		
		// Angle checking...  
		//    90  
		//   Q1|Q0  
		//180--+--0  
		//   Q2|Q3  
		//    270
		quad1 = targetangle / ANG90;
		quad2 = cam->angle / ANG90;
		bigangle = targetangle > cam->angle ? targetangle : cam->angle;
		smallangle = targetangle < cam->angle ? targetangle : cam->angle;
		if((quad1 > quad2 && quad1 - 1 == quad2) || (quad2 > quad1 && quad2 - 1 == quad1) ||
			quad1 == quad2)
		{
			angledist = bigangle - smallangle;
			angledir = targetangle > cam->angle ? 1 : -1;
		}
		else
		{
			angle_t diff180 = (bigangle + ANG180) - (smallangle + ANG180);
			
			if(quad2 == 3 && quad1 == 0)
			{
				angledist = diff180;
				angledir = 1;
			}
			else if(quad1 == 3 && quad2 == 0)
			{
				angledist = diff180;
				angledir = -1;
			}
			else
			{
				angledist = bigangle - smallangle;
				if(angledist > ANG180)
				{
					angledist = diff180;
					angledir = targetangle > cam->angle ? -1 : 1;
				}
				else
					angledir = targetangle > cam->angle ? 1 : -1;
			}
		}
		
		// set step variables based on distance and speed
		mobjangle = R_PointToAngle2(cam->x, cam->y, target->x, target->y);
		xydist = R_PointToDist2(target->x - cam->x, target->y - cam->y);
		
		xstep = FixedMul(finecosine[mobjangle >> ANGLETOFINESHIFT], movespeed);
		ystep = FixedMul(finesine[mobjangle >> ANGLETOFINESHIFT], movespeed);
		
		if(xydist && movespeed)
			zstep = FixedDiv(zdist, FixedDiv(xydist, movespeed));
		else
			zstep = zdist > 0 ? movespeed : -movespeed;
		
		if(xydist && movespeed && !anglespeed)
		{
			fangledist = ((double)angledist / (ANG45/45));
			fmovestep = ((double)FixedDiv(xydist, movespeed) / FRACUNIT);
			if(fmovestep)
				fanglestep = fangledist / fmovestep;
			else
				fanglestep = 360;
			
			anglestep =(angle_t) (fanglestep * (ANG45/45));
		}
		else
			anglestep = anglespeed;
		
		if(abs(xstep) >= (abs(xdist) - 1))
			x = target->x;
		else
		{
			x = cam->x + xstep;
			moved = 1;
		}
		
		if(abs(ystep) >= (abs(ydist) - 1))
			y = target->y;
		else
		{
			y = cam->y + ystep;
			moved = 1;
		}
		
		if(abs(zstep) >= (abs(zdist) - 1))
			z = targetheight;
		else
		{
			z = cam->z + zstep;
			moved = 1;
		}
		
		if(anglestep >= angledist)
			cam->angle = targetangle;
		else
		{
			if(angledir == 1)
			{
				cam->angle += anglestep;
				moved = 1;
			}
			else if(angledir == -1)
			{
				cam->angle -= anglestep;
				moved = 1;
			}
		}

		cam->radius=8;
		cam->height=8;
		if ((x != cam->x || y != cam->y) && !P_TryMove(cam, x, y, true))
		{
			Printf("Illegal camera move to (%f, %f)\n", x/65536.f, y/65536.f);
			return;
		}
		cam->z = z;

		t_return.type = svt_int;
		t_return.value.i = moved;
	}
}



//==========================================================================
//
// SF_ObjAwaken
//
// Implements: void objawaken([mobj mo])
//
//==========================================================================

void SF_ObjAwaken(void)
{
   AActor *mo;

   if(!t_argc)
      mo = current_script->trigger;
   else
      mo = MobjForSvalue(t_argv[0]);

   if(mo)
   {
	   mo->Activate(current_script->trigger);
   }
}

//==========================================================================
//
// SF_AmbientSound
//
// Implements: void ambientsound(string name)
//
//==========================================================================

void SF_AmbientSound(void)
{
	if (T_CheckArgs(1))
	{
		S_SoundID(CHAN_AUTO, T_FindSound(stringvalue(t_argv[0])), 1, ATTN_NORM);
	}
}


//==========================================================================
// 
// SF_ExitSecret
//
// Implements: void exitsecret()
//
//==========================================================================

void SF_ExitSecret(void)
{
	G_ExitLevel(0, false);
}


//==========================================================================
//
//
//
//==========================================================================

// Type forcing functions -- useful with arrays et al

void SF_MobjValue(void)
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_mobj;
		t_return.value.mobj = MobjForSvalue(t_argv[0]);
	}
}

void SF_StringValue(void)
{  
	if (T_CheckArgs(1))
	{
		t_return.type = svt_string;
		t_return.value.s = strdup(stringvalue(t_argv[0]));
		// this cannot be handled in any sensible way - yuck!
		levelpointers.Push(t_return.value.s);
	}
}

void SF_IntValue(void)
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_int;
		t_return.value.i = intvalue(t_argv[0]);
	}
}

void SF_FixedValue(void)
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = fixedvalue(t_argv[0]);
	}
}


//==========================================================================
//
// Starting here are functions present in Legacy but not Eternity.
//
//==========================================================================

void SF_SpawnExplosion()
{
	fixed_t   x, y, z;
	AActor*   spawn;
	const PClass * PClass;
	
	if (T_CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		x = fixedvalue(t_argv[1]);
		y = fixedvalue(t_argv[2]);
		if(t_argc > 3)
			z = fixedvalue(t_argv[3]);
		else
			z = R_PointInSubsector(x, y)->sector->floorplane.ZatPoint(x,y);
		
		spawn = Spawn (PClass, x, y, z, ALLOW_REPLACE);
		t_return.type = svt_int;
		t_return.value.i=0;
		if (spawn)
		{
			if (spawn->flags&MF_COUNTKILL) level.total_monsters--;
			if (spawn->flags&MF_COUNTITEM) level.total_items--;
			spawn->flags&=~(MF_COUNTKILL|MF_COUNTITEM);
			t_return.value.i = spawn->SetState(spawn->DeathState);
			if(spawn->DeathSound) S_SoundID (spawn, CHAN_BODY, spawn->DeathSound, 1, ATTN_NORM);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_RadiusAttack()
{
    AActor *spot;
    AActor *source;
    int damage;

	if (T_CheckArgs(3))
	{
		spot = MobjForSvalue(t_argv[0]);
		source = MobjForSvalue(t_argv[1]);
		damage = intvalue(t_argv[2]);

		if (spot && source)
		{
			P_RadiusAttack(spot, source, damage, damage, MOD_UNKNOWN, true);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SetObjPosition()
{
	AActor* mobj;

	if (T_CheckArgs(2))
	{
		mobj = MobjForSvalue(t_argv[0]);

		if (!mobj) return;

		mobj->UnlinkFromWorld();

		mobj->x = intvalue(t_argv[1]) << FRACBITS;
		if(t_argc >= 3) mobj->y = intvalue(t_argv[2]) << FRACBITS;
		if(t_argc == 4)	mobj->z = intvalue(t_argv[3]) << FRACBITS;

		mobj->LinkToWorld();
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_TestLocation()
{
    AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    if (mo)
	{
       t_return.type = svt_int;
       t_return.value.f = !!P_TestMobjLocation(mo);
    }
}

//==========================================================================
//
//
//
//==========================================================================

void SF_HealObj()  //no pain sound
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

	if(t_argc < 2)
	{
		mo->health = mo->GetDefault()->health;
		if(mo->player) mo->player->health = mo->health;
	}

	else if (t_argc == 2)
	{
		mo->health += intvalue(t_argv[1]);
		if(mo->player) mo->player->health = mo->health;
	}

	else
		script_error("invalid number of arguments for objheal");
}


//==========================================================================
//
//
//
//==========================================================================

void SF_ObjDead()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	if(mo && (mo->health <= 0 || mo->flags&MF_CORPSE))
		t_return.value.i = 1;
	else
		t_return.value.i = 0;
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SpawnMissile()
{
    AActor *mobj;
    AActor *target;
	const PClass * PClass;

	if (T_CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[2]))) return;

		mobj = MobjForSvalue(t_argv[0]);
		target = MobjForSvalue(t_argv[1]);
		if (mobj && target) P_SpawnMissile(mobj, target, PClass);
	}
}

//==========================================================================
//
//checks to see if a Map Thing Number exists; used to avoid script errors
//
//==========================================================================

void SF_MapThingNumExist()
{

    int intval;

	if (T_CheckArgs(1))
	{
		intval = intvalue(t_argv[0]);

		if (intval < 0 || intval >= SpawnedThings.Size() || !SpawnedThings[intval]->actor)
		{
			t_return.type = svt_int;
			t_return.value.i = 0;
		}
		else
		{
			t_return.type = svt_int;
			t_return.value.i = 1;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_MapThings()
{
    t_return.type = svt_int;
    t_return.value.i = SpawnedThings.Size();
}


//==========================================================================
//
//
//
//==========================================================================

void SF_ObjState()  //DarkWolf95:November 15, 2003: Adaptaion of Exl's code
{					//DarkWolf95:December 7, 2003: Change to set only
	FState * newstate;
	int state;
	AActor	*mo;

	if (T_CheckArgs(1))
	{
		if(t_argc == 1)
		{
			mo = current_script->trigger;
			state = intvalue(t_argv[0]);
		}

		else if(t_argc == 2)
		{
			mo = MobjForSvalue(t_argv[0]);
			state = intvalue(t_argv[1]);
		}

		if (!mo) 
		{
			t_return.type = svt_int;
			t_return.value.i = 0;
			return;
		}

		switch (state)
		{
			case 1:
				newstate = mo->SpawnState;
				break;
			case 2:
				newstate = mo->SeeState;
				break;
			case 3:
				newstate = mo->MissileState;
				break;
			case 4:
				newstate = mo->MeleeState;
				break;
			case 5:
				newstate = mo->PainState;
				break;
			case 6:
				newstate = mo->DeathState;
				break;
			case 7:
				newstate = mo->RaiseState;
				break;
			case 8:
				newstate = mo->XDeathState;
				break;
			case 9:
				newstate = mo->CrashState;
				break;
			default:
				script_error("objstate: invalid state");
				return;
		}

		t_return.type = svt_int;
		t_return.value.i = mo->SetState(newstate);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_LineFlag()
{
	line_t*  line;
	int      linenum;
	int      flagnum;
	
	if (T_CheckArgs(2))
	{
		linenum = intvalue(t_argv[0]);
		if(linenum < 0 || linenum > numlines)
		{
			script_error("LineFlag: Invalid line number.\n");
			return;
		}
		
		line = lines + linenum;
		
		flagnum = intvalue(t_argv[1]);
		if(flagnum < 0 || (flagnum > 8 && flagnum!=15))
		{
			script_error("LineFlag: Invalid flag number.\n");
			return;
		}
		
		if(t_argc > 2)
		{
			line->flags &= ~(1 << flagnum);
			if(intvalue(t_argv[2]))
				line->flags |= (1 << flagnum);
		}
		
		t_return.type = svt_int;
		t_return.value.i = line->flags & (1 << flagnum);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void SF_PlayerAddFrag()
{
	int playernum1;
	int playernum2;

	if (T_CheckArgs(1))
	{
		if (t_argc == 1)
		{
			playernum1 = T_GetPlayerNum(t_argv[0]);

			players[playernum1].fragcount++;

			t_return.type = svt_int;
			t_return.value.f = players[playernum1].fragcount;
		}

		else
		{
			playernum1 = T_GetPlayerNum(t_argv[0]);
			playernum2 = T_GetPlayerNum(t_argv[1]);

			players[playernum1].frags[playernum2]++;

			t_return.type = svt_int;
			t_return.value.f = players[playernum1].frags[playernum2];
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_SkinColor()
{
	// Ignoring it for now.
}

void SF_PlayDemo()
{ 
	// Ignoring it for now.
}

void SF_CheckCVar()
{
	// can't be done so return 0.
}
//==========================================================================
//
//
//
//==========================================================================

void SF_Resurrect()
{

	AActor *mo;

	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);

		if(!mo->RaiseState)  //Don't resurrect things that can't be resurrected
			return;

		mo->SetState(mo->RaiseState);
		mo->height = mo->GetDefault()->height;
		mo->radius = mo->GetDefault()->radius;
		mo->flags =  mo->GetDefault()->flags;
		mo->flags2 = mo->GetDefault()->flags2;
		mo->health = mo->GetDefault()->health;
		mo->target = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void SF_LineAttack()
{
	AActor	*mo;
	int		damage, angle, slope;

	if (T_CheckArgs(3))
	{
		mo = MobjForSvalue(t_argv[0]);
		damage = intvalue(t_argv[2]);

		angle = (intvalue(t_argv[1]) * (ANG45 / 45));
		slope = P_AimLineAttack(mo, angle, MISSILERANGE);

		P_LineAttack(mo, angle, MISSILERANGE, slope, damage, MOD_UNKNOWN, RUNTIME_CLASS(ABulletPuff));
	}
}

//==========================================================================
//
// This is a lousy hack. It only works for the standard actors
// and it is quite inefficient.
//
//==========================================================================

void SF_ObjType()
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

	for(int i=0;i<countof(ActorTypes);i++) if (mo->GetClass() == ActorTypes[i])
	{
		t_return.type = svt_int;
		t_return.value.i = i;
		return;
	}
	t_return.type = svt_int;
	t_return.value.i = -1;
}

//==========================================================================
//
// some new math functions
//
//==========================================================================

inline fixed_t double2fixed(double t)
{
	return (fixed_t)(t*65536.0);
}



void SF_Sin()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(sin(floatvalue(t_argv[0])));
	}
}


void SF_ASin()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(asin(floatvalue(t_argv[0])));
	}
}


void SF_Cos()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(cos(floatvalue(t_argv[0])));
	}
}


void SF_ACos()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(acos(floatvalue(t_argv[0])));
	}
}


void SF_Tan()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(tan(floatvalue(t_argv[0])));
	}
}


void SF_ATan()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(atan(floatvalue(t_argv[0])));
	}
}


void SF_Exp()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(exp(floatvalue(t_argv[0])));
	}
}

void SF_Log()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(log(floatvalue(t_argv[0])));
	}
}


void SF_Sqrt()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(sqrt(floatvalue(t_argv[0])));
	}
}


void SF_Floor()
{
	if (T_CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = fixedvalue(t_argv[0]) & 0xffFF0000;
	}
}


void SF_Pow()
{
	if (T_CheckArgs(2))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(pow(floatvalue(t_argv[0]), floatvalue(t_argv[1])));
	}
}

//==========================================================================
//
// HUD pics (not operational yet!)
//
//==========================================================================


int HU_GetFSPic(int lumpnum, int xpos, int ypos);
int HU_DeleteFSPic(unsigned int handle);
int HU_ModifyFSPic(unsigned int handle, int lumpnum, int xpos, int ypos);
int HU_FSDisplay(unsigned int handle, bool newval);

void SF_NewHUPic()
{
	if (T_CheckArgs(3))
	{
		t_return.type = svt_int;
		t_return.value.i = HU_GetFSPic(
			TexMan.GetTexture(stringvalue(t_argv[0]), FTexture::TEX_MiscPatch, FTextureManager::TEXMAN_TryAny), 
			intvalue(t_argv[1]), intvalue(t_argv[2]));
	}
}

void SF_DeleteHUPic()
{
	if (T_CheckArgs(1))
	{
	    if (HU_DeleteFSPic(intvalue(t_argv[0])) == -1)
		    script_error("deletehupic: Invalid sfpic handle: %i\n", intvalue(t_argv[0]));
	}
}

void SF_ModifyHUPic()
{
    if (t_argc != 4)
    {
        script_error("modifyhupic: invalid number of arguments\n");
        return;
    }

    if (HU_ModifyFSPic(intvalue(t_argv[0]), 
			TexMan.GetTexture(stringvalue(t_argv[0]), FTexture::TEX_MiscPatch, FTextureManager::TEXMAN_TryAny),
			intvalue(t_argv[2]), intvalue(t_argv[3])) == -1)
	{
        script_error("modifyhypic: invalid sfpic handle %i\n", intvalue(t_argv[0]));
	}
    return;
}

void SF_SetHUPicDisplay()
{
    if (t_argc != 2)
    {
        script_error("sethupicdisplay: invalud number of arguments\n");
        return;
    }

    if (HU_FSDisplay(intvalue(t_argv[0]), intvalue(t_argv[1]) > 0 ? 1 : 0) == -1)
        script_error("sethupicdisplay: invalid pic handle %i\n", intvalue(t_argv[0]));
}


//==========================================================================
//
// Yet to be made operational.
//
//==========================================================================

void SF_SetCorona(void)
{
	if(t_argc != 3)
	{
		script_error("incorrect arguments to function\n");
		return;
	}
	
    int num = t_argv[0].value.i;    // which corona we want to modify
    int what = t_argv[1].value.i;   // what we want to modify (type, color, offset,...)
    int ival = t_argv[2].value.i;   // the value of what we modify
    double fval = ((double) t_argv[2].value.f / FRACUNIT);

  	/*
    switch (what)
    {
        case 0:
            lspr[num].type = ival;
            break;
        case 1:
            lspr[num].light_xoffset = fval;
            break;
        case 2:
            lspr[num].light_yoffset = fval;
            break;
        case 3:
            if (t_argv[2].type == svt_string)
                lspr[num].corona_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].corona_color, &ival, sizeof(int));
            break;
        case 4:
            lspr[num].corona_radius = fval;
            break;
        case 5:
            if (t_argv[2].type == svt_string)
                lspr[num].dynamic_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].dynamic_color, &ival, sizeof(int));
            break;
        case 6:
            lspr[num].dynamic_radius = fval;
            lspr[num].dynamic_sqrradius = sqrt(lspr[num].dynamic_radius);
            break;
        default:
            CONS_Printf("Error in setcorona\n");
            break;
    }
    */

	// no use for this!
	t_return.type = svt_int;
	t_return.value.i = 0;
}

//==========================================================================
//
// new for GZDoom: Call a Hexen line special
//
//==========================================================================

void SF_Ls()
{
	int args[5]={0,0,0,0,0};
	int spc;

	if (T_CheckArgs(1))
	{
		spc=intvalue(t_argv[0]);
		for(int i=0;i<5;i++)
		{
			if (t_argc>=i+2) args[i]=intvalue(t_argv[i+1]);
		}
		if (spc>=0 && spc<256)
			LineSpecials[spc](NULL,current_script->trigger,false, args[0],args[1],args[2],args[3],args[4]);
	}
}


//==========================================================================
//
// new for GZDoom: Gets the levelnum
//
//==========================================================================

void SF_LevelNum()
{
	t_return.type = svt_int;
	t_return.value.f = level.levelnum;
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void SF_MobjRadius(void)
{
	AActor*   mo;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->radius = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->radius : 0;
	}
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void SF_MobjHeight(void)
{
	AActor*   mo;
	
	if (T_CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->height = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->height : 0;
	}
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void SF_ThingCount(void)
{
	const PClass *PClass;
	AActor * mo;
	int count=0;
	
	
	if (T_CheckArgs(1))
	{
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		TThinkerIterator<AActor> it;

		if (t_argc<2 || intvalue(t_argv[1])==0)
		{
			while (mo=it.Next()) 
			{
				if (mo->IsA(PClass))
				{
					if (!mo->IsKindOf (RUNTIME_CLASS(AInventory)) ||
						static_cast<AInventory *>(mo)->Owner == NULL)
					{
						count++;
					}
				}
			}
		}
		else
		{
			while (mo=it.Next())
			{
				if (mo->IsA(PClass) && mo->health>0) count++;
			}
		}
		t_return.type = svt_int;
		t_return.value.i = count;
	}	
}

//==========================================================================
//
// new for GZDoom: Sets a sector color
//
//==========================================================================

void SF_SetColor(void)
{
	int tagnum, secnum;
	int c=2;
	int i = -1;
	PalEntry color=0;
	
	if (T_CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ 
			return;
		}
		
		if(t_argc >1 && t_argc<4)
		{
			color=intvalue(t_argv[1]);
		}
		else if (t_argc>=4)
		{
			color.r=intvalue(t_argv[1]);
			color.g=intvalue(t_argv[2]);
			color.b=intvalue(t_argv[3]);
			color.a=0;
		}
		else return;

		// set all sectors with tag
		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sectors[i].ColorMap = GetSpecialLights (color, sectors[i].ColorMap->Fade, 0);
		}
	}
}


//==========================================================================
//
// Spawns a projectile at a map spot
//
//==========================================================================

void SF_SpawnShot2(void)
{
	AActor *source = NULL;
	const PClass * PClass;
	int z=0;
	
	// t_argv[0] = type to spawn
	// t_argv[1] = source mobj, optional, -1 to default
	// shoots at source's angle
	
	if (T_CheckArgs(2))
	{
		if(t_argv[1].type == svt_int && t_argv[1].value.i < 0)
			source = current_script->trigger;
		else
			source = MobjForSvalue(t_argv[1]);

		if (t_argc>2) z=fixedvalue(t_argv[2]);
		
		if(!source)	return;
		
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		t_return.type = svt_mobj;

		AActor *mo = Spawn (PClass, source->x, source->y, source->z+z, ALLOW_REPLACE);
		if (mo) 
		{
			S_SoundID (mo, CHAN_VOICE, mo->SeeSound, 1, ATTN_NORM);
			mo->target = source;
			P_ThrustMobj(mo, mo->angle = source->angle, mo->Speed);
			if (!P_CheckMissileSpawn(mo)) mo = NULL;
		}
		t_return.value.mobj = mo;
	}
}



//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void  SF_KillInSector()
{
	if (T_CheckArgs(1))
	{
		TThinkerIterator<AActor> it;
		AActor * mo;
		int tag=intvalue(t_argv[0]);

		while (mo=it.Next())
		{
			if (mo->flags3&MF3_ISMONSTER && mo->Sector->tag==tag) P_DamageMobj(mo, NULL, NULL, 1000000, MOD_UNKNOWN);
		}
	}
}

//==========================================================================
//
// new for GZDoom: Sets a sector's type
// (Sure, this is not particularly useful. But having it made it possible
//  to fix a few annoying bugs in some old maps ;) )
//
//==========================================================================

void SF_SectorType(void)
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (T_CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int spec = intvalue(t_argv[1]);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].special = spec;
			}
		}
		
		t_return.type = svt_int;
		t_return.value.i = sector->special;
	}
}

//==========================================================================
//
// new for GZDoom: Sets a new line trigger type (Doom format!)
// (Sure, this is not particularly useful. But having it made it possible
//  to fix a few annoying bugs in some old maps ;) )
//
//==========================================================================

void SF_SetLineTrigger()
{
	int i,id,spec,tag;

	if (T_CheckArgs(2))
	{
		id=intvalue(t_argv[0]);
		spec=intvalue(t_argv[1]);
		if (t_argc>2) tag=intvalue(t_argv[2]);
		for (i = -1; (i = P_FindLineFromID (id, i)) >= 0; )
		{
			if (t_argc==2) tag=lines[i].id;
			maplinedef_t mld;
			mld.special=spec;
			mld.tag=tag;
			mld.flags=0;
			int f = lines[i].flags;
			P_TranslateLineDef(&lines[i], &mld);	
			lines[i].id=tag;
			lines[i].flags = (lines[i].flags & (ML_MONSTERSCANACTIVATE|ML_REPEAT_SPECIAL|ML_SPAC_MASK)) |
										(f & ~(ML_MONSTERSCANACTIVATE|ML_REPEAT_SPECIAL|ML_SPAC_MASK));

		}
	}
}


//==========================================================================
//
// new for GZDoom: Changes a sector's tag
// (I only need this because MAP02 in RTC-3057 has some issues with the GL 
// renderer that I can't fix without the scripts. But loading a FS on top on
// ACS still works so I can hack around it with this.)
//
//==========================================================================

void P_InitTagLists();

void SF_ChangeTag()
{
	if (T_CheckArgs(2))
	{
		for (int secnum = -1; (secnum = P_FindSectorFromTag (t_argv[0].value.i, secnum)) >= 0; ) 
		{
			sectors[secnum].tag=t_argv[1].value.i;
		}

		// Recreate the hash tables
		int i;

		for (i=numsectors; --i>=0; ) sectors[i].firsttag = -1;
		for (i=numsectors; --i>=0; )
		{
			int j = (unsigned) sectors[i].tag % (unsigned) numsectors;
			sectors[i].nexttag = sectors[j].firsttag;
			sectors[j].firsttag = i;
		}
	}
}


void SF_WallGlow()
{
	// Development garbage!
}



//////////////////////////////////////////////////////////////////////////
//
// Init Functions
//
static int zoom=1;	// Dummy - no longer needed!

void init_functions(void)
{
	for(int i=0;i<countof(ActorNames_init);i++)
	{
		ActorTypes[i]=PClass::FindClass(ActorNames_init[i]);
	}

	// add all the functions
	add_game_int("consoleplayer", &consoleplayer);
	add_game_int("displayplayer", &consoleplayer);
	add_game_int("zoom", &zoom);
	add_game_int("fov", &zoom); // haleyjd: temporary alias (?)
	add_game_mobj("trigger", &trigger_obj);
	
	// important C-emulating stuff
	new_function("break", SF_Break);
	new_function("continue", SF_Continue);
	new_function("return", SF_Return);
	new_function("goto", SF_Goto);
	new_function("include", SF_Include);
	
	// standard FraggleScript functions
	new_function("print", SF_Print);
	new_function("rnd", SF_Rnd);	// Legacy uses a normal rand() call for this which is extremely dangerous.
	new_function("prnd", SF_Rnd);	// I am mapping rnd and prnd to the same named RNG which should eliminate any problem
	new_function("input", SF_Input);
	new_function("beep", SF_Beep);
	new_function("clock", SF_Clock);
	new_function("wait", SF_Wait);
	new_function("tagwait", SF_TagWait);
	new_function("scriptwait", SF_ScriptWait);
	new_function("startscript", SF_StartScript);
	new_function("scriptrunning", SF_ScriptRunning);
	
	// doom stuff
	new_function("startskill", SF_StartSkill);
	new_function("exitlevel", SF_ExitLevel);
	new_function("tip", SF_Tip);
	new_function("timedtip", SF_TimedTip);
	new_function("message", SF_Message);
	new_function("gameskill", SF_Gameskill);
	new_function("gamemode", SF_Gamemode);
	
	// player stuff
	new_function("playermsg", SF_PlayerMsg);
	new_function("playertip", SF_PlayerTip);
	new_function("playeringame", SF_PlayerInGame);
	new_function("playername", SF_PlayerName);
    new_function("playeraddfrag", SF_PlayerAddFrag);
	new_function("playerobj", SF_PlayerObj);
	new_function("isplayerobj", SF_IsPlayerObj);
	new_function("isobjplayer", SF_IsPlayerObj);
	new_function("skincolor", SF_SkinColor);
	new_function("playerkeys", SF_PlayerKeys);
	new_function("playerammo", SF_PlayerAmmo);
    new_function("maxplayerammo", SF_MaxPlayerAmmo); 
	new_function("playerweapon", SF_PlayerWeapon);
	new_function("playerselwep", SF_PlayerSelectedWeapon);
	
	// mobj stuff
	new_function("spawn", SF_Spawn);
	new_function("spawnexplosion", SF_SpawnExplosion);
    new_function("radiusattack", SF_RadiusAttack);
	new_function("kill", SF_KillObj);
	new_function("removeobj", SF_RemoveObj);
	new_function("objx", SF_ObjX);
	new_function("objy", SF_ObjY);
	new_function("objz", SF_ObjZ);
    new_function("testlocation", SF_TestLocation);
	new_function("teleport", SF_Teleport);
	new_function("silentteleport", SF_SilentTeleport);
	new_function("damageobj", SF_DamageObj);
	new_function("healobj", SF_HealObj);
	new_function("player", SF_Player);
	new_function("objsector", SF_ObjSector);
	new_function("objflag", SF_ObjFlag);
	new_function("pushobj", SF_PushThing);
	new_function("pushthing", SF_PushThing);
	new_function("objangle", SF_ObjAngle);
	new_function("objhealth", SF_ObjHealth);
	new_function("objdead", SF_ObjDead);
	new_function("reactiontime", SF_ReactionTime);
	new_function("objreactiontime", SF_ReactionTime);
	new_function("objtarget", SF_MobjTarget);
	new_function("objmomx", SF_MobjMomx);
	new_function("objmomy", SF_MobjMomy);
	new_function("objmomz", SF_MobjMomz);

    new_function("spawnmissile", SF_SpawnMissile);
    new_function("mapthings", SF_MapThings);
    new_function("objtype", SF_ObjType);
    new_function("mapthingnumexist", SF_MapThingNumExist);
	new_function("objstate", SF_ObjState);
	new_function("resurrect", SF_Resurrect);
	new_function("lineattack", SF_LineAttack);
	new_function("setobjposition", SF_SetObjPosition);

	// sector stuff
	new_function("floorheight", SF_FloorHeight);
	new_function("floortext", SF_FloorTexture);
	new_function("floortexture", SF_FloorTexture);   // haleyjd: alias
	new_function("movefloor", SF_MoveFloor);
	new_function("ceilheight", SF_CeilingHeight);
	new_function("ceilingheight", SF_CeilingHeight); // haleyjd: alias
	new_function("moveceil", SF_MoveCeiling);
	new_function("moveceiling", SF_MoveCeiling);     // haleyjd: aliases
	new_function("ceilingtexture", SF_CeilingTexture);
	new_function("ceiltext", SF_CeilingTexture);  // haleyjd: wrong
	new_function("lightlevel", SF_LightLevel);    // handler - was
	new_function("fadelight", SF_FadeLight);      // SF_FloorTexture!
	new_function("colormap", SF_SectorColormap);
	
	// cameras!
	new_function("setcamera", SF_SetCamera);
	new_function("clearcamera", SF_ClearCamera);
	new_function("movecamera", SF_MoveCamera);
	
	// trig functions
	new_function("pointtoangle", SF_PointToAngle);
	new_function("pointtodist", SF_PointToDist);
	
	// sound functions
	new_function("startsound", SF_StartSound);
	new_function("startsectorsound", SF_StartSectorSound);
	new_function("ambientsound", SF_AmbientSound);
	new_function("startambiantsound", SF_AmbientSound);	// Legacy's incorrectly spelled name!
	new_function("changemusic", SF_ChangeMusic);
	
	// hubs!
	new_function("changehublevel", SF_ChangeHubLevel);
	
	// doors
	new_function("opendoor", SF_OpenDoor);
	new_function("closedoor", SF_CloseDoor);

    // HU Graphics
    new_function("newhupic", SF_NewHUPic);
    new_function("createpic", SF_NewHUPic);
    new_function("deletehupic", SF_DeleteHUPic);
    new_function("modifyhupic", SF_ModifyHUPic);
    new_function("modifypic", SF_ModifyHUPic);
    new_function("sethupicdisplay", SF_SetHUPicDisplay);
    new_function("setpicvisible", SF_SetHUPicDisplay);

	//
    new_function("playdemo", SF_PlayDemo);
	new_function("runcommand", SF_RunCommand);
    new_function("checkcvar", SF_CheckCVar);
	new_function("setlinetexture", SF_SetLineTexture);
	new_function("linetrigger", SF_LineTrigger);
	new_function("lineflag", SF_LineFlag);

	//Hurdler: new math functions
	new_function("max", SF_Max);
	new_function("min", SF_Min);
	new_function("abs", SF_Abs);

	new_function("sin", SF_Sin);
	new_function("asin", SF_ASin);
	new_function("cos", SF_Cos);
	new_function("acos", SF_ACos);
	new_function("tan", SF_Tan);
	new_function("atan", SF_ATan);
	new_function("exp", SF_Exp);
	new_function("log", SF_Log);
	new_function("sqrt", SF_Sqrt);
	new_function("floor", SF_Floor);
	new_function("pow", SF_Pow);
	
	// Eternity extensions
	new_function("setlineblocking", SF_SetLineBlocking);
	new_function("setlinetrigger", SF_SetLineTrigger);
	new_function("setlinemnblock", SF_SetLineMonsterBlocking);
	new_function("scriptwaitpre", SF_ScriptWaitPre);
	new_function("exitsecret", SF_ExitSecret);
	new_function("objawaken", SF_ObjAwaken);
	
	// forced coercion functions
	new_function("mobjvalue", SF_MobjValue);
	new_function("stringvalue", SF_StringValue);
	new_function("intvalue", SF_IntValue);
	new_function("fixedvalue", SF_FixedValue);

	// new for GZDoom
	new_function("spawnshot2", SF_SpawnShot2);
	new_function("setcolor", SF_SetColor);
	new_function("sectortype", SF_SectorType);
	new_function("wallglow", SF_WallGlow);
	new_function("objradius", SF_MobjRadius);
	new_function("objheight", SF_MobjHeight);
	new_function("thingcount", SF_ThingCount);
	new_function("killinsector", SF_KillInSector);
	new_function("changetag", SF_ChangeTag);
	new_function("levelnum", SF_LevelNum);

	// new inventory
	new_function("giveinventory", SF_GiveInventory);
	new_function("takeinventory", SF_TakeInventory);
	new_function("checkinventory", SF_CheckInventory);
	new_function("setweapon", SF_SetWeapon);

	new_function("ls", SF_Ls);	// execute Hexen type line special

	// Dummies - shut up warnings
	new_function("setcorona", SF_SetCorona);
}

