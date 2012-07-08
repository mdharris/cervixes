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


static void _lms_mux_default_read(int fd, short e, void *msoptr);
static void _lms_mux_default_write(int fd, short e, void *msoptr);
static void _lms_mux_default_error(int fd, short e, void *msoptr);

static struct event *_lms_mux_revents[LMS_HIGHSOCK];
static struct event *_lms_mux_wevents[LMS_HIGHSOCK];


struct event_base *lms_mux_evtb;

/*
 * lms_mux_init() initializes the I/O multiplexer
 *
 */
int lms_mux_init()
{
	register unsigned int i;

	lms_mux_evtb = event_init();
	event_priority_init(6);

	for (i = 0; i < LMS_HIGHSOCK; ++i)
	{
		_lms_mux_revents[i] = (struct event *)NULL;
		_lms_mux_wevents[i] = (struct event *)NULL;
	}

	return(0);
}

/*
 * lms_mux_addfd() adds a file descriptor to the I/O multiplexer
 *
 * ms = the MSocket with the file descriptor to add to the I/O multiplexer (or (MSocket *)NULL if just fd is specified)
 * fd = the file descriptor to add to the I/O multiplexer (-1 if ms is specified)
 * t = the type (LMSTYPE_*)
 *
 */
int lms_mux_addfd(MSocket *ms, int fd, unsigned short t)
{
	struct event *re;
	struct event *we;
	int prio;

	if (ms)
	{
		fd = ms->fd;
		t = ms->type;
	}
	else if (fd < 0)
	{
		errno = EINVAL;
		return(-1);
	}

	re = (struct event *)malloc(sizeof(struct event));
	if (!re)
	{
		return(-1);
	}
	we = (struct event *)malloc(sizeof(struct event));
	if (!we)
	{
		free(re);
		return(-1);
	}

	if (ms)
	{
		if ((ms->type == LMSTYPE_LISTEN4) || (ms->type == LMSTYPE_LISTEN6))
		{
			prio = 2;
		}
		else if ((ms->type == LMSTYPE_STREAM4) || (ms->type == LMSTYPE_STREAM6))
		{
			prio = 3;
		}
		else if (ms->type == LMSTYPE_LOCALLISTEN)
		{
			prio = 1;
		}
		else if (ms->type == LMSTYPE_LOCALCLIENT)
		{
			prio = 3;
		}
		else
		{
			prio = 4;
		}
	}
	else
	{
		prio = 4;
	}

	event_set(re, fd, EV_READ|EV_PERSIST, _lms_mux_default_read, ms ? (void *)ms : (void *)NULL);
	event_set(we, fd, EV_WRITE|EV_PERSIST, _lms_mux_default_write, ms ? (void *)ms : (void *)NULL);
	event_base_set(lms_mux_evtb, re);
	event_base_set(lms_mux_evtb, we);
	event_priority_set(re, prio);
	event_priority_set(we, prio);
	_lms_mux_revents[fd] = re;
	_lms_mux_wevents[fd] = we;
	event_add(re, (struct timeval *)NULL);
	event_add(we, (struct timeval *)NULL);

	if (ms)
	{
		ms->flags |= LMSFLG_MUXACTIVE;
	}

	return(0);
}

/*
 * lms_mux_remfd() froms a file descriptor from the I/O multiplexer
 *
 * fd = the file descriptor to remove from the I/O multiplexer
 *
 */
int lms_mux_remfd(int fd)
{
	MSocket *mso;

	if (fd < 0)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!_lms_mux_revents[fd] || !_lms_mux_wevents[fd])
	{
		errno = EBADF;
		return(-1);
	}

	mso = lms_socket_findbyfd(fd);
	if (mso)
	{
		mso->flags &= ~LMSFLG_MUXACTIVE;
	}

	event_del(_lms_mux_revents[fd]);
	event_del(_lms_mux_wevents[fd]);

	free(_lms_mux_revents[fd]);
	_lms_mux_revents[fd] = (struct event *)NULL;
	free(_lms_mux_wevents[fd]);
	_lms_mux_wevents[fd] = (struct event *)NULL;

	return(0);
}

/*
 * lms_mux_setprio() modifies the priority in the mux of a socket
 *
 * s = socket
 * prio = the priority to set, between 1 and 5
 *
 */
void lms_mux_setprio(MSocket *s, short prio)
{
	event_priority_set(_lms_mux_revents[s->fd], prio);
	event_priority_set(_lms_mux_wevents[s->fd], prio);
}

/*
 * Generic mux interface callback functions
 * _lms_socket_default_read() = read condition from mux
 *
 * fd = file descriptor
 * e = event type
 * msoptr = pointer to the MSocket object on which the mux found something for us
 *
 */
void _lms_mux_default_read(int fd, short e, void *msoptr)
{
	MSocket *mso;

	if (msoptr)
	{
		mso = (MSocket *)msoptr;
	}
	else if (fd >= 0)
	{
		mso = lms_socket_findbyfd(fd);
	}
	else
	{
		/* mux is going a lot batty */
		return;
	}
	if (!mso)
	{
		/* mux is going a little batty */
		return;
	}

	if ((mso->type == LMSTYPE_LISTEN4) || (mso->type == LMSTYPE_LISTEN6))
	{
		lms_conn_accept(mso);
	}
	else if ((mso->type == LMSTYPE_STREAM4) || (mso->type == LMSTYPE_STREAM6))
	{
		/* Allows us to differentiate between an SSL connection and a non-SSL connection... */
		mso->func_r(mso);
	}
/*	This isn't supported in libmsocket yet
	else if (mso->type == LMSTYPE_LOCALLISTEN)
	{
		interface_local_accept(mso);
	}
	else if (mso->type == LMSTYPE_LOCALCLIENT)
	{
		interface_local_read(mso);
	}
*/
	else
	{
		/* This should be used for datagram sockets and such */
	}

	return;
}

/*
 * Generic mux interface callback functions, set when creating a new MSocket object.
 *  I don't think most of these should be used.
 * _lms_socket_default_write() = write condition from mux
 *
 * fd = file descriptor
 * e = event type
 * msoptr = pointer to the MSocket object on which the mux found something for us
 *
 */
void _lms_mux_default_write(int fd, short e, void *msoptr)
{
	MSocket *mso;

	if (msoptr)
	{
		mso = (MSocket *)msoptr;
	}
	else if (fd >= 0)
	{
		mso = lms_socket_findbyfd(fd);
	}
	else
	{
		/* mux is going a lot batty */
		return;
	}
	if (!mso)
	{
		/* mux is going a little batty */
		return;
	}

	if (mso->flags & LMSFLG_WAITCONN)
	{
		mso->flags &= ~LMSFLG_WAITCONN;
	}
	if (!(mso->flags & LMSFLG_CONNECTED))
	{
		mso->flags |= LMSFLG_CONNECTED;
	}

	mso->func_w(mso);

	return;
}

/*
 * Generic mux interface callback function
 * _lms_mux_default_error() = error condition from mux
 *
 * fd = file descriptor
 * e = event type
 * msoptr = pointer to the MSocket object on which the mux found something for us
 *
 */
void _lms_mux_default_error(int fd, short e, void *msoptr)
{
	MSocket *mso;

	if (msoptr)
	{
		mso = (MSocket *)msoptr;
	}
	else if (fd >= 0)
	{
		mso = lms_socket_findbyfd(fd);
	}
	else
	{
		/* mux is going a lot batty */
		return;
	}

	return;
}
