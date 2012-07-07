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

#ifndef _INCLUDED_CERVIXES_H
# define _INCLUDED_CERVIXES_H

/* defines */

#define NICKLEN		16
#define SERVERLEN	64

#define UMODE_ADMIN	0x001
#define UMODE_INVISIBLE	0x002
#define UMODE_OPER	0x004
#define UMODE_WALLOPS	0x008

#define CAP_CAP		0x00001
#define CAP_QS		0x00002
#define CAP_EX		0x00004
#define CAP_CHW		0x00008
#define CAP_IE		0x00010
#define CAP_EOB		0x00020
#define CAP_KLN		0x00040
#define CAP_GLN		0x00080
#define CAP_TS6		0x00100
#define CAP_ZIP		0x00200
#define CAP_ENC		0x00400
#define CAP_KNOCK	0x00800
#define CAP_TB		0x01000
#define CAP_UNKLN	0x02000
#define CAP_HOPS	0x04000
#define CAP_CLUSTER	0x08000
#define CAP_ENCAP	0x10000
#define CAP_TBURST	0x20000
#define CAP_LAST	CAP_TBURST
#define CERVIXES_CAPS	(CAP_CAP|CAP_QS|CAP_EX|CAP_IE|CAP_EOB|CAP_KLN|CAP_GLN|CAP_TS6|CAP_UNKLN|CAP_HOPS|CAP_CLUSTER|CAP_ENCAP)

/* structs */

struct _Server
{
	uint32_t flags;
	uint32_t caps;
	time_t ts;
	char id[4];
	char name[SERVERLEN + 1];
	uint32_t nclients;

	struct _Server *prev;
	struct _Server *next;
};
typedef struct _Server IRCServer;

struct _User
{
	uint32_t flags;
	uint32_t umodes;
	time_t ts;
	IRCServer *s;
	char id[?];
	char nick[NICKLEN + 1];

	struct _User *prev;
	struct _User *next;
};
typedef struct _User IRCUser;

struct _Nick
{
	uint32_t flags;
	IRCUser *l;

	struct _Nick *prev;
	struct _Nick *next;
};
typedef struct _Nick SVCNick;

struct _Memo
{
	uint16_t flags;
	time_t sent;
	time_t read;
	SVCNick *from;
	SVCNick *to;
	uint32_t textlen;
	char *text;

	struct _Memo *prev;
	struct _Memo *next;
};
typedef struct _Memo SVCMemo;

/* macros */
#define IsOper(x)		((x)->umodes & UMODE_OPER || (x)->umodes & UMODE_ADMIN)
#define IsAdmin(x)		((x)->umodes & UMODE_ADMIN)
#define IsInvisible(x)		((x)->umodes & UMODE_INVISIBLE)
#define IsWallops(x)		((x)->umodes & UMODE_WALLOPS)
#define SetOper(x)		((x)->umodes |= UMODE_OPER)
#define SetAdmin(x)		((x)->umodes |= UMODE_ADMIN)
#define SetInvisible(x)		((x)->umodes |= UMODE_INVISIBLE)
#define SetWallops(x)		((x)->umodes |= UMODE_WALLOPS)
#define ClearOper(x)		((x)->umodes &= ~UMODE_OPER)
#define ClearAdmin(x)		((x)->umodes &= ~UMODE_ADMIN)
#define ClearInvisible(x)	((x)->umodes &= ~UMODE_INVISIBLE)
#define ClearWallops(x)		((x)->umodes &= ~UMODE_WALLOPS)

/* dl.c */

/* irc.c */

/* main.c */

/* version.c */
extern char *cervixes_version(void);

#endif /* _INCLUDED_CERVIXES_H */
