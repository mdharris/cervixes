/*
** Copyright (c) 2012, Matt Harris
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
** 
**    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
**    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
**    materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
** THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
** 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "cervixes.h"

struct _Timeout
{
	time_t set;
	time_t run;
	uint8_t action;
	void *data;

	struct _Timeout *prev;
	struct _Timeout *next;
};
typedef struct _Timeout To;

To *_to_first;

static void _to_exec(To *tptr);
static void _to_del(To *tptr);

int init_to()
{
	_to_first = (To *)NULL;

	return(0);
}

int to_chk()
{
	To *md;
	int nran;

	nran = 0;
	md = _to_first;
	while (md)
	{
		if (time(NULL) >= md->run)
		{
			_to_exec(md);
			_to_del(md);
			nran++;
		}
		md = md->next;
	}

	return(nran);
}

int to_add(uint8_t action, void *aptr, time_t cuando)
{
	To *tptr;
	To *new;

	if (action >= TOACT_INVALID)
	{
		errno = EINVAL;
		return(-1);
	}

	new = (To *)malloc(sizeof(To));
	if (!new) { return(-1); }
	memset(new, 0, sizeof(To));
	new->prev = (To *)NULL;
	new->next = (To *)NULL;

	new->set = time(NULL);
	new->run = cuando;
	new->action = action;
	new->data = aptr;

	if (!_to_first)
	{
		_to_first = new;
		return(0);
	}
	tptr = _to_first;
	while (tptr->next)
	{
		tptr = tptr->next;
	}
	tptr->next = new;
	new->prev = tptr;

	return(0);
}

int to_del(uint8_t action, void *aptr)
{
	To *tptr;
	To *tmp_tptr;
	int nfound;

	if (!_to_first)
	{
		return(0);
	}

	nfound = 0;
	tptr = _to_first;
	while (tptr)
	{
		tmp_tptr = tptr->next;

		if ((tptr->action == action) && (tptr->data == aptr))
		{
			_to_del(tptr);
			nfound++;
		}

		if (tmp_tptr)
		{
			tptr = tmp_tptr;
		}
		else
		{
			break;
		}
	}

	return(nfound);
}

void _to_exec(To *tptr)
{

}

void _to_del(To *tptr)
{
	if (_to_first == tptr)
	{
		if (tptr->next)
			_to_first = tptr->next;
		}
		else
		{
			_to_first = (To *)NULL;
		}
	}

	if (tptr->next && tptr->prev)
	{
		tptr->next->prev = tptr->prev;
		tptr->prev->next = tptr->next;
	}
	if (tptr->next && !tptr->prev)
	{
		tptr->next->prev = (To *)NULL;
	}
	if (tptr->prev && !tptr->next)
	{
		tptr->prev->next = (to *)NULL;
	}
}
