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

#ifndef INCLUDED_MSOCKET_H
# define INCLUDED_MSOCKET_H		1

/*
**
** DEFINES
**
*/

# define LMS_VERSION			"0.5"
# define LMS_VERSION_INT		0x000005

# define LMS_LEN_V4ADDR			16
# define LMS_LEN_V6ADDR			0

# define LMS_CONNTIMEOUT		60

# define LMSTYPE_ERROR			0
# define LMSTYPE_LOCALLISTEN		1
# define LMSTYPE_LOCALCLIENT		2
# define LMSTYPE_STREAM4		3
# define LMSTYPE_DGRAM4			4
# define LMSTYPE_LISTEN4		5
# define LMSTYPE_STREAM6		6
# define LMSTYPE_DGRAM6			7
# define LMSTYPE_LISTEN6		8

# define LMSFLG_CONNECTED		0x00001		/* Socket is connected */
# define LMSFLG_LISTEN			0x00002		/* Socket is listening */
# define LMSFLG_READY			0x00004		/* Datagram socket is ready */
# define LMSFLG_WAITDNS			0x00008		/* Waiting for a DNS response */
# define LMSFLG_WAITIDENT		0x00010		/* Waiting for an ident response */
# define LMSFLG_MUXACTIVE		0x00020		/* Socket is in the mux */
# define LMSFLG_SSL			0x00040		/* SSL connection */
# define LMSFLG_SSLHDSHK		0x00080		/* SSL handshake phase */
# define LMSFLG_SSLRDY			0x00100		/* SSL is ready */
# define LMSFLG_WAITDESTROY		0x00200		/* Socket is dead, but we are waiting for DNS and/or ident queries to return before destroying it */
# define LMSFLG_INBOUND			0x01000		/* Inbound connection via accept() */
# define LMSFLG_OUTBOUND		0x02000		/* Outbound connection via connect() */
# define LMSFLG_WAITCONN		0x04000		/* Not yet connected */

# define LMSOPTION_TRANSIENT		0x002		/* Doesn't do anything */
# define LMSOPTION_BLOCK		0x004		/* A blocking socket (default is to set nonblocking mode */
# define LMSOPTION_CWAIT		0x008		/* Blocks during connect() or whatever else, then sets nonblocking mode */
# define LMSOPTION_SSL			0x010		/* Uses SSL - self-explanatory */
# define LMSOPTION_UCREP		0x020		/* For an SSL socket, this sets some additional OpenSSL options for an "unclean" remote end-point which may require bug-adaptability */
# define LMSOPTION_ALLOWIDLE		0x040		/* Allow a socket to idle */

# define LMS_MAXDNSCACHE		30000

# if !defined(LMS_MAXDNSCACHE) || (LMS_MAXDNSCACHE <= 0)
#  define LMS_NODNSCACHE
# endif
# ifdef LMS_NODNSCACHE
#  define LMS_MAXDNSCACHE		0
# endif

# define LMS_DNS_TYPE_NONE		0
# define LMS_DNS_TYPE_A			1
# define LMS_DNS_TYPE_CNAME		2
# define LMS_DNS_TYPE_PTR		3
# define LMS_DNS_TYPE_TXT		4
# define LMS_DNS_TYPE_MX		5

# define ABSTRACT_NOTHING		0
# define ABSTRACT_STRING		1
# define ABSTRACT_MSOCKET		2
# define ABSTRACT_DNSREQUEST		9
# define ABSTRACT_CALLBACK		10
# define ABSTRACT_MAX			10240

/*
**
** STRUCTURES
**
*/

/*
 * A totally abstract thing, which can be any type of thing stored in the abstract thing...
 * along with a handy variable to tell you what type of thing is pointed to by the
 * pointer stored in the abstract thing.
 */
struct _abstract
{
	unsigned short what;
	void *where;
	/* abstract_callback *how; */
	void (*how)(struct _abstract *);
};
typedef struct _abstract Abstract;

typedef void (*abstract_callback)(struct _abstract *);

/*
 * MSocket is the structure utilized by our socket abstraction layer
 */
struct _MSocket
{
	unsigned short type;			/* type (MSTYPE_*) */
	unsigned int opts;			/* options for the socket (MSOPTION_*) */

	char *localhost;			/* local address if INET/INET6 or path to file if UNIX */
	int localport;				/* local port if INET/INET6 */
	char *remotehost;			/* remote address if INET/INET6 */
	int remoteport;				/* remote port if INET/INET6 */
	char *remotedns;			/* DNS name of the remote host if INET/INET6 */
	struct in_addr *addr;			/* in_addr structure for evdns and throttling API */

	int fd;					/* file descriptor */
	unsigned long flags;			/* flags on the socket/connection/etc (MSFLAG_*) */

	size_t sendQ_sz;			/* allocated size of current sendQ */
	size_t sendQ_len;			/* length of current sendQ */
	unsigned char *sendQ;			/* queue of data to be written */
	time_t last_send;			/* the time at which I last sent data */
	size_t bytes_s;				/* bytes sent via this connection */

	size_t recvQ_sz;			/* allocated size of current recvQ */
	size_t recvQ_len;			/* length of current recvQ */
	unsigned char *recvQ;			/* queue of data to be parsed */
	time_t last_recv;			/* the time at which I last received data */
	size_t bytes_r;				/* bytes received via this connection */

	time_t conn_start;			/* when we started a connect() */
	unsigned int conn_to;			/* time before we should give up on a connect() */

	int (*func_r)(struct _MSocket *);	/* function to call when mux says read */
	int (*func_w)(struct _MSocket *);	/* function to call when mux says write */
	int (*func_e)(struct _MSocket *);	/* function to call when mux cries foul */
	void (*func_p)(struct _MSocket *);	/* function to call when data is available in recvQ */
	void (*func_a)(struct _MSocket *);	/* function to call when a new socket has been accepted on a listener */

	void *appdata;				/* abstract application data */

	/* DNS (temp) crap down here... */
	char *possible_revdns;			/* possible reverse dns, but not yet confirmed */
	unsigned short retries;			/* retry attempts for the reverse DNS lookup */
};
typedef struct _MSocket MSocket;

/*
 * Password data storage structure used by utils_passwords_*multi()
 */
struct _lms_passwords_data
{
	unsigned char version;

	unsigned char salt[8];
	char salt_b64[17];
	unsigned char hash[32];
	char hash_b64[65];
};
typedef struct _lms_passwords_data lms_passwords_data;

/*
 * Structure for keeping track of throttled IP addresses to prevent brute-force authentication attacks
 */
struct _lms_throttle_data
{
	char ipaddr[16];
	in_addr_t addr;

	time_t last_bad;
	unsigned int offenses;

	struct _lms_throttle_data *prev;
	struct _lms_throttle_data *next;
};
typedef struct _lms_throttle_data lms_throttle_data;

/*
**
** FUNCTIONS
**
*/

/* rand.c */
extern int lms_rand_get(size_t bytes, unsigned char *dst);

/* conn.c */
extern unsigned int lms_throttle_check(in_addr_t ip);
extern int lms_throttle_expire(void);
extern lms_throttle_data *lms_throttle_setbad(MSocket *m);
extern void lms_throttle_remove(lms_throttle_data *throttle);

/* lms.c */
extern int lms_init(unsigned char print);
extern int lms_loop(void);
extern int lms_version_int(void);
extern char *lms_version(void);

/* socket.c */
extern void lms_socket_insertfd(MSocket *m);
extern inline MSocket *lms_socket_findbyfd(int fd);
extern MSocket *lms_socket_create(int type);
extern int lms_socket_close(MSocket *ptr);
extern int lms_socket_destroy(MSocket *ptr);
extern unsigned int lms_socket_destroytype(unsigned short type, short killad);
extern unsigned int lms_socket_housekeeping(void);
extern int lms_socket_ilisten(MSocket *s);
extern int lms_socket_iaccept(MSocket *s, MSocket *new);
extern int lms_socket_uaccept(MSocket *s, MSocket *new);
extern int lms_socket_iconn(MSocket *s);
extern int lms_socket_idgram(MSocket *s);
extern int lms_socket_appendq(MSocket *m, unsigned char *data, size_t data_len);
extern int lms_socket_clearsq(MSocket *m, ssize_t len);
extern int lms_socket_freerq(MSocket *m);

/* dns.c */
extern void lms_dns_cleancache(void);
extern int lms_dns_lookup(const char *h, Abstract *a);
extern int lms_dns_findrev(MSocket *m);
extern int lms_dns_getip(const char *host, char *buf, size_t buf_len);
extern int lms_dns_gethost(const char *ip, char *buf, size_t buf_len);

/* mux.c */
extern int lms_mux_addfd(MSocket *ms, int fd, unsigned short t);
extern int lms_mux_remfd(int fd);
extern void lms_mux_setprio(MSocket *s, short prio);

/* ssl.c */
extern int lms_ssl_startsock(MSocket *m);
extern int lms_ssl_closesock(MSocket *m);
extern int lms_ssl_stopsock(MSocket *m);
extern int lms_ssl_unclean(MSocket *m);
extern char *lms_ssl_getclientinfo(MSocket *m);

/*
**
** MACROS
**
*/

/* These macros set various flags on a socket */
# define LMS_SetTimeout(s, t)		((s)->conn_to = (t))
# define LMS_SetBlocking(s)		((s)->opts |= LMSOPTION_BLOCK)
# define LMS_SetAllowIdle(s)		((s)->opts |= LMSOPTION_ALLOWIDLE)
# define LMS_SetCWait(s)		((s)->opts |= LMSOPTION_CWAIT)
# define LMS_SetSSL(s)			((s)->opts |= LMSOPTION_SSL)
# define LMS_SetSSLUnClean(s)		((s)->opts |= LMSOPTION_UCREP)

/* These macros evaluate as true if the circumstance described is true */
# define LMS_IsConnected(s)		((s)->flags & LMSFLG_CONNECTED)
# define LMS_IsWaiting(s)		(((s)->flags & LMSFLG_WAITCONN) || ((s)->flags & LMSFLG_WAITDNS) || ((s)->flags & LMSFLG_WAITIDENT))

/* The following macro sends out the contents of the sockets sendQ */
# define LMS_SendQueue(s)		(s)->func_w((s))

#endif /* INCLUDED_MSOCKET_H */
