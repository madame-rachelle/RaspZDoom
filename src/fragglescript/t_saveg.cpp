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
/*
 * DESCRIPTION:
 *      Archiving: SaveGame I/O.
 *
 *
 *-----------------------------------------------------------------------------*/

/*
FraggleScript is from SMMU which is under the GPL. Technically, therefore, 
combining the FraggleScript code with the non-free ZDoom code is a violation of the GPL.

As this may be a problem for you, I hereby grant an exception to my copyright on the 
SMMU source (including FraggleScript). You may use any code from SMMU in GZDoom, provided that:

    * For any binary release of the port, the source code is also made available.
    * The copyright notice is kept on any file containing my code.
*/

#include "t_script.h"
#include "farchive.h"
#include "doomstat.h"
#include "i_system.h"

//==========================================================================
//
// Archive one script variable
//
//==========================================================================

FArchive & operator <<(FArchive & ar, svariable_t & var)
{
	if (ar.IsStoring())	ar.WriteName(var.name);
	else 
	{
		var.name = strdup(ar.ReadName());
		var.ChangeType(svt_int);	// Just to clear old actor pointers in it!
	}
	ar << var.type;
	
	switch(var.type)        // store depending on type
	{
	case svt_string:
		if (ar.IsStoring()) ar.WriteName(var.value.s);
		else var.value.s = strdup(ar.ReadName());
		break;

	case svt_int:
		ar << var.value.i;
		break;

	case svt_mobj:
		ar << var.value.acp;
		break;

	case svt_fixed:
		ar << var.value.fixed;
		break;
	}

	return ar;
}


//==========================================================================
//
// save a script
//
// all we really need to do is save the variables
//
//==========================================================================

void T_ArchiveScript(FArchive & ar, script_t * script)
{
	short num_variables;
	int i;
	
	
	// count number of variables
	num_variables = 0;
	for(i=0; i<VARIABLESLOTS; i++)
    {
		svariable_t *sv = script->variables[i];
		while(sv && sv->Type() != svt_label)
		{
			num_variables++;
			sv = sv->next;
		}
    }
	
	ar << num_variables;

	// go thru hash chains, store each variable
	for(i=0; i<VARIABLESLOTS; i++)
    {
		// go thru this hashchain
		svariable_t *sv = script->variables[i];
		
		// once we get to a label there can be no more actual
		// variables in the list to store
		while(sv && sv->Type() != svt_label)
		{
			ar << *sv;
			sv = sv->next;
		}
    }
}

//==========================================================================
//
// load a script from a savegame
//
// all we really need to do is to set the variables
//
//==========================================================================

void T_UnArchiveScript(FArchive & ar, script_t * script)
{
	int i;
	short num_variables;
	
	// free all the variables in the current script first
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		svariable_t *sv = script->variables[i];
		
		while(sv && sv->Type() != svt_label)
		{
			svariable_t *next = sv->next;
			delete sv;
			sv = next;
		}
		script->variables[i] = sv;       // null or label
    }
	
	// now read the number of variables from the savegame file
	
	ar << num_variables;

	for(i=0; i<num_variables; i++)
    {
		svariable_t *sv = new svariable_t;
		int hashkey;
		
		ar << * sv;
		
		// link in the new variable
		hashkey = variable_hash(sv->name);
		sv->next = script->variables[hashkey];
		script->variables[hashkey] = sv;
    }
}


//==========================================================================
//
// save a given runningscript
//
//==========================================================================

void T_ArchiveRunningScript(FArchive & ar,DRunningScript *rs)
{
	int i;
	short num_variables;
	short scriptnum=rs->script->scriptnum;
	short savepoint=(short)(rs->savepoint - rs->script->data);
	
	ar << scriptnum << savepoint << rs->wait_type << rs->wait_data << rs->trigger;
	
	// count number of variables
	num_variables = 0;
	for(i=0; i<VARIABLESLOTS; i++)
    {
		svariable_t *sv = rs->variables[i];
		while(sv && sv->Type() != svt_label)
		{
			num_variables++;
			sv = sv->next;
		}
    }
	ar << num_variables;
	
	// store variables
	// go thru hash chains, store each variable
	for(i=0; i<VARIABLESLOTS; i++)
    {
		// go thru this hashchain
		svariable_t *sv = rs->variables[i];
		
		// once we get to a label there can be no more actual
		// variables in the list to store
		while(sv && sv->Type() != svt_label)
		{
			ar << *sv;
			sv = sv->next;
		}
    }
}

//==========================================================================
//
// get the next runningscript
//
//==========================================================================

DRunningScript *T_UnArchiveRunningScript(FArchive & ar)
{
	int i;
	short scriptnum;
	short savepoint;
	short num_variables;
	DRunningScript *rs;
	
	// create a new runningscript
	rs = new DRunningScript();
	
	ar << scriptnum << savepoint;
	rs->script = (scriptnum == -1) ? &levelscript : levelscript.children[scriptnum];// levelscript?
	rs->savepoint = rs->script->data + (savepoint); 	// read out offset from save
	ar << rs->wait_type << rs->wait_data << rs->trigger << num_variables;
	
	// read out the variables now (fun!)
	
	// start with basic script slots/labels
	
	for(i=0; i<VARIABLESLOTS; i++)
		rs->variables[i] = rs->script->variables[i];
	
	for(i=0; i<num_variables; i++)
    {
		svariable_t *sv = new svariable_t;
		int hashkey;
		
		// name
		ar << *sv;

		// link in the new variable
		hashkey = variable_hash(sv->name);
		sv->next = rs->variables[hashkey];
		rs->variables[hashkey] = sv;
    }
	return rs;
}

//==========================================================================
//
// archive all runningscripts in chain
//
//==========================================================================

void T_ArchiveRunningScripts(FArchive & ar)
{
	DRunningScript *rs;
	short num_runningscripts = 0;
	
	// count runningscripts
	for(rs = runningscripts.next; rs; rs = rs->next) num_runningscripts++;
	
	ar << num_runningscripts;

	// now archive them
	rs = runningscripts.next;
	while(rs)
    {
		T_ArchiveRunningScript(ar,rs);
		rs = rs->next;
    }
}

//==========================================================================
//
// restore all runningscripts
//
//==========================================================================

void T_UnArchiveRunningScripts(FArchive & ar)
{
	DRunningScript *rs;
	short num_runningscripts;
	int i;
	
	// remove all runningscripts first : may have been started
	// by levelscript on level load
	clear_runningscripts();
	ar << num_runningscripts;
	
	for(i=0; i<num_runningscripts; i++)
    {
		// get next runningscript
		rs = T_UnArchiveRunningScript(ar);
		
		// hook into chain
		rs->next = runningscripts.next;
		rs->prev = &runningscripts;
		rs->prev->next = rs;
		if(rs->next) rs->next->prev = rs;
    }
}

//==========================================================================
//
// 
//
//==========================================================================

void T_ArchiveSpawnedThings(FArchive & ar)
{
	int count = SpawnedThings.Size ();
	ar << count;
	if (ar.IsLoading()) SpawnedThings.Resize(count);
	for(int i=0;i<count;i++)
	{
		ar << SpawnedThings[i];
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void T_SerializeScripts(FArchive & ar)
{
	if(!HasScripts) return;
	
	if (ar.IsStoring())
	{
		T_ArchiveScript(ar, &levelscript);
		T_ArchiveScript(ar, &hub_script);
		T_ArchiveRunningScripts(ar);
		T_ArchiveSpawnedThings(ar);
	}
	else
	{
		T_UnArchiveScript(ar, &levelscript);
		T_UnArchiveScript(ar, &hub_script);
		T_UnArchiveRunningScripts(ar);
		T_ArchiveSpawnedThings(ar);
	}
}

