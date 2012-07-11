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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "cervixes.h"
#include "inilib/ini.h"

struct _ConfEnt
{
	uint16_t idx;
	char name[128];
	char sect[128];
	char data[758];		/* 1kb each struct */

	struct _ConfEnt *next;
	struct _ConfEnt *prev;
};
typedef struct _ConfEnt Config;

static int _hdlr_config(void *pc, const char *s, const char *n, const char *v);

static Config *_conf_first;

int init_conf(char *file)
{
	_conf_global_idx = 0;
	_conf_first = (Config *)NULL;

	if (ini_parse(file, hdlr_config, (void *)NULL) != 0)
	{
		return(-1);
	}

	return(0);
}

void clear_conf()
{
	Config *cptr;
	Config *tmpptr;

	if (!_conf_first) { return; }
	cptr = _conf_first;
	while (cptr)
	{
		tmpptr = cptr->next;
		if (cptr->next)
		{
			cptr->next->prev = (Config *)NULL;
		}
		free(cptr);
		cptr = tmpptr;
	}
	_conf_first = (Config *)NULL;
}

char *cxconf(char *search)
{
	register Config *cptr;

	if (!_conf_first) { return((char *)NULL); }
	cptr = _conf_first;
	while (cptr->next)
	{
		if (strncmp(cptr->name, search, 128) == 0)
		{
			return(cptr->data);
		}
		cptr = cptr->next;
	}

	return((char *)NULL);
}

char *cxmconf(char *ssearch, char *nsearch)
{
	register Config *cptr;

	if (!_conf_first) { return((char *)NULL); }
	cptr = _conf_first;
	while (cptr->next)
	{
		if ((strncmp(cptr->sect, ssearch, 128) == 0) && (strncmp(cptr->name, nsearch, 128) == 0))
		{
			return(cptr->data);
		}
		cptr = cptr->next;
	}

	return((char *)NULL);
}

int _hdlr_config(void *pc, const char *s, const char *n, const char *v)
{
	Config *n;
	Config *cptr;

	cptr = (Config *)NULL;

	if (cxmconf(s, n))
	{
		/* don't bother with duplicate entries */
		return(0);
	}

	n = (Config *)malloc(sizeof(Config));
	if (!n) { return(0); }
	memset(n, 0, sizeof(Config));

	_conf_global_idx++;
	n->idx = _conf_global_idx;
	n->next = (Config *)NULL;
	n->prev = (Config *)NULL;
	memset(n->name, 0, 128);
	strncpy(n->name, n, 128);
	memset(n->sect, 0, 128);
	strncpy(n->sect, s, 128);
	memset(n->data, 0, 758);
	strncpy(n->data, v, 758);

	if (!_conf_first)
	{
		_conf_first = n;
	}
	else
	{
		cptr = _conf_first;
		while (cptr->next)
		{
			cptr = cptr->next;
		}
		cptr->next = n;
		n->prev = cptr;
	}

	return(1);
}
