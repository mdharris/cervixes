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
#include <msocket.h>
#include "cervixes.h"

static int _cx_sockerr(MSocket *m);
static int _cx_sockaccept(MSocket *m);

int init_network()
{
	MSocket *l;

	event_set_log_callback(levlog);
	cx_base = event_base_new();
	if (lms_init((config->debugmode > 0) ? 1 : 0) != 0)
	{
		return(-1);
	}

	l = lms_socket_create(LMSTYPE_LISTEN4);
	if (!l) { fprintf(stdout, "lms_socket_create(): %s\n", strerror(errno)); return(-1); }
	/* if you want to enable vhosting support, this would be set.
        snprintf(l->localhost, LMS_LEN_V4ADDR, "127.0.0.1");
	*/
	l->localport = stringtoint(cxconf("myport"));
	if (lms_socket_ilisten(l) < 0) { fprintf(stdout, "lms_socket_ilisten(): %s\n", strerror(errno)); return(-1); }
	l->func_p = cx_sock;
	l->func_a = cx_sockaccept;
	if (lms_mux_addfd(l, -1, 0) < 0) { fprintf(stdout, "lms_mux_addfd(): %s\n", strerror(errno)); return(-1); }

	return(0);
}

int _cx_sockerr(MSocket *m)
{
	if (!m) { errno = EINVAL; return(-1); }

	/* fprintf(stdout, "[LMS] Disconnect from %s:%i\n", m->remotehost, m->remoteport); */
	return(lms_socket_destroy(m));
}

int _cx_sockaccept(MSocket *m)
{
	m->func_e = _cx_sockerr;
	return(0);
}
