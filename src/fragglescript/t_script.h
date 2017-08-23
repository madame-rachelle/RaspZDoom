// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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

#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__

#include "p_setup.h"
#include "t_parse.h"
#include "t_func.h"
#include "t_oper.h"
#include "t_prepro.h"
#include "t_spec.h"
#include "t_vari.h"

extern bool HasScripts;

enum waittype_e
{
    wt_none,        // not waiting
	wt_delay,       // wait for a set amount of time
	wt_tagwait,     // wait for sector to stop moving
	wt_scriptwait,  // wait for script to finish
	wt_scriptwaitpre, // haleyjd - wait for script to start
};

class DRunningScript : public DObject
{
	DECLARE_CLASS(DRunningScript, DObject)
	HAS_OBJECT_POINTERS

public:
	DRunningScript() {}
	script_t *script;
	
	// where we are
	char *savepoint;
	
	int wait_type;
	int wait_data;  // data for wait: tagnum, counter, script number etc
	
	// saved variables
	svariable_t *variables[VARIABLESLOTS];
	
	DRunningScript *prev, *next;  // for chain
	AActor *trigger;
};

void T_Init();
void T_ClearScripts();
void T_RunScript(int n,AActor *);
void T_PreprocessScripts();
void T_DelayedScripts();
AActor *MobjForSvalue(svalue_t svalue);

        // console commands
void T_Dump();
void T_ConsRun();

extern script_t levelscript;

// savegame related stuff
void T_SerializeScripts(FArchive & ar);
void T_FixPointers(DObject *,DObject *);

void T_LoadLevelInfo(MapData * map);
void T_PrepareSpawnThing();
void T_RegisterSpawnThing(AActor * );
extern TArray<DActorPointer*> SpawnedThings;

extern DRunningScript runningscripts;        // t_script.c

void clear_runningscripts();		      // t_script.c

void FS_EmulateCmd(char * string);

#endif

