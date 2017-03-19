/*
** gl_cycler.cpp
** Implements the cycler for dynamic lights and texture shaders.
**
**---------------------------------------------------------------------------
** Copyright 2003 Timothy Stump
** Copyright 2006 Christoph Oelckers
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

#include <math.h>
#include "serializer.h"
#include "gl/utility/gl_cycler.h"

//==========================================================================
//
// This will never be called with a null-def, so don't bother with that case.
//
//==========================================================================

FSerializer &Serialize(FSerializer &arc, const char *key, FCycler &c, FCycler *def)
{
	if (arc.BeginObject(key))
	{
		arc("start", c.m_start, def->m_start)
			("end", c.m_end, def->m_end)
			("current", c.m_current, def->m_current)
			("time", c.m_time, def->m_time)
			("cycle", c.m_cycle, def->m_cycle)
			("increment", c.m_increment, def->m_increment)
			("shouldcycle", c.m_shouldCycle, def->m_shouldCycle)
			.Enum("type", c.m_cycleType)
			.EndObject();
	}
	return arc;
}

//==========================================================================
//
//
//
//==========================================================================

FCycler::FCycler()
{
	m_cycle = 0.f;
	m_cycleType = CYCLE_Linear;
	m_shouldCycle = false;
	m_start = m_current = 0.f;
	m_end = 0.f;
	m_increment = true;
}


//==========================================================================
//
//
//
//==========================================================================

void FCycler::SetParams(float start, float end, float cycle, bool update)
{
	if (!update || cycle != m_cycle)
	{
		m_cycle = cycle;
		m_time = 0.f;
		m_increment = true;
		m_current = start;
	}
	else
	{
		// When updating and keeping the same cycle, scale the current light size to the new dimensions.
		float fact = (m_current - m_start) / (m_end - m_start);
		m_current = start + fact *(end - start);
	}
	m_start = start;
	m_end = end;
}


//==========================================================================
//
//
//
//==========================================================================

void FCycler::Update(float diff)
{
	float mult, angle;
	float step = m_end - m_start;
	
	if (!m_shouldCycle)
	{
		return;
	}
	
	m_time += diff;
	if (m_time >= m_cycle)
	{
		m_time = m_cycle;
	}
	
	mult = m_time / m_cycle;
	
	switch (m_cycleType)
	{
	case CYCLE_Linear:
		if (m_increment)
		{
			m_current = m_start + (step * mult);
		}
		else
		{
			m_current = m_end - (step * mult);
		}
		break;
	case CYCLE_Sin:
		angle = float(M_PI * 2.f * mult);
		mult = sinf(angle);
		mult = (mult + 1.f) / 2.f;
		m_current = m_start + (step * mult);
		break;
	case CYCLE_Cos:
		angle = float(M_PI * 2.f * mult);
		mult = cosf(angle);
		mult = (mult + 1.f) / 2.f;
		m_current = m_start + (step * mult);
		break;
	case CYCLE_SawTooth:
		m_current = m_start + (step * mult);
		break;
	case CYCLE_Square:
		if (m_increment)
		{
			m_current = m_start;
		}
		else
		{
			m_current = m_end;
		}
		break;
	}
	
	if (m_time == m_cycle)
	{
		m_time = 0.f;
		m_increment = !m_increment;
	}
}

