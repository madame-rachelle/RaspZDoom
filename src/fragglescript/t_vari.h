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

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#define VARIABLESLOTS 16


// variable types

enum
{
  svt_string,
  svt_int,
  svt_mobj,         // a map object
  svt_function,     // functions are stored as variables
  svt_label,        // labels for goto calls are variables
  svt_const,        // const
  svt_fixed,        // haleyjd: fixed-point int - 8-17 std
  svt_arraysweredeleted,
  svt_pInt,         // pointer to game int
  svt_pMobj,        // pointer to game mobj
  svt_parraysweredeleted,
};


// hash the variables for speed: this is the hashkey

#define variable_hash(n)                \
              (n[0]? (   ( (n)[0] + (n)[1] +   \
                   ((n)[1] ? (n)[2] +   \
				   ((n)[2] ? (n)[3]  : 0) : 0) ) % VARIABLESLOTS ) :0)


// It is impractical to have svariable_t being declared as a DObject so
// I have to use an intermediate class to make this subject to
// automatic pointer cleanup.

class DActorPointer : public DObject
{
	DECLARE_CLASS(DActorPointer, DObject)
	HAS_OBJECT_POINTERS

public:

	AActor * actor;

	DActorPointer()
	{
		actor=NULL;
	}

	void Serialize(FArchive & ar)
	{
		Super::Serialize(ar);
		ar << actor;
	}
};
     // svariable_t
struct svariable_t
{
	char *name;
	svariable_t *next;       // for hashing

private:
	signed char type;       // vt_string or vt_int: same as in svalue_t

	union value_t
	{
		char *s;
		long i;
		//AActor *mobj;
		DActorPointer * acp;
		fixed_t fixed;          // haleyjd: fixed-point
		sfarray_t *a;           // haleyjd 05/27: arrays
		
		char **pS;              // pointer to game string
		int *pI;                // pointer to game int
		AActor **pMobj;         // pointer to game obj
		fixed_t *pFixed;        // haleyjd: fixed ptr
		sfarray_t **pA;         // haleyjd 05/27: arrays
		
		void (*handler)();      // for functions
		char *labelptr;         // for labels
	} value;

public:

	svariable_t(const char * _name=NULL)
	{
		name=_name? strdup(_name):NULL;
		type=svt_int;
		value.i=0;
		next=NULL;
	}

	~svariable_t()
	{
		if (name) free(name);
		if (type==svt_string && value.s) free(value.s);
		else if (type==svt_mobj) value.acp->Destroy();
	}

	int Type() const
	{
		return type;
	}

	void ChangeType(int newtype)
	{
		if (type==svt_mobj && newtype!=svt_mobj)
		{
			value.acp->Destroy();
		}
		else if (type!=svt_mobj && newtype==svt_mobj)
		{
			value.acp = new DActorPointer;
		}
		type = newtype;
	}

	const value_t & Value() const
	{
		return value;
	}

	friend svariable_t *new_variable(script_t *script, char *name, int vtype);
	friend svalue_t getvariablevalue(svariable_t *v);
	friend void setvariablevalue(svariable_t *v, svalue_t newvalue);
	friend svariable_t *add_game_int(char *name, int *var);
	friend svariable_t *add_game_mobj(char *name, AActor **mo);
	friend svariable_t *new_function(char *name, void (*handler)() );
	friend svariable_t *new_label(char *labelptr);
	friend FArchive & operator <<(FArchive & ar, svariable_t & var);


};



// variables

void T_ClearHubScript();

void init_variables();
svariable_t *find_variable(char *name);
svariable_t *variableforname(script_t *script, char *name);
void clear_variables(script_t *script);
svariable_t *new_variable(script_t *script, char *name, int vtype);
svalue_t getvariablevalue(svariable_t *v);
void setvariablevalue(svariable_t *v, svalue_t newvalue);

// functions

svariable_t *add_game_int(char *name, int *var);
svariable_t *add_game_mobj(char *name, AActor **mo);
svalue_t evaluate_function(int start, int stop);   // actually run a function
svariable_t *new_function(char *name, void (*handler)() );

// arguments to handler functions

#define MAXARGS 128
extern int t_argc;
extern svalue_t *t_argv;
extern svalue_t t_return;
extern char * t_func;

#endif

