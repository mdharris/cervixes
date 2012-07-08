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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

static MSocket *_lms_socket_list[LMS_HIGHSOCK];
static MSocket *_lms_socket_corpses[LMS_HIGHSOCK];


/*
 * Initialize the list of MSocket objects in _lms_socket_list
 *
 */
int *lms_socket_init()
{
	register unsigned int i;

	for (i = 0; i < LMS_HIGHSOCK; ++i)
	{
		_lms_socket_list[i] = (MSocket *)NULL;
		_lms_socket_corpses[i] = (MSocket *)NULL;
	}

	return(0);
}

/*
 * Add an fd to the local socket list
 *
 * fd = the fd to add it as an indexer
 * m = the MSocket object to add to the list
 *
 */
void lms_socket_insertfd(MSocket *m)
{
	if (!m)
	{
		return;
	}
	if ((m->fd < 0) || (m->fd >= LMS_HIGHSOCK))
	{
		return;
	}

	_lms_socket_list[m->fd] = m;
	return;
}

/*
 * Find the MSocket object corresponding to a given file descriptor
 *
 * fd = the file descriptor you wish to find the corresponding MSocket object for
 *
 */
inline MSocket *lms_socket_findbyfd(int fd)
{
	if (_lms_socket_list[fd])
	{
		return(_lms_socket_list[fd]);
	}
	else
	{
		return((MSocket *)NULL);
	}
}

/*
 * Create and allocate memory for a new MSocket object
 *
 * type = the type of the new socket to allocate
 *
 */
MSocket *lms_socket_create(int type)
{
	MSocket *ptr;

	ptr = (MSocket *)NULL;
#ifdef LMS_HARDCORE_ALLOC
	while (!ptr)
	{
		ptr = (MSocket *)malloc(sizeof(MSocket));
	}
#else
	ptr = (MSocket *)malloc(sizeof(MSocket));
	if (!ptr)
	{
		return((MSocket *)NULL);
	}
#endif
	memset(ptr, 0, sizeof(MSocket));

	ptr->type = type;
	ptr->fd = -1;
	ptr->flags = 0;
	ptr->opts = 0;

	ptr->sendQ_sz = 0;
	ptr->sendQ_len = 0;
	ptr->sendQ = (unsigned char *)NULL;
	ptr->recvQ_sz = 0;
	ptr->recvQ_len = 0;
	ptr->recvQ = (unsigned char *)NULL;

	if (type == LMSTYPE_LOCALLISTEN)
	{
		ptr->localhost = (char *)malloc(MAXPATHLEN);
		if (!ptr->localhost)
		{
			free(ptr);
			return((MSocket *)NULL);
		}
		memset(ptr->localhost, 0, MAXPATHLEN);
	}
	else if ((type == LMSTYPE_LISTEN4) || (type == LMSTYPE_STREAM4) || (type == LMSTYPE_DGRAM4))
	{
		/* Space for an IPv4 address */
		ptr->localhost = (char *)malloc(LMS_LEN_V4ADDR);
		if (!ptr->localhost)
		{
			free(ptr);
			return((MSocket *)NULL);
		}
		memset(ptr->localhost, 0, LMS_LEN_V4ADDR);
	}
	else if ((type == LMSTYPE_LISTEN6) || (type == LMSTYPE_STREAM6) || (type == LMSTYPE_DGRAM6))
	{
		/* Space for an IPv6 address */
		ptr->localhost = (char *)malloc(LMS_LEN_V6ADDR);
		if (!ptr->localhost)
		{
			free(ptr);
			return((MSocket *)NULL);
		}
		memset(ptr->localhost, 0, LMS_LEN_V6ADDR);
	}
	else
	{
		ptr->localhost = (char *)NULL;
	}
	ptr->localport = 0;

	if ((type == LMSTYPE_STREAM4) || (type == LMSTYPE_DGRAM4))
	{
		/* Space for an IPv4 address */
		ptr->remotehost = (char *)malloc(LMS_LEN_V4ADDR);
		if (!ptr->remotehost)
		{
			if (ptr->localhost)
			{
				free(ptr->localhost);
			}
			free(ptr);
			return((MSocket *)NULL);
		}
		memset(ptr->remotehost, 0, LMS_LEN_V4ADDR);
	}
	else if ((type == LMSTYPE_STREAM6) || (type == LMSTYPE_DGRAM6))
	{
		/* Space for an IPv6 address */
		ptr->remotehost = (char *)malloc(LMS_LEN_V6ADDR);
		if (!ptr->remotehost)
		{
			if (ptr->localhost)
			{
				free(ptr->localhost);
			}
			free(ptr);
			return((MSocket *)NULL);
		}
		memset(ptr->remotehost, 0, LMS_LEN_V6ADDR);
	}
	else
	{
		ptr->remotehost = (char *)NULL;
	}
	ptr->remoteport = 0;

	ptr->last_send = time(NULL);
	ptr->bytes_s = 0;
	ptr->last_recv = time(NULL);
	ptr->bytes_r = 0;

	ptr->remotedns = (char *)NULL;

	ptr->appdata = (void *)NULL;

	/*
	 * Callbacks to the application abstraction layer -
	 * These end up called when the back-end stuff has been taken care of and the application layer can act
	 */
	ptr->func_r = lms_socket_read;
	ptr->func_w = lms_socket_flushq;
	ptr->func_e = lms_conn_default_error;
	ptr->func_p = lms_conn_default_read;
	ptr->func_a = lms_conn_default_accept;

	/*
	 * Stuff for the reverse DNS resolver.
	 */
	/* ptr->addr is the remote address, and is only set once we start doing a reverse DNS lookup */
	ptr->addr = (struct in_addr *)malloc(sizeof(struct in_addr));
	if (!ptr->addr)
	{
		free(ptr);
		return((MSocket *)NULL);
	}
	memset(ptr->addr, 0, sizeof(struct in_addr));
	/* This is not usually set; it is used between the first and second phase of reverse DNS lookup prior to the verification that the PTR is valid */
	ptr->possible_revdns = (char *)NULL;
	/* This specifies retries for the reverse DNS resolver */
	ptr->retries = 0;

	ptr->conn_start = 0;
	ptr->conn_to = LMS_CONNTIMEOUT;

	return(ptr);
}

/*
 * Close a Socket cleanly - dump sendQ before destroying it
 *
 * ptr = the socket to close
 *
 */
int lms_socket_close(MSocket *ptr)
{
	if (!ptr)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (ptr->flags & LMSFLG_WAITDESTROY)
	{
		errno = EINVAL;
		return(-1);
	}

	if (ptr->flags & LMSFLG_MUXACTIVE)
	{
		lms_mux_remfd(ptr->fd);
	}

	if (ptr->sendQ_len > 0)
	{
		ptr->func_w(ptr);
	}

	if (ptr->flags & LMSFLG_SSL)
	{
		lms_ssl_closesock(ptr);
	}

	lms_socket_destroy(ptr);

	return(0);
}

/*
 * Destroy an existing MSocket object and free the memory
 *
 * ptr = an MSocket created by lms_socket_create()
 *
 */
int lms_socket_destroy(MSocket *ptr)
{
	if (!ptr)
	{
		errno = EINVAL;
		return(-1);
	}

	if (ptr->flags & LMSFLG_SSL)
	{
		lms_ssl_stopsock(ptr);
	}

	if (ptr->flags & LMSFLG_MUXACTIVE)
	{
		lms_mux_remfd(ptr->fd);
	}

	if (ptr->recvQ_sz > 0)
	{
		if (ptr->recvQ)
		{
			free(ptr->recvQ);
			ptr->recvQ = (unsigned char *)NULL;
			ptr->recvQ_sz = 0;
		}
	}
	if (ptr->sendQ_sz > 0)
	{
		if (ptr->sendQ)
		{
			free(ptr->sendQ);
			ptr->sendQ = (unsigned char *)NULL;
			ptr->sendQ_sz = 0;
		}
	}

	/*
	 * One thing worth noting is that appdata needs to be taken care of elsewhere before
	 * calling lms_socket_destroy() - some things, like interface/local utilize appdata
	 * as the only place where a pointer is stored, so a failure to deal with appdata could
	 * result in memory leaks in such places.
	 */

	if (!(ptr->flags & LMSFLG_WAITDESTROY))
	{
		_lms_socket_list[ptr->fd] = (MSocket *)NULL;

		if (ptr->type != LMSTYPE_LOCALLISTEN)
		{
			/*
			 * interface/local.c has its own method of dealing with closing the socket
			 * which also includes calling unlink() on the file and such
			 */
			if (ptr->flags & LMSFLG_CONNECTED)
			{
				/* stream */
				shutdown(ptr->fd, SHUT_RDWR);
				ptr->flags &= ~LMSFLG_CONNECTED;
			}
			else if (ptr->flags & LMSFLG_READY)
			{
				/* dgram */
				shutdown(ptr->fd, SHUT_RDWR);
				ptr->flags &= ~LMSFLG_READY;
			}
			close(ptr->fd);
		}
	}
	else
	{
		if ((ptr->flags & LMSFLG_WAITDNS) || (ptr->flags & LMSFLG_WAITIDENT))
		{
			_lms_socket_corpses[ptr->fd] = ptr;
			return(0);
		}
		else
		{
			/* 
			 * Do you see it?
			 * There's a possible race condition if a socket becomes a 'corpse' before the last corpse that had that same fd
			 * number has its DNS query return... Hmm.  The likelihood is small, however, and the impact is a small memory
			 * leak, so I'll worry about it later.  
			 */
			_lms_socket_corpses[ptr->fd] = (MSocket *)NULL;
		}
	}

	if ((ptr->flags & LMSFLG_WAITDNS) || (ptr->flags & LMSFLG_WAITIDENT))
	{
		/* We can't safely destroy it yet. */
		ptr->flags |= LMSFLG_WAITDESTROY;
		ptr->flags &= ~LMSFLG_CONNECTED;
		ptr->flags &= ~LMSFLG_READY;
		_lms_socket_corpses[ptr->fd] = ptr;
		return(0);
	}

	if (ptr->remotedns)
	{
		free(ptr->remotedns);
		ptr->remotedns = (char *)NULL;
	}
	if (ptr->remotehost)
	{
		free(ptr->remotehost);
		ptr->remotehost = (char *)NULL;
	}
	if (ptr->localhost)
	{
		free(ptr->localhost);
		ptr->localhost = (char *)NULL;
	}

	if (ptr->addr)
	{
		free(ptr->addr);
		ptr->addr = (struct in_addr *)NULL;
	}
	if (ptr->possible_revdns)
	{
		free(ptr->possible_revdns);
		ptr->possible_revdns = (char *)NULL;
	}

	free(ptr);

	return(0);
}

/*
 * Dump all connections of a specific type
 *
 * type = the type to dump all of
 * killad = boolean var as to whether the appdata field should be checked and free()'d if it exists
 *
 */
unsigned int lms_socket_destroytype(unsigned short type, short killad)
{
	unsigned int num;
	register unsigned int i;

	errno = 0;

	if ((killad != 0) && (killad != 1))
	{
		errno = EINVAL;
		return(0);
	}
	if (type == LMSTYPE_ERROR)
	{
		return(0);
	}
	else if (type == LMSTYPE_LOCALLISTEN)
	{
		return(0);
	}

	num = 0;

	for (i = 0; i < LMS_HIGHSOCK; ++i)
	{
		if (!_lms_socket_list[i])
		{
			continue;
		}

		if (_lms_socket_list[i]->type == type)
		{
			if (killad && _lms_socket_list[i]->appdata)
			{
				free(_lms_socket_list[i]->appdata);
				_lms_socket_list[i]->appdata = (void *)NULL;
			}
			if (_lms_socket_list[i]->flags & LMSFLG_MUXACTIVE)
			{
				lms_mux_remfd(_lms_socket_list[i]->fd);
			}
			lms_socket_destroy(_lms_socket_list[i]);
			num++;
		}
	}

	return(num);
}

/*
 * Dump dumpable corpses
 * Dump connections with nothing going on to prevent resource-starvation attacks as best as possible
 * Flush sendQs
 *
 */
unsigned int lms_socket_housekeeping()
{
	unsigned int num;
	register unsigned int i;
	int ka;
	time_t current;

	current = time(NULL);
	num = 0;

	for (i = 0; i < LMS_HIGHSOCK; ++i)
	{
		if (_lms_socket_corpses[i])
		{
			if ((_lms_socket_corpses[i]->flags & LMSFLG_WAITDESTROY) && !(_lms_socket_list[i]->flags & LMSFLG_WAITDNS) && !(_lms_socket_list[i]->flags & LMSFLG_WAITIDENT))
			{
				/* Query finished, we can now safely destroy it without crashing. */
				lms_socket_destroy(_lms_socket_corpses[i]);
				continue;
			}
		}

		if (!_lms_socket_list[i])
		{
			continue;
		}

		if ((_lms_socket_list[i]->flags & LMSFLG_WAITCONN) && (_lms_socket_list[i]->conn_start < (current - _lms_socket_list[i]->conn_to)))
		{
			if (_lms_socket_list[i]->func_e)
			{
				_lms_socket_list[i]->func_e(_lms_socket_list[i]);
			}
			else
			{
				if (_lms_socket_list[i]->flags & LMSFLG_MUXACTIVE)
				{
					lms_mux_remfd(_lms_socket_list[i]->fd);
				}
				lms_socket_destroy(_lms_socket_list[i]);
			}
			num++;
			continue;
		}

		if ((_lms_socket_list[i]->type == LMSTYPE_LISTEN4) || (_lms_socket_list[i]->type == LMSTYPE_LISTEN6) || (_lms_socket_list[i]->type == LMSTYPE_LOCALLISTEN))
		{
			continue;
		}
		else if (_lms_socket_list[i]->opts & LMSOPTION_ALLOWIDLE)
		{
			continue;
		}
		else if (_lms_socket_list[i]->type == LMSTYPE_LOCALCLIENT)
		{
			/* Give local clients at least an hour. */
#if (LMS_MAXKEEPALIVE < 3600)
			ka = 3600;
#else
			ka = LMS_MAXKEEPALIVE;
#endif
		}
		else
		{
			ka = LMS_MAXKEEPALIVE;
		}

		if ((_lms_socket_list[i]->last_recv <= (current - ka)) && (_lms_socket_list[i]->last_send <= (current - ka)))
		{
			if (_lms_socket_list[i]->flags & LMSFLG_MUXACTIVE)
			{
				lms_mux_remfd(_lms_socket_list[i]->fd);
			}
			lms_socket_destroy(_lms_socket_list[i]);
			num++;
			continue;
		}

		if (_lms_socket_list[i]->sendQ_len > 0)
		{
			_lms_socket_list[i]->func_w(_lms_socket_list[i]);
		}
	}

	return(num);
}

/*
 * Initiate listening on an MSocket object
 *
 * s = a new MSocket created by lms_socket_create()
 *
 */
int lms_socket_ilisten(MSocket *s)
{
        int fd_opts;
        struct sockaddr_in s_local_host;

	if (!s)
	{
		errno = EINVAL;
		return(-1);
	}

	if ((s->type != LMSTYPE_LISTEN4) && (s->type != LMSTYPE_LISTEN6))
	{
		errno = EINVAL;
		return(-1);
	}
	if (s->localport <= 0)
	{
		errno = EADDRNOTAVAIL;
		return(-1);
	}
	if ((s->flags & LMSFLG_CONNECTED) || (s->flags & LMSFLG_LISTEN))
	{
		errno = EISCONN;
		return(-1);
	}

	s->fd = socket(PF_INET, SOCK_STREAM, 0);
	if (s->fd < 0)
	{
		return(-1);
	}

	memset(&s_local_host, 0, sizeof(s_local_host));
	s_local_host.sin_family = AF_INET;
	s_local_host.sin_port = htons(s->localport);
	if (!s->localhost || (*s->localhost == '\0'))
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else if (inet_addr(s->localhost) == INADDR_NONE)
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		s_local_host.sin_addr.s_addr = inet_addr(s->localhost);
	}

	if (bind(s->fd, (struct sockaddr *)&s_local_host, sizeof(s_local_host)) != 0)
	{
		close(s->fd);
		return(-1);
	}

	fd_opts = fcntl(s->fd, F_GETFL, 0);
	fcntl(s->fd, F_SETFL, fd_opts|O_NONBLOCK);
	if (listen(s->fd, LMS_BACKLOG) != 0)
	{
		close(s->fd);
		return(-1);
	}

	if (s->opts & LMSOPTION_SSL)
	{
		s->flags |= LMSFLG_SSL;
	}

	s->flags |= LMSFLG_LISTEN;
	_lms_socket_list[s->fd] = s;

	return(0);
}

/*
 * Accept a new connection on a listener
 *
 * s = the listener on which to accept the new connection
 * new = a new MSocket created by lms_socket_create() which will be propagated by this function
 *
 */
int lms_socket_iaccept(MSocket *s, MSocket *new)
{
	int fd_opts;
	unsigned int srh_sz;
	struct sockaddr_in s_remote_host;

	if (!s || !new)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!(s->flags & LMSFLG_LISTEN))
	{
		errno = EOPNOTSUPP;
		return(-1);
	}
	if ((new->type != LMSTYPE_STREAM4) && (new->type != LMSTYPE_STREAM6))
	{
		errno = EINVAL;
		return(-1);
	}

	srh_sz = sizeof(struct sockaddr_in);

	while ((new->fd = accept(s->fd, (struct sockaddr *)&s_remote_host, &srh_sz)) < 0)
	{
		if ((errno == EAGAIN) || (errno = EINTR))
		{
			continue;
		}
		else
		{
			return(-1);
		}
	}

	if (*s->localhost != '\0')
	{
		/* FIXME: V6 support needed here. */
		if ((new->type == LMSTYPE_STREAM4) || (new->type == LMSTYPE_DGRAM4))
		{
			memcpy(new->localhost, s->localhost, LMS_LEN_V4ADDR);
		}
		else if ((new->type == LMSTYPE_STREAM6) || (new->type == LMSTYPE_DGRAM6))
		{
			memcpy(new->localhost, s->localhost, LMS_LEN_V6ADDR);
		}
	}
	new->localport = s->localport;
	if ((new->type == LMSTYPE_STREAM4) || (new->type == LMSTYPE_DGRAM4))
	{
		snprintf(new->remotehost, LMS_LEN_V4ADDR, "%s", inet_ntoa(s_remote_host.sin_addr));
	}
	else if ((new->type == LMSTYPE_STREAM6) || (new->type == LMSTYPE_DGRAM6))
	{
		snprintf(new->remotehost, LMS_LEN_V6ADDR, "%s", inet_ntoa(s_remote_host.sin_addr));
	}
	new->remoteport = s_remote_host.sin_port;

	new->addr->s_addr = s_remote_host.sin_addr.s_addr;

	if (!(new->opts & LMSOPTION_BLOCK))
	{
		fd_opts = fcntl(new->fd, F_GETFL, 0);
		fcntl(new->fd, F_SETFL, fd_opts|O_NONBLOCK);
	}

	new->flags |= LMSFLG_CONNECTED|LMSFLG_INBOUND;

	new->func_r = lms_socket_read;
	new->func_w = lms_socket_flushq;

	if (s->flags & LMSFLG_SSL)
	{
		new->opts |= LMSOPTION_SSL;
		/* lms_ssl_startsock() will update func_r and func_w appropriately */
		lms_ssl_startsock(new);
	}

	_lms_socket_list[new->fd] = new;

	return(0);
}

/*
 * Accept a new connection on a UNIX domain socket listener
 *
 * s = the listener on which to accept the new connection
 * new = a new MSocket created by lms_socket_create() which will be propagated by this function
 *
 */
int lms_socket_uaccept(MSocket *s, MSocket *new)
{
	int fd_opts;
	unsigned int srh_sz;
	struct sockaddr_un s_remote_host;

	if (!s || !new)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!(s->flags & LMSFLG_LISTEN))
	{
		errno = EOPNOTSUPP;
		return(-1);
	}
	if (new->type != LMSTYPE_LOCALCLIENT)
	{
		errno = EINVAL;
		return(-1);
	}

	srh_sz = sizeof(struct sockaddr_un);

	while ((new->fd = accept(s->fd, (struct sockaddr *)&s_remote_host, &srh_sz)) < 0)
	{
		if (errno == EAGAIN)
		{
			continue;
		}
		else
		{
			return(-1);
		}
	}

	fd_opts = fcntl(new->fd, F_GETFL, 0);
	fcntl(new->fd, F_SETFL, fd_opts|O_NONBLOCK);

	new->flags |= LMSFLG_CONNECTED|LMSFLG_INBOUND;
	_lms_socket_list[new->fd] = new;

	return(0);
}

/*
 * Initiate an outbound connection on an MSocket object
 *
 * s = a new MSocket created by lms_socket_create()
 *
 */
int lms_socket_iconn(MSocket *s)
{
	int fd_opts;
	struct sockaddr_in s_local_host;
	static struct sockaddr_in s_remote_host;

	if (!s)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!s->remotehost || (*s->remotehost == '\0') || (s->remoteport <= 0))
	{
		errno = EDESTADDRREQ;
		return(-1);
	}
	if ((s->type != LMSTYPE_STREAM4) && (s->type != LMSTYPE_STREAM6))
	{
		errno = EINVAL;
		return(-1);
	}
	if ((s->flags & LMSFLG_CONNECTED) || (s->flags & LMSFLG_LISTEN))
	{
		errno = EISCONN;
		return(-1);
	}

	s->fd = socket(PF_INET, SOCK_STREAM, 0);
	if (s->fd < 0)
	{
		return(-1);
	}

	memset(&s_local_host, 0, sizeof(s_local_host));
	s_local_host.sin_family = AF_INET;
	s_local_host.sin_port = htons(s->localport);
	if (!s->localhost || (s->localhost[0] == 0))
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else if (inet_addr(s->localhost) == INADDR_NONE)
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		s_local_host.sin_addr.s_addr = inet_addr(s->localhost);
	}

	if (bind(s->fd, (struct sockaddr *)&s_local_host, sizeof(struct sockaddr_in)) != 0)
	{
		close(s->fd);
		return(-1);
	}

	if (!(s->opts & LMSOPTION_CWAIT) && !(s->opts & LMSOPTION_BLOCK))
	{
		fd_opts = fcntl(s->fd, F_GETFL, 0);
		fcntl(s->fd, F_SETFL, fd_opts|O_NONBLOCK);
	}

	memset(&s_remote_host, 0, sizeof(s_remote_host));
	s_remote_host.sin_family = AF_INET;
	s_remote_host.sin_addr.s_addr = inet_addr(s->remotehost);
	s_remote_host.sin_port = htons(s->remoteport);
	if (connect(s->fd, (struct sockaddr *)&s_remote_host, sizeof(struct sockaddr_in)) != 0)
	{
		if (errno == EISCONN)
		{
			/* This is a "wtf?" */
			return(-1);
		}
		else if (errno != EINPROGRESS)
		{
			close(s->fd);
			return(-1);
		}

		s->flags |= LMSFLG_WAITCONN;
		s->conn_start = time(NULL);
	}
	else
	{
		s->flags |= LMSFLG_CONNECTED;
	}

	s->addr->s_addr = inet_addr(s->remotehost);

	if ((s->opts & LMSOPTION_CWAIT) && !(s->opts & LMSOPTION_BLOCK))
	{
		fd_opts = fcntl(s->fd, F_GETFL, 0);
		fcntl(s->fd, F_SETFL, fd_opts|O_NONBLOCK);
	}

	s->flags |= LMSFLG_OUTBOUND;

	s->func_r = lms_socket_read;
	s->func_w = lms_socket_flushq;

	if (s->opts & LMSOPTION_SSL)
	{
		/* lms_ssl_startsock() will update func_r and func_w appropriately */
		lms_ssl_startsock(s);
	}

	_lms_socket_list[s->fd] = s;

	return(0);
}

/*
 * Initiate a connection-less datagram socket on an MSocket object
 *
 * s = a new MSocket created by lms_socket_create()
 *
 */
int lms_socket_idgram(MSocket *s)
{
	int fd_opts;
	struct sockaddr_in s_local_host;

	if (!s)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!((s->type == LMSTYPE_DGRAM4) || (s->type == LMSTYPE_DGRAM6)))
	{
		errno = EINVAL;
		return(-1);
	}
	if ((s->flags & LMSFLG_CONNECTED) || (s->flags & LMSFLG_LISTEN))
	{
		errno = EISCONN;
		return(-1);
	}

	s->fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (s->fd < 0)
	{
		return(-1);
	}

	memset(&s_local_host, 0, sizeof(s_local_host));
	s_local_host.sin_family = AF_INET;
	s_local_host.sin_port = htons(s->localport);
	if (!s->localhost || (s->localhost[0] == 0))
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else if (inet_addr(s->localhost) == INADDR_NONE)
	{
		s_local_host.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		s_local_host.sin_addr.s_addr = inet_addr(s->localhost);
	}

	if (bind(s->fd, (struct sockaddr *)&s_local_host, sizeof(struct sockaddr_in)) != 0)
	{
		close(s->fd);
		return(-1);
	}

	fd_opts = fcntl(s->fd, F_GETFL, 0);
	fcntl(s->fd, F_SETFL, fd_opts|O_NONBLOCK);

	_lms_socket_list[s->fd] = s;

	/* If a remoteport and remotehost are given, call it an outbound socket, otherwise treat it as an inbound socket */
	if ((s->remoteport > 0) && s->remotehost && (s->remotehost[0] != 0))
	{
		s->flags |= LMSFLG_CONNECTED|LMSFLG_OUTBOUND;
	}
	else
	{
		s->flags |= LMSFLG_LISTEN|LMSFLG_INBOUND;
	}
	s->flags |= LMSFLG_READY;

	return(0);
}

/*
 * Read from a connection
 *
 * m = the connection to read from
 *
 */
int lms_socket_read(MSocket *m)
{
	unsigned char *c;
	int recvrv;
	size_t bufsz;
	unsigned char callpfunc;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!(m->flags & LMSFLG_CONNECTED))
	{
		errno = ENOTCONN;
		return(-1);
	}
	if ((m->type != LMSTYPE_STREAM4) && (m->type != LMSTYPE_STREAM6))
	{
		errno = EINVAL;
		return(-1);
	}

	callpfunc = 0;
	bufsz = 1024;

	c = (unsigned char *)malloc(bufsz);
	if (!c)
	{
		return(-1);
	}
	memset(c, 0, bufsz);

	if ((m->recvQ_sz > 0) && m->recvQ)
	{
		if (m->recvQ_sz < (m->recvQ_len + bufsz))
		{
#ifdef HAVE_REALLOCF
			m->recvQ = (unsigned char *)reallocf(m->recvQ, (m->recvQ_sz + bufsz));
#else
			m->recvQ = (unsigned char *)realloc(m->recvQ, (m->recvQ_sz + bufsz));
#endif /* HAVE_REALLOCF */
			if (!m->recvQ)
			{
				free(c);
				return(-1);
			}
			m->recvQ_sz += bufsz;
		}
	}
	else
	{
		m->recvQ = (unsigned char *)malloc(bufsz);
		if (!m->recvQ)
		{
			free(c);
			return(-1);
		}
		m->recvQ_sz = bufsz;
	}

	while (1)
	{
#ifdef MSG_DONTWAIT
# define _SERVER_SOCKET_RECVFLAGS		MSG_DONTWAIT
#else
# define _SERVER_SOCKET_RECVFLAGS		0
#endif
		recvrv = recv(m->fd, c, bufsz, _SERVER_SOCKET_RECVFLAGS);

		if (recvrv < 0)
		{
			if ((errno == EAGAIN) || (errno == EINTR))
			{
				break;
			}
			else
			{
				m->func_e(m);
				return(0);
			}
		}
		else if (recvrv == 0)
		{
			m->func_e(m);
			return(0);
		}
		else
		{
			lms_str_ocopy(c, m->recvQ, recvrv, m->recvQ_len);
			m->recvQ_len += recvrv;
			memset(c, 0, bufsz);
			callpfunc = 1;
			m->bytes_r += recvrv;
		}

		if (m->recvQ_sz < (m->recvQ_len + bufsz))
		{
#ifdef HAVE_REALLOCF
			m->recvQ = (unsigned char *)reallocf(m->recvQ, (m->recvQ_sz + bufsz));
#else
			m->recvQ = (unsigned char *)realloc(m->recvQ, (m->recvQ_sz + bufsz));
#endif /* HAVE_REALLOCF */
			if (!m->recvQ)
			{
				free(c);
				return(-1);
			}
			m->recvQ_sz += bufsz;
		}
	}

	if (callpfunc)
	{
		m->func_p(m);
	}
	m->last_recv = time(NULL);

	free(c);
	return(0);
}

/*
 * Flush the sendQ of a socket
 *
 * m = the socket to flush
 *
 */
int lms_socket_flushq(MSocket *m)
{
	ssize_t rv;
	ssize_t sl;
	unsigned char *tmpptr;
	size_t tmplen;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!(m->flags & LMSFLG_CONNECTED))
	{
		errno = ENOTCONN;
		return(-1);
	}
	if ((m->type != LMSTYPE_STREAM4) && (m->type != LMSTYPE_STREAM6) && (m->type != LMSTYPE_LOCALCLIENT))
	{
		errno = EINVAL;
		return(-1);
	}
	if ((m->sendQ_len == 0) || (m->sendQ_sz == 0))
	{
		return(0);
	}
	if (!m->sendQ)
	{
#ifdef EDOOFUS
		errno = EDOOFUS;
#else
		errno = EINVAL;
#endif
		return(-1);
	}

	sl = 0;
	tmpptr = m->sendQ;
	tmplen = m->sendQ_len;
	while (tmplen > 0)
	{
		rv = send(m->fd, tmpptr, tmplen, 0);
		if (rv < 0)
		{
			if (sl > 0)
			{
				lms_socket_clearsq(m, sl);
				m->sendQ_len -= sl;
				m->last_send = time(NULL);
			}
			return((int)sl);
		}
		else
		{
			sl += rv;
			tmplen -= rv;
			if (sl < m->sendQ_len)
			{
				tmpptr += rv;
			}
			m->bytes_r += rv;
		}
	}

	m->sendQ_len = 0;
	free(m->sendQ);
	m->sendQ = (unsigned char *)NULL;
	m->sendQ_sz = 0;

	m->last_send = time(NULL);

	return((int)sl);
}

/*
 * lms_socket_appendq() eithers creates the sendQ on a socket, or appends to it
 *
 * m = the socket
 * data = the data to be added to the sendQ
 * data_len = the exact length of the data to append to the sendQ
 *
 */
int lms_socket_appendq(MSocket *m, unsigned char *data, size_t data_len)
{
	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}

	if (m->sendQ_sz > 0)
	{
		if (!m->sendQ)
		{
#ifdef EDOOFUS
			errno = EDOOFUS;
#else
			errno = EINVAL;
#endif
			return(-1);
		}

#ifdef HAVE_REALLOCF
		m->sendQ = reallocf(m->sendQ, (m->sendQ_sz + data_len));
#else
		m->sendQ = realloc(m->sendQ, (m->sendQ_sz + data_len));
#endif /* HAVE_REALLOCF */
		if (!m->sendQ)
		{
			return(-1);
		}
		m->sendQ_sz += data_len;
		lms_str_ocopy(data, m->sendQ, data_len, m->sendQ_len);
		m->sendQ_len += data_len;
	}
	else
	{
		m->sendQ = (unsigned char *)malloc(data_len);
		if (!m->sendQ)
		{
			return(-1);
		}
		m->sendQ_sz = data_len;
		lms_str_copy(data, m->sendQ, data_len);
		m->sendQ_len = data_len;
	}

	return(0);
}

/*
 * lms_socket_clearsq() clears bytes from the front of the sendQ of a socket
 *
 * m = the socket to clear them from the sendQ of
 * len = the number of bytes to clear
 *
 */
int lms_socket_clearsq(MSocket *m, ssize_t len)
{
	ssize_t newlen;
	unsigned char *p;

	newlen = (m->sendQ_len - len);
	if (newlen < 0)
	{
		return(-1);
	}
	else if (newlen == 0)
	{
		if (m->sendQ)
		{
			free(m->sendQ);
		}
		m->sendQ_sz = 0;
		m->sendQ_len = 0;
		m->sendQ = (unsigned char *)NULL;

		return(0);
	}
	p = (unsigned char *)malloc(m->sendQ_len);
	if (!p)
	{
		return(-1);
	}
	memcpy(p, m->sendQ, m->sendQ_len);
#ifdef HAVE_REALLOCF
	m->sendQ = reallocf(m->sendQ, newlen);
#else
	m->sendQ = realloc(m->sendQ, newlen);
#endif /* HAVE_REALLOCF */
	if (!m->sendQ)
	{
		m->sendQ_sz = 0;
		free(p);
		return(-1);
	}
	m->sendQ_sz = newlen;
	memset(m->sendQ, 0, m->sendQ_sz);
	lms_str_ocopy(p, m->sendQ, m->sendQ_sz, len);

	free(p);
	return(0);
}

/*
 * lms_socket_freerq() frees up the recvQ of a socket once the application is done with it
 *
 * m = the socket which it's done with the recvQ of
 *
 */
int lms_socket_freerq(MSocket *m)
{
	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	if (!m->recvQ)
	{
		m->recvQ_len = 0;
		m->recvQ_sz = 0;
		errno = EINVAL;
		return(-1);
	}

	m->recvQ_len = 0;
	m->recvQ_sz = 0;
	free(m->recvQ);
	m->recvQ = (unsigned char *)NULL;

	return(0);
}
