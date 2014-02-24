/*
** Copyright (c) 2012-2014, Matt Harris
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
#include <string.h>
#include "cervixes.h"

struct _Capab
{
	uint32_t i;
	char name[32];

	struct _Capab *next;
	struct _Capab *prev;
};
typedef struct _Capab IRCCap;

static IRCCap *_cap_first;
static IRCUser *_user_first;
static IRCServer *_server_first;

static void _cap_rem(IRCCap *cptr);

int init_irc()
{
	/*
         * capsstr[CAP_CAP] = "";
         * capsstr[CAP_QS] = "QS";
         * capsstr[CAP_EX] = "EX";
         * capsstr[CAP_CHW] = "CHW";
         * capsstr[CAP_IE] = "IE";
         * capsstr[CAP_EOB] = "EOB";
         * capsstr[CAP_KLN] = "KLN";
         * capsstr[CAP_GLN] = "GLN";
         * capsstr[CAP_TS6] = "TS6";
         * capsstr[CAP_ZIP] = "ZIP";
         * capsstr[CAP_ENC] = "ENC";
         * capsstr[CAP_KNOCK] = "KNOCK";
         * capsstr[CAP_TB] = "TB";
         * capsstr[CAP_UNKLN] = "UNKLN";
         * capsstr[CAP_HOPS] = "HOPS";
         * capsstr[CAP_CLUSTER] = "CLUSTER";
         * capsstr[CAP_ENCAP] = "ENCAP";
         * capsstr[CAP_TBURST] = "TBURST";
	 */
	_cap_first = (IRCCap *)NULL;
	_user_first = (IRCUser *)NULL;
	_server_first = (IRCServer *)NULL;

	return(0);
}

int cap_add(uint32_t idx, char *txt)
{
	IRCCap *new;
	IRCCap *cptr;

	if (!txt)
	{
		return(-1);
	}

	if (_cap_first)
	{
		cptr = _cap_first;
		while (cptr)
		{
			if ((cptr->i == idx) || (strncmp(txt, cptr->name, 32) == 0))
			{
				return(0);
			}
			cptr = cptr->next;
		}
	}

	new = (IRCCap *)malloc(sizeof(IRCCap));
	if (!new) { return(-1); }
	memset(new, 0, sizeof(IRCCap));

	if (!_cap_first)
	{
		_cap_first = new;
	}
	else
	{
		cptr = _cap_first;
		while (cptr->next)
		{
			cptr = cptr->next;
		}
		cptr->next = new;
		new->prev = cptr;
		new->next = (IRCCap *)NULL;
	}

	return(0);
}

int cap_rem(uint32_t idx)
{
	IRCCap *cptr;
	IRCCap *tcptr;

	if (!_cap_first)
	{
		return(-1);
	}
	cptr = _cap_first;
	while (cptr)
	{
		tcptr = cptr->next;

		if (cptr->i == idx)
		{
			_cap_rem(cptr);
		}

		cptr = tcptr;
	}

	return(0);
}

void _cap_rem(IRCCap *cptr)
{
	if (_cap_first == cptr)
	{
		if (cptr->next)
		{
			_cap_first = cptr->next;
		}
		else
		{
			_cap_first = (IRCCap *)NULL;
		}
	}

	if (cptr->next && cptr->prev)
	{
		cptr->next->prev = cptr->prev;
		cptr->prev->next = cptr->next;
	}
	if (cptr->next && !cptr->prev)
	{
		cptr->next->prev = (IRCCap *)NULL;
	}
	if (cptr->prev && !cptr->next)
	{
		cptr->prev->next = (IRCCap *)NULL;
	}
}

char *cap_getstr(uint32_t list)
{
	char *capstr;
	char tmpstr[512];

	capstr = (char *)malloc(512);
	if (!capstr)
	{
		return((char *)NULL);
	}
	memset(capstr, 0, 512);

	return(capstr);
}

int user_add(char *uid, char *nick, char *user, char *host, char *realname, IRCServer *server)
{

}

int user_rem(char *uid, char *nick)
{
	IRCUser *uptr;

	if (!uid && !nick)
	{
		errno = EINVAL;
		return(-1);
	}

	if (uid)
	{
		uptr = finduser_uid(uid);
	}
	else
	{
		uptr = finduser_nick(nick);
	}
}

int server_add(char *sid, char *servername, char *description, IRCServer *uplink)
{

}

int server_rem(char *sid, char *servername)
{
	IRCServer *sptr;

	if (!sid && !servername)
	{
		errno = EINVAL;
		return(-1);
	}

	if (sid)
	{
		sptr = findserver_sid(sid);
	}
	else
	{
		sptr = findserver_name(servername);
	}
}
