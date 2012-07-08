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
 * This sample program grabs the main page from www.google.com
 * It will print the document to stdout
 *
 */

static unsigned char quitting;
extern void pfunc(MSocket *m);
extern int efunc(MSocket *m);
static void cntrlc_hdlr(int sig);
extern void dnscallback(Abstract *aptr);


int main(int argc, char **argv)
{
	MSocket *sock;
	Abstract *dnslookup;
	char google[] = "www.google.com";

	quitting = 0;
	signal(SIGINT, cntrlc_hdlr);

	fprintf(stdout, "libmsocket example code: libmsocket version %s\n", lms_version());

	if (lms_init(1) < 0) { return(1); }

	sock = lms_socket_create(LMSTYPE_STREAM4);
	if (!sock) { fprintf(stdout, "lms_socket_create(): %s\n", strerror(errno)); return(1); }
	if (!sock->remotedns)
	{
		/* remotedns begins life as a null pointer - no sense allocating the memory if it isn't necessary, but since we're doing a DNS callback, it will be necessary. */
		sock->remotedns = (char *)malloc(256);
		if (!sock->remotedns) { fprintf(stdout, "malloc(sock->remotedns): %s\n", strerror(errno)); return(1); }
		memset(sock->remotedns, 0, 256);
	}
	strncpy(sock->remotedns, google, LMS_LEN_V4ADDR);
	sock->remoteport = 80;
	sock->opts |= LMSOPTION_CWAIT;

	dnslookup = (Abstract *)malloc(sizeof(Abstract));
	if (!dnslookup) { fprintf(stdout, "malloc(dnslookup): %s\n", strerror(errno)); return(1); }
	memset(dnslookup, 0, sizeof(Abstract));
	dnslookup->what = ABSTRACT_CALLBACK;
	dnslookup->where = sock;
	dnslookup->how = dnscallback;

	lms_dns_lookup(google, dnslookup);

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

int efunc(MSocket *m)
{
	if (!m) { errno = EINVAL; return(-1); }

	fprintf(stdout, "Disconnected from %s:%i\n", m->remotehost, m->remoteport);
	if (lms_socket_destroy(m) < 0)
	{
		return(-1);
	}
	quitting = 1;
	return(0);
}

void cntrlc_hdlr(int sig)
{
	quitting = 1;
}

void dnscallback(Abstract *aptr)
{
	MSocket *m;
	char ipbuf[LMS_LEN_V4ADDR];

	if (!aptr) { return; }

	m = (MSocket *)aptr->where;
	if (lms_dns_getip(m->remotedns, ipbuf, LMS_LEN_V4ADDR) > 0)
	{
		strncpy(m->remotehost, ipbuf, LMS_LEN_V4ADDR);
		if (lms_socket_iconn(m) < 0)
		{
			fprintf(stdout, "Connection to %s:%i failed: %s\n", m->remotehost, m->remoteport, strerror(errno));
			quitting = 1;
			return;
		}
		if (lms_mux_addfd(m, 0, 0) < 0)
		{
			fprintf(stdout, "lms_mux_addfd(): %s\n", strerror(errno));
			quitting = 1;
			return;
		}
		m->func_p = pfunc;
		m->func_e = efunc;
		fprintf(stdout, "Connecting to %s:%i\n", m->remotehost, m->remoteport);
	}
	else
	{
		fprintf(stdout, "Host %s could not be resolved.\n", m->remotedns);
		quitting = 1;
	}
}
