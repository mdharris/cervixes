/*
 * Copyright (c) 2008
 *      Matt Harris.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      "This product includes software developed by Matt Harris."
 * 4. Neither the name of the Mr. Harris nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 5. Any modifications and/or variations in the end-product which you are 
 *    distributing from the original source code are clearly noted in
 *    the standard end-user documentation distributed with any package
 *    containing this software in either source or binary form, as well
 *    as on any internet sites or media on which this software is included.
 *
 * THIS SOFTWARE IS PROVIDED BY Mr. Harris AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Mr. Harris OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "localconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#include <msocket.h>


/*
 * This sample program listens on 127.0.0.1 port 5202
 * It will print any data received on connections to that port to stdout.
 *
 */

static unsigned char quitting;
extern void pfunc(MSocket *m);
extern void afunc(MSocket *m);
extern int efunc(MSocket *m);
static void cntrlc_hdlr(int sig);


int main(int argc, char **argv)
{
	MSocket *l;

	quitting = 0;
	signal(SIGINT, cntrlc_hdlr);

	fprintf(stdout, "libmsocket example code: libmsocket version %s\n", lms_version());

	if (lms_init(1) < 0) { return(1); }

	l = lms_socket_create(LMSTYPE_LISTEN4);
	if (!l) { fprintf(stdout, "lms_socket_create(): %s\n", strerror(errno)); return(1); }
	snprintf(l->localhost, LMS_LEN_V4ADDR, "127.0.0.1");
	l->localport = 5202;
	if (lms_socket_ilisten(l) < 0) { fprintf(stdout, "lms_socket_ilisten(): %s\n", strerror(errno)); return(1); }
	l->func_p = pfunc;
	l->func_a = afunc;
	if (lms_mux_addfd(l, 0, 0) < 0) { fprintf(stdout, "lms_mux_addfd(): %s\n", strerror(errno)); return(1); }

	while (!quitting)
	{
		lms_loop();
	}

	fputc('\n', stdout);
	fflush(stdout);
	return(0);
}

void pfunc(MSocket *m)
{
	if (!m) { return; }

	fprintf(stdout, "%s", m->recvQ);
	fflush(stdout);
	lms_socket_freerq(m);
}

void afunc(MSocket *m)
{
	if (!m) { return; }

	m->func_e = efunc;
	fprintf(stdout, "[LMS] Connect from %s:%i\n", m->remotehost, m->remoteport);
}

int efunc(MSocket *m)
{
	if (!m) { errno = EINVAL; return(-1); }

	fprintf(stdout, "[LMS] Disconnect from %s:%i\n", m->remotehost, m->remoteport);
	return(lms_socket_destroy(m));
}

void cntrlc_hdlr(int sig)
{
	quitting = 1;
}
