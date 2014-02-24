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
#include <string.h>
#include "cervixes.h"

aConn *_conn_first;

static int _cx_sockrecv(MSocket *m);
static int _cx_sockerr(MSocket *m);
static int _cx_sockaccept(MSocket *m);
static aConn *_aconn_new(void);

int init_network()
{
	MSocket *l;
	char *localaddr;

	_conn_first = (aConn *)NULL;

	if (lms_init((config->debugmode > 0) ? 1 : 0) != 0)
	{
		return(-1);
	}

	l = lms_socket_create(LMSTYPE_LISTEN4);
	if (!l) { fprintf(stdout, "lms_socket_create(): %s\n", strerror(errno)); return(-1); }
	localaddr = cxmconf("listen", "vhost");
	if (localaddr)
	{
	        snprintf(l->localhost, LMS_LEN_V4ADDR, "127.0.0.1");
	}
	l->localport = stringtoint(cxmconf("listen", "port"));
	if (lms_socket_ilisten(l) < 0) { fprintf(stdout, "lms_socket_ilisten(): %s\n", strerror(errno)); return(-1); }
	l->func_p = cx_sockrecv;
	l->func_a = cx_sockaccept;
	if (lms_mux_addfd(l, -1, 0) < 0) { fprintf(stdout, "lms_mux_addfd(): %s\n", strerror(errno)); return(-1); }

	return(0);
}

int _cx_sockrecv(MSocket *m)
{

}

int _cx_sockerr(MSocket *m)
{
	aConn *aptr;

	if (!m) { errno = EINVAL; return(-1); }

	aptr = (aConn *)m->appdata;

	/* fprintf(stdout, "[LMS] Disconnect from %s:%i\n", m->remotehost, m->remoteport); */
	return(lms_socket_destroy(m));
}

int _cx_sockaccept(MSocket *m)
{
	aConn *newa;
	MSocket *mptr;

	mptr = lms_socket_create(LMSTYPE_STREAM4);
	newa = _aconn_new();
	if (!mptr || !newa)
	{
		if (mptr) { free(mptr); }
		if (newa) { free(newa); }
		return(-1);
	}

	lms_socket_iaccept(m, mptr);
	m->func_e = _cx_sockerr;
	newa->state = CONN_UNREGISTERED;
	newa->flags = 0;
	newa->m = mptr;
	newa->us = (void *)NULL;
	m->appdata = (void *)newa;

	return(0);
}

aConn *_aconn_new()
{
	aConn *n;
	aConn *aptr;

	n = (aConn *)malloc(sizeof(aConn));
	if (!n) { return((aConn *)NULL); }
	memset(n, 0, sizeof(aConn));

	if (!_conn_first)
	{
		_conn_first = n;
		n->prev = (aConn *)NULL;
		n->next = (aConn *)NULL;
		return(n);
	}
	aptr = _conn_first;
	while (aptr->next)
	{
		aptr = aptr->next;
	}
	aptr->next = n;
	n->next = (aConn *)NULL;
	n->prev = aptr;

	return(n);
}
