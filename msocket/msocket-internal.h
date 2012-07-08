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

#ifndef INCLUDED_MSOCKETINTERNAL_H
# define INCLUDED_MSOCKETINTERNAL_H	1

# include "localconfig.h"
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
# include <signal.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/param.h>
# include <limits.h>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/un.h>
/* libevent and OpenSSL includes */
# include <event.h>
# include <openssl/opensslv.h>
# include <openssl/rand.h>
# include <openssl/bio.h>
# include <openssl/crypto.h>
# include <openssl/evp.h>
# include <openssl/ssl.h>
# include <openssl/rsa.h>
# include <openssl/dsa.h>
# include <openssl/dh.h>
# include <openssl/engine.h>
# include <openssl/err.h>

# define stringtoint(x)			(int)strtol((x), (char **)NULL, 10)
# define stringtolong(x)		strtol((x), (char **)NULL, 10)
# define stringtouint(x)		(unsigned int)strtoul((x), (char **)NULL, 10)
# define stringtoulong(x)		strtoul((x), (char **)NULL, 10)
# define stringtofloat(x)		strtof((x), (char **)NULL)

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

# define LMS_VERSION			"0.5"
# define LMS_VERSION_INT		0x000005

# define LMS_HIGHSOCK			32768

# define LMS_CONNTIMEOUT		60

/*
 * Basically, these defaults state that after 5 bad attempts in 30 minutes, you'll be throttled for 30 minutes (from the time of the most recent
 * bad attempt.  This is relatively easy to tune, for example if you wish to use less memory you can lower the expiration time, or
 * if you undergo frequent brute-force attacks you may wish to decrease the amount of bad attempts before a client IP receives the throttle
 * handler by decreasing `LMS_THROTTLE_AFTER'.  
 */
# ifdef LMS_THROTTLE_ENABLE
#  define LMS_THROTTLE_EXPIRE		1800		/* How long after the last bad attempt do we remove the throttle entry */
#  define LMS_THROTTLE_AFTER		5		/* How many bad attempts can an IP make before feeling the negative effects of throttling */
# endif /* LMS_THROTTLE_ENABLE */

# define LMS_BACKLOG			SOMAXCONN

# define LMS_LEN_V4ADDR			16
# define LMS_LEN_V6ADDR			0

# define LMS_MAXKEEPALIVE		600		/* Ten minutes before we blow away an idle socket */

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

# define LMSOPTION_TRANSIENT		0x002
# define LMSOPTION_BLOCK		0x004
# define LMSOPTION_CWAIT		0x008
# define LMSOPTION_SSL			0x010
# define LMSOPTION_UCREP		0x020
# define LMSOPTION_ALLOWIDLE		0x040

# define LMS_SSL_SEEDLEN		1024
# undef LMS_SSLV2

# define ABSTRACT_NOTHING		0
# define ABSTRACT_STRING		1
# define ABSTRACT_MSOCKET		2
# define ABSTRACT_DNSREQUEST		9
# define ABSTRACT_CALLBACK		10
# define ABSTRACT_MAX			10240

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

struct _lms_DNSCache
{
	unsigned short type;			/* type of record (DNS_TYPE_*) */

	unsigned short negative;		/* negative cache entry or not */
	char ip[16];				/* ip address is string format */
	char *host;				/* host associated with record */

	time_t expiry;				/* time() + TTL when we recv the info */

};
typedef struct _lms_DNSCache lms_DNSCache;

struct _lms_ssl_store
{
	X509 *ca;

	char *crt_file;
	X509 *crt;

	char *privkey_file;
	RSA *privkey;
	char *pubkey_file;
	RSA *pubkey;

	char *dhp_file;
	DH *dhp;
};
typedef struct _lms_ssl_store lms_ssl_store;

/* dns.c */
extern unsigned int lms_dns_activequeries;
extern int lms_dns_init(void);
extern void lms_dns_cleancache(void);
extern int lms_dns_lookup(const char *h, Abstract *a);
extern int lms_dns_findrev(MSocket *m);
extern int lms_dns_getip(const char *host, char *buf, size_t buf_len);
extern int lms_dns_gethost(const char *ip, char *buf, size_t buf_len);
extern size_t lms_file_readln(int fd, char *buf, size_t buf_len);
extern int lms_file_writepid(const char *fn, pid_t pid);
extern short lms_file_icanrw(struct stat *fs);
extern short lms_file_icanr(struct stat *fs);

/* conn.c */
extern int lms_conn_accept(MSocket *l);
extern void lms_conn_default_read(MSocket *m);
extern void lms_conn_throttled_read(MSocket *m);
extern int lms_conn_default_write(MSocket *m);
extern int lms_conn_default_error(MSocket *m);
extern void lms_conn_default_accept(MSocket *m);
extern unsigned int lms_throttle_check(in_addr_t ip);
extern int lms_throttle_expire(void);
extern lms_throttle_data *lms_throttle_setbad(MSocket *m);
extern void lms_throttle_remove(lms_throttle_data *throttle);

/* lms.c */
extern int lms_loop(void);
extern int lms_init(unsigned char print);

/* socket.c */
extern int *lms_socket_init(void);
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
extern int lms_socket_read(MSocket *m);
extern int lms_socket_flushq(MSocket *m);
extern int lms_socket_appendq(MSocket *m, unsigned char *data, size_t data_len);
extern int lms_socket_clearsq(MSocket *m, ssize_t len);
extern int lms_socket_freerq(MSocket *m);

/* mux.c */
extern struct event_base *lms_mux_evtb;
extern int lms_mux_init(void);
extern int lms_mux_addfd(MSocket *ms, int fd, unsigned short t);
extern int lms_mux_remfd(int fd);
extern void lms_mux_setprio(MSocket *s, short prio);

/* base64.c */
  /* Note here: strlen() is used, so the string must be null-terminated to use this macro... */
# define lms_base64_dstrlen(x)			(3 * (strlen((x)) / 4 + 1))
  /* This one is safe if it's not a null-terminated string, just pass the length of the data to be decoded. */
# define lms_base64_dlen(x)			(3 * ((x) / 4 + 1))
extern unsigned char *lms_base64_encode(unsigned char *src, unsigned char *dst, size_t len);
extern unsigned char *lms_base64_decode(unsigned char *src, unsigned char *dst);

/* passwords.c */
extern int lms_passwords_encode(char *indata, char *outdata, unsigned short use_b64);
extern int lms_passwords_check(char *chk, const char *real, unsigned short is_b64);
extern size_t lms_passwords_len(unsigned short use_b64);
extern int lms_passwords_encodemulti(char *indata, lms_passwords_data *outdata);
extern int lms_passwords_checkmulti(char *chk, lms_passwords_data *real);
extern int lms_passwords_converttomulti(unsigned char *indata, lms_passwords_data *outdata, unsigned short is_b64);

/* ssl.c */
extern int lms_ssl_init(void);
extern int lms_ssl_startsock(MSocket *m);
extern int lms_ssl_closesock(MSocket *m);
extern int lms_ssl_stopsock(MSocket *m);
extern int lms_ssl_unclean(MSocket *m);
extern int lms_ssl_read(MSocket *m);
extern int lms_ssl_handshake(MSocket *m);
extern int lms_ssl_flushq(MSocket *m);
extern char *lms_ssl_getclientinfo(MSocket *m);

/* rand.c */
extern int lms_rand_get(size_t bytes, unsigned char *dst);

/* str.c */
extern void lms_str_memnuke(volatile void *b, size_t sz);
extern int lms_str_splitchr(const char *str, int c, char **mstr);
extern int lms_str_splitnum(const char *str, int c, char **mstr, unsigned int num);
extern unsigned int lms_str_cntchr(const char *str, int c);
extern unsigned int lms_str_haschr(const char *str, int c);
extern size_t lms_str_arraycat(char **src, char *dst, size_t dst_len);
extern int lms_str_toupper(char *str);
extern int lms_str_tolower(char *str);
extern int lms_str_cnttochar(char *str, char c);
extern inline void lms_str_freearray(void **array, unsigned char fa);
extern void lms_str_copy(void *src, void *dst, size_t len);
extern void lms_str_ocopy(void *src, void *dst, size_t len, unsigned int offset);
extern unsigned int lms_str_ltochr(unsigned char *str, char chr, unsigned int maxlen);
extern size_t lms_str_getlen(void *str);
extern void free_array(void **ptr);

#endif /* INCLUDED_MSOCKETINTERNAL_H */
