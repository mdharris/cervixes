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

#include <msocket-internal.h>


static lms_throttle_data *_lms_throttle_last = (lms_throttle_data *)NULL;


/*
 * lms_conn_accept() accepts a new connection and tries to deal with it
 *
 * lms_mux calls this directly.  it's basically just a fancy wrapper around lms_socket_iaccept()
 *
 * l = the listen() socket
 *
 */
int lms_conn_accept(MSocket *l)
{
	MSocket *new;

	new = (MSocket *)NULL;

	if (!l)
	{
		errno = EINVAL;
		return(-1);
	}

	if (l->type == LMSTYPE_LISTEN4)
	{
		new = lms_socket_create(LMSTYPE_STREAM4);
	}
	else if (l->type == LMSTYPE_LISTEN6)
	{
		new = lms_socket_create(LMSTYPE_STREAM6);
	}
	else
	{
		errno = EINVAL;
		return(-1);
	}
	if (!new)
	{
		return(-1);
	}

	if (lms_socket_iaccept(l, new) < 0)
	{
		lms_socket_destroy(new);
		return(-1);
	}

	if (lms_mux_addfd(new, 0, 0) < 0)
	{
		lms_socket_destroy(new);
		return(-1);
	}

#ifdef LMS_THROTTLE_ENABLE
	if (lms_throttle_check(new->addr->s_addr) >= LMS_THROTTLE_AFTER)
	{
		/* Client is on throttle */
		lms_mux_setprio(new, 5);
	}
	else
	{
		new->func_p = lms_conn_default_read;
	}
#else
	new->func_p = lms_conn_default_read;
#endif

	if (l->func_p)
	{
		new->func_p = l->func_p;
	}

	l->func_a(new);

	return(0);
}

/*
 * lms_conn_default_read() - a default function to call when data has been recv()'d from a client
 *
 * This is set as the default read handler for a socket and will likely be used for listener clients as well
 * Called from lms_socket_read() which recv()s the new data into the client's recvQ buffer
 * Called from lms_ssl_read() which recv()s the new data and then decrypts it before placing it into the client's recvQ buffer
 *
 * m = the socket on which to deal with recvQ
 *
 */
void lms_conn_default_read(MSocket *m)
{
	if ((m->flags & LMSFLG_WAITDNS) || (m->flags & LMSFLG_WAITIDENT))
	{
		/* If we are still waiting for DNS or ident responses, don't process data on the socket yet */
		return;
	}

	return;
}

/*
 * lms_conn_default_write() - probably never called
 *
 * This is set as the default write handler for a socket
 *
 * m = the socket against which we were called
 *
 */
int lms_conn_default_write(MSocket *m)
{
	return(0);
}

/*
 * lms_conn_default_error() - a default function to call when a socket is closed, or has a non-transient error condition
 *
 * This is set as the default error handler for a socket and will likely be used for listener clients as well
 * Called from lms_socket_read(), or from lms_mux, or any number of other places (but the mux itself won't use it as a callback)
 *
 * m = the socket which has gone away or otherwise blown up
 *
 */
int lms_conn_default_error(MSocket *m)
{
	if (m->flags & LMSFLG_SSL)
	{
		lms_ssl_stopsock(m);
	}

	if ((m->flags & LMSFLG_WAITDESTROY) && ((m->flags & LMSFLG_WAITDNS) || (m->flags & LMSFLG_WAITIDENT)))
	{
		return(0);
	}

	lms_socket_destroy(m);

	return(0);
}

/*
 * lms_conn_default_accept() - a default function to call when we just accepted a new connection
 *
 * This is set as the default new-socket-has-been-accepted handler
 *
 * m = the socket which we just accept()'d
 *
 */
void lms_conn_default_accept(MSocket *m)
{
	return;
}

/*
 * lms_throttle_expire() removes any throttles which have expired from the list
 *
 */
int lms_throttle_expire()
{
	lms_throttle_data *d;
	lms_throttle_data *saved_d;
	int num;

#ifndef LMS_THROTTLE_ENABLE
	return(0);
#else
	num = 0;
	for (d = _lms_throttle_last; d; d = saved_d->prev)
	{
		if (!d)
		{
			break;
		}

		saved_d = d;

		if (d->last_bad < (time(NULL) - LMS_THROTTLE_EXPIRE))
		{
			lms_throttle_remove(d);
			++num;
		}
	}

	return(num);
#endif /* LMS_THROTTLE_ENABLE */
}

/*
 * lms_throttle_check() returns 1 if the client ought to be throttle, 0 otherwise
 *
 * ip = the IP address to check for
 *
 */
unsigned int lms_throttle_check(in_addr_t ip)
{
	lms_throttle_data *d;

	if (!ip)
	{
		errno = EINVAL;
		return(0);
	}

#ifndef LMS_THROTTLE_ENABLE
	return(0);
#endif

	for (d = _lms_throttle_last; d; d = d->prev)
	{
		if (!d)
		{
			break;
		}

		if (ip == d->addr)
		{
			return(d->offenses);
		}
	}

	return(0);
}

/*
 * lms_throttle_setbad()
 *
 * ip = the IP address to add/update an entry for
 *
 */
lms_throttle_data *lms_throttle_setbad(MSocket *m)
{
	lms_throttle_data *d;

	if (!m)
	{
		errno = EINVAL;
		return((lms_throttle_data *)NULL);
	}
	else if (!m->addr)
	{
		errno = EINVAL;
		return((lms_throttle_data *)NULL);
	}

#ifndef LMS_THROTTLE_ENABLE
	return((lms_throttle_data *)NULL);
#endif

	for (d = _lms_throttle_last; d; d = d->prev)
	{
		if (!d)
		{
			break;
		}

		if (m->addr->s_addr == d->addr)
		{
			break;
		}
	}

	if (!d)
	{
		d = (lms_throttle_data *)malloc(sizeof(lms_throttle_data));
		if (!d)
		{
			return((lms_throttle_data *)NULL);
		}
		memset(d, 0, sizeof(lms_throttle_data));

		if (!_lms_throttle_last)
		{
			_lms_throttle_last = d;
			d->prev = (lms_throttle_data *)NULL;
			d->next = (lms_throttle_data *)NULL;
		}
		else
		{
			d->prev = _lms_throttle_last;
			_lms_throttle_last->next = d;
			d->next = (lms_throttle_data *)NULL;
			_lms_throttle_last = d;
		}

		d->addr = m->addr->s_addr;
		strncpy(d->ipaddr, m->remotehost, 16);

		d->last_bad = time(NULL);
		d->offenses = 1;
	}
	else
	{
		d->last_bad = time(NULL);
		d->offenses++;
	}

	return(d);
}

/*
 * lms_throttle_remove() removes a structure from the list of throttles
 *
 * throttle = the throttle data to remove from the list
 *
 */
void lms_throttle_remove(lms_throttle_data *throttle)
{
	if (!throttle)
	{
		return;
	}

	if (_lms_throttle_last == throttle)
	{
		if (throttle->prev)
		{
			_lms_throttle_last = throttle->prev;
		}
		else
		{
			_lms_throttle_last = (lms_throttle_data *)NULL;
		}
	}
	if (throttle->next)
	{
		if (throttle->prev)
		{
			throttle->next->prev = throttle->prev;
		}
		else
		{
			throttle->next->prev = (lms_throttle_data *)NULL;
		}
	}
	if (throttle->prev)
	{
		if (throttle->next)
		{
			throttle->prev->next = throttle->next;
		}
		else
		{
			throttle->prev->next = (lms_throttle_data *)NULL;
		}
	}

	free(throttle);
}

/*
 * lms_throttle_getstats() returns statistics about throttled connections
 *
 */
int lms_throttle_getstats()
{
	lms_throttle_data *d;
	int num;

	num = 0;

	for (d = _lms_throttle_last; d; d = d->prev)
	{
		if (!d)
		{
			break;
		}
	}

	return(num);
}
