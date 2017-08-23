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
//
// Variables.
//
// Variable code: create new variables, look up variables, get value,
// set value
//
// variables are stored inside the individual scripts, to allow for
// 'local' and 'global' variables. This way, individual scripts cannot
// access variables in other scripts. However, 'global' variables can
// be made which can be accessed by all scripts. These are stored inside
// a dedicated script_t which exists only to hold all of these global
// variables.
//
// functions are also stored as variables, these are kept in the global
// script so they can be accessed by all scripts. function variables
// cannot be set or changed inside the scripts themselves.
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

/* includes ************************/
#include "t_script.h"

IMPLEMENT_POINTY_CLASS(DActorPointer)
	DECLARE_POINTER(actor)
END_POINTERS

// the global script just holds all
// the global variables
script_t global_script;

// the hub script holds all the variables
// shared between levels in a hub
script_t hub_script;


// initialise the global script: clear all the variables

void init_variables()
{
	int i;
	
	// globalscript is the root script
	global_script.parent = NULL;
	// hub script is the next level down
	hub_script.parent = &global_script;
	
	for(i=0; i<VARIABLESLOTS; i++)
		global_script.variables[i] = hub_script.variables[i] = NULL;
	
	// any hardcoded global variables can be added here
}

void T_ClearHubScript()
{
	int i;
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		while(hub_script.variables[i])
		{
			svariable_t *next = hub_script.variables[i]->next;
			delete hub_script.variables[i];
			hub_script.variables[i] = next;
		}
    }
}

// find_variable checks through the current script, level script
// and global script to try to find the variable of the name wanted

svariable_t *find_variable(char *name)
{
	svariable_t *var;
	script_t *current;
	
	current = current_script;
	
	while(current)
    {
		// check this script
		if((var = variableforname(current, name)))
			return var;
		current = current->parent;    // try the parent of this one
    }
	
	return NULL;    // no variable
}

// create a new variable in a particular script.
// returns a pointer to the new variable.

svariable_t *new_variable(script_t *script, char *name, int vtype)
{
	int n;
	svariable_t *newvar;
	
	newvar = new svariable_t(name);
	newvar->type = vtype;
	
	if(vtype == svt_string)
	{
		// 256 bytes for string
		newvar->value.s = new char[256];
		newvar->value.s[0] = 0;
	}
	else if (vtype != svt_mobj)
	{
		newvar->value.i = 0;
	}
	else
	{
		newvar->value.acp = new DActorPointer;
	}
	
	n = variable_hash(name);
	newvar->next = script->variables[n];
	script->variables[n] = newvar;
	
	return script->variables[n];
}

// search a particular script for a variable, which
// is returned if it exists

svariable_t *variableforname(script_t *script, char *name)
{
	int n;
	svariable_t *current;
	
	n = variable_hash(name);
	
	current = script->variables[n];
	
	while(current)
    {
		if(!strcmp(name, current->name))        // found it?
			return current;         
		current = current->next;        // check next in chain
    }
	
	return NULL;
}

// free all the variables in a given script
void clear_variables(script_t *script)
{
	int i;
	svariable_t *current, *next;
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		current = script->variables[i];
		
		// go thru this chain
		while(current)
		{
			// labels are added before variables, during
			// preprocessing, so will be at the end of the chain
			// we can be sure there are no more variables to free
			if(current->Type() == svt_label) break;
			
			next = current->next; // save for after freeing
			
			// if a string, free string data
			delete current;
			current = next; // go to next in chain
		}
		// start of labels or NULL
		script->variables[i] = current;
    }
}

// returns an svalue_t holding the current
// value of a particular variable.

svalue_t getvariablevalue(svariable_t *v)
{
	svalue_t returnvar;
	
	if(!v) return nullvar;
	
	else if(v->type == svt_pInt)
    {
		returnvar.type = svt_int;
		returnvar.value.i = *v->value.pI;
    }
	else if(v->type == svt_pMobj)
    {
		returnvar.type = svt_mobj;
		returnvar.value.mobj = *v->value.pMobj;
    }
	else if (v->type == svt_mobj)
	{
		returnvar.type = v->type;
		returnvar.value.mobj = v->value.acp->actor;
	}
	else
    {
		returnvar.type = v->type;
		// copy the value
		returnvar.value.i = v->value.i;
    }
	
	return returnvar;
}

// set a variable to a value from an svalue_t
// haleyjd: significant reformatting in 8-17

void setvariablevalue(svariable_t *v, svalue_t newvalue)
{
	if(killscript) return;  // protect the variables when killing script
	
	if(!v) return;
	
	if(v->type == svt_const)
    {
		// const adapts to the value it is set to
		v->ChangeType(newvalue.type);
		
		// alloc memory for string
		if(v->type == svt_string)   // static incase a global_script var
			v->value.s = new char[256];
    }
	
	if(v->type == svt_int)
		v->value.i = intvalue(newvalue);
	
	if(v->type == svt_string)
		strcpy(v->value.s, stringvalue(newvalue));
	
	if(v->type == svt_fixed)
		v->value.fixed = fixedvalue(newvalue);
	
	if(v->type == svt_mobj)
		v->value.acp->actor = MobjForSvalue(newvalue);
	
	if(v->type == svt_pInt)
		*v->value.pI = intvalue(newvalue);
	
	if(v->type == svt_pMobj)
		*v->value.pMobj = MobjForSvalue(newvalue);
	
	if(v->type == svt_function)
		script_error("attempt to set function to a value\n");
}

svariable_t *add_game_int(char *name, int *var)
{
	svariable_t* newvar;
	newvar = new_variable(&global_script, name, svt_pInt);
	newvar->value.pI = var;
	
	return newvar;
}

svariable_t *add_game_mobj(char *name, AActor **mo)
{
	svariable_t* newvar;
	newvar = new_variable(&global_script, name, svt_pMobj);
	newvar->value.pMobj = mo;
	
	return newvar;
}


/********************************
FUNCTIONS
********************************/
// functions are really just variables
// of type svt_function. there are two
// functions to control functions (heh)

// new_function: just creates a new variable
//      of type svt_function. give it the
//      handler function to be called, and it
//      will be stored as a pointer appropriately.

// evaluate_function: once parse.c is pretty
//      sure it has a function to run it calls
//      this. evaluate_function makes sure that
//      it is a function call first, then evaluates all
//      the arguments given to the function.
//      these are built into an argc/argv-style
//      list. the function 'handler' is then called.

// the basic handler functions are in func.c

int t_argc;                     // number of arguments
svalue_t *t_argv;               // arguments
svalue_t t_return;              // returned value
char * t_func;					// name of current function

svalue_t evaluate_function(int start, int stop)
{
	svariable_t *func = NULL;
	int startpoint, endpoint;
	
	// the arguments need to be built locally in case of
	// function returns as function arguments eg
	// print("here is a random number: ", rnd() );
	
	int argc;
	svalue_t argv[MAXARGS];
	
	if(tokentype[start] != function || tokentype[stop] != operator_
		|| tokens[stop][0] != ')' )
		script_error("misplaced closing paren\n");
	
	// all the functions are stored in the global script
	else if( !(func = variableforname(&global_script, tokens[start]))  )
		script_error("no such function: '%s'\n",tokens[start]);
	
	else if(func->Type() != svt_function)
		script_error("'%s' not a function\n", tokens[start]);
	
	if(killscript) return nullvar; // one of the above errors occurred
	
	// build the argument list
	// use a C command-line style system rather than
	// a system using a fixed length list
	
	argc = 0;
	endpoint = start + 2;   // ignore the function name and first bracket
	
	while(endpoint < stop)
    {
		startpoint = endpoint;
		endpoint = find_operator(startpoint, stop-1, ",");
		
		// check for -1: no more ','s 
		if(endpoint == -1)
		{               // evaluate the last expression
			endpoint = stop;
		}
		if(endpoint-1 < startpoint)
			break;
		
		argv[argc] = evaluate_expression(startpoint, endpoint-1);
		endpoint++;    // skip the ','
		argc++;
    }
	
	// store the arguments in the global arglist
	t_argc = argc;
	t_argv = argv;
	
	if(killscript) return nullvar;
	
	// haleyjd: return values can propagate to void functions, so
	// t_return needs to be cleared now
	
	t_return.type = svt_int;
	t_return.value.i = 0;
	
	// now run the function
	func->Value().handler();
	
	// return the returned value
	return t_return;
}

// structure dot (.) operator
// there are not really any structs in FraggleScript, it's
// just a different way of calling a function that looks
// nicer. ie
//      a.b()  = a.b   =  b(a)
//      a.b(c) = b(a,c)

// this function is just based on the one above

svalue_t OPstructure(int start, int n, int stop)
{
	svariable_t *func = NULL;
	
	// the arguments need to be built locally in case of
	// function returns as function arguments eg
	// print("here is a random number: ", rnd() );
	
	int argc;
	svalue_t argv[MAXARGS];
	
	// all the functions are stored in the global script
	if( !(func = variableforname(&global_script, tokens[n+1]))  )
		script_error("no such function: '%s'\n",tokens[n+1]);
	
	else if(func->Type() != svt_function)
		script_error("'%s' not a function\n", tokens[n+1]);
	
	if(killscript) return nullvar; // one of the above errors occurred
	
	// build the argument list
	
	// add the left part as first arg
	
	argv[0] = evaluate_expression(start, n-1);
	argc = 1; // start on second argv
	
	if(stop != n+1)         // can be a.b not a.b()
    {
		int startpoint, endpoint;
		
		// ignore the function name and first bracket
		endpoint = n + 3;
		
		while(endpoint < stop)
		{
			startpoint = endpoint;
			endpoint = find_operator(startpoint, stop-1, ",");
			
			// check for -1: no more ','s 
			if(endpoint == -1)
			{               // evaluate the last expression
				endpoint = stop;
			}
			if(endpoint-1 < startpoint)
				break;
			
			argv[argc] = evaluate_expression(startpoint, endpoint-1);
			endpoint++;    // skip the ','
			argc++;
		}
    }
	
	// store the arguments in the global arglist
	t_argc = argc;
	t_argv = argv;
	t_func = func->name;
	
	if(killscript) return nullvar;
	
	// haleyjd: return values can propagate to void functions, so
	// t_return needs to be cleared now
	
	t_return.type = svt_int;
	t_return.value.i = 0;
	
	// now run the function
	func->Value().handler();
	
	// return the returned value
	return t_return;
}


// create a new function. returns the function number

svariable_t *new_function(char *name, void (*handler)() )
{
	svariable_t *newvar;
	
	// create the new variable for the function
	// add to the global script
	
	newvar = new_variable(&global_script, name, svt_function);
	
	// add neccesary info
	
	newvar->value.handler = handler;
	
	return newvar;
}

