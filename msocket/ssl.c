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


static SSL *_lms_ssl_list[LMS_HIGHSOCK];
static SSL_CTX *_lms_ssl_ctx;
static SSL_CTX *_lms_ssl_clientctx;
static lms_ssl_store *_lms_ssl_masterstore;
#ifndef LMS_SSLV2
static char _lms_ssl_sciphers[] = "-ALL:DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA:AES256-SHA:DHE-RSA-CAMELLIA256-SHA:DHE-DSS-CAMELLIA256-SHA:CAMELLIA256-SHA:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA:AES128-SHA:DHE-RSA-CAMELLIA128-SHA:DHE-DSS-CAMELLIA128-SHA:CAMELLIA128-SHA";
static char _lms_ssl_cciphers[] = "-ALL:DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA:AES256-SHA:DHE-RSA-CAMELLIA256-SHA:DHE-DSS-CAMELLIA256-SHA:CAMELLIA256-SHA:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA:AES128-SHA:DHE-RSA-CAMELLIA128-SHA:DHE-DSS-CAMELLIA128-SHA:CAMELLIA128-SHA:IDEA-CBC-SHA:RC4-SHA:EDH-RSA-DES-CBC3-SHA:EDH-DSS-DES-CBC3-SHA:DES-CBC3-SHA";
static char _lms_ssl_jciphers[] = "ALL:!ADH:@STRENGTH";
#else
static char _lms_ssl_sciphers[] = "ALL:!ADH:@STRENGTH";
static char _lms_ssl_cciphers[] = "ALL:!ADH:@STRENGTH";
#endif /* LMS_SSLV2 */

static lms_ssl_store *_lms_ssl_loadfiles(X509 *ca, const char *path);
static int _lms_ssl_freestore(lms_ssl_store *s);
static int _lms_ssl_getbiofd(BIO *b);


/*
 * lms_ssl_init() initializes the SSL functionality of NGCast
 *
 */
int lms_ssl_init()
{
	unsigned int i;
	unsigned char *buffer;
	X509_STORE *serverstore;
	X509_STORE *clientstore;

	serverstore = 0;
	clientstore = 0;

	SSL_library_init();
	ERR_load_crypto_strings();

	buffer = (unsigned char *)malloc(LMS_SSL_SEEDLEN);
	if (!buffer)
	{
		return(-1);
	}
	while (RAND_status() != 1)
	{
		memset(buffer, 0, LMS_SSL_SEEDLEN);

		if (lms_rand_get(LMS_SSL_SEEDLEN, buffer) < 0)
		{
			return(-1);
		}

		RAND_seed(buffer, LMS_SSL_SEEDLEN);
	}
	free(buffer);

	for (i = 0; i < LMS_HIGHSOCK; ++i)
	{
		_lms_ssl_list[i] = (SSL *)NULL;
	}

	SSL_load_error_strings();

	// _lms_ssl_masterstore = _lms_ssl_loadfiles(Config->ssl_files);

	/* Server CTX */
	_lms_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!_lms_ssl_ctx)
	{
		char ssl_error_buf[128];

		ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
		return(-1);
	}
	if (SSL_CTX_set_cipher_list(_lms_ssl_ctx, _lms_ssl_sciphers) == 0)
	{
#ifndef LMS_SSLV2
		/* If we can't use our best cipher set, try enabling some less awesome ciphers like 3DES and RC4. */
		if (SSL_CTX_set_cipher_list(_lms_ssl_ctx, _lms_ssl_cciphers) == 0)
		{
			/* Try junk ciphers only as a last resort! */
			if (SSL_CTX_set_cipher_list(_lms_ssl_ctx, _lms_ssl_jciphers) == 0)
			{
				return(-1);
			}
			else
			{
			}
		}
#else
		return(-1);
#endif /* LMS_SSLV2 */
	}
#ifdef LMS_SSLV2
	SSL_CTX_set_options(_lms_ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE|SSL_OP_SINGLE_DH_USE);
#else
	SSL_CTX_set_options(_lms_ssl_ctx, SSL_OP_NO_SSLv2|SSL_OP_CIPHER_SERVER_PREFERENCE|SSL_OP_SINGLE_DH_USE);
#endif /* LMS_SSLV2 */
	SSL_CTX_set_verify(_lms_ssl_ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_set_mode(_lms_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	// SSL_CTX_set_cert_store(_lms_ssl_ctx, serverstore);
	// SSL_CTX_set_ex_data(_lms_ssl_ctx, 0, );

	/* Client CTX */
	_lms_ssl_clientctx = SSL_CTX_new(SSLv23_client_method());
	if (!_lms_ssl_clientctx)
	{
		char ssl_error_buf[128];

		ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
		SSL_CTX_free(_lms_ssl_ctx);
		return(-1);
	}
	if (SSL_CTX_set_cipher_list(_lms_ssl_clientctx, _lms_ssl_cciphers) == 0)
	{
#ifndef LMS_SSLV2
		/* Try junk ciphers only as a last resort! */
		SSL_CTX_set_cipher_list(_lms_ssl_clientctx, _lms_ssl_jciphers);
#else
		return(-1);
#endif /* LMS_SSLV2 */
	}
#ifdef LMS_SSLV2
	SSL_CTX_set_options(_lms_ssl_clientctx, SSL_OP_SINGLE_DH_USE);
#else
	SSL_CTX_set_options(_lms_ssl_clientctx, SSL_OP_NO_SSLv2|SSL_OP_SINGLE_DH_USE);
#endif /* LMS_SSLV2 */
	SSL_CTX_set_verify(_lms_ssl_clientctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_set_mode(_lms_ssl_clientctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	// SSL_CTX_set_cert_store(_lms_ssl_ctx, clientstore);
	// SSL_CTX_set_ex_data(_lms_ssl_clientctx, 0, );

	return(0);
}

/*
 * lms_ssl_startsock() initiates SSL on socket m
 *
 * m = the socket on which to begin SSL processing
 *
 */
int lms_ssl_startsock(MSocket *m)
{
	SSL *s;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (m->fd < 0)
	{
		errno = EINVAL;
		return(-1);
	}
	else if ((m->type != LMSTYPE_STREAM4) && (m->type != LMSTYPE_STREAM6) && (m->type != LMSTYPE_LOCALCLIENT))
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->opts & LMSOPTION_SSL))
	{
		errno = ENODEV;
		return(-1);
	}

	if (m->flags & LMSFLG_INBOUND)
	{
		s = SSL_new(_lms_ssl_ctx);
	}
	else if (m->flags & LMSFLG_OUTBOUND)
	{
		s = SSL_new(_lms_ssl_clientctx);
	}
	else
	{
		return(-1);
	}

	if (!s)
	{
		char *ssl_error_buf;

		ssl_error_buf = (char *)malloc(128);
		if (!ssl_error_buf)
		{
			return(-1);
		}
		memset(ssl_error_buf, 0, 128);
		ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
		return(-1);
	}

	if (!SSL_set_fd(s, m->fd))
	{
		char *ssl_error_buf;

		ssl_error_buf = (char *)malloc(128);
		if (!ssl_error_buf)
		{
			return(-1);
		}
		memset(ssl_error_buf, 0, 128);
		ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
		SSL_free(s);
		return(-1);
	}

	_lms_ssl_list[m->fd] = s;

	m->flags |= LMSFLG_SSL;

	if (m->opts & LMSOPTION_UCREP)
	{
		lms_ssl_unclean(m);
	}

	m->func_w = lms_ssl_handshake;
	m->func_r = lms_ssl_handshake;
	m->flags |= LMSFLG_SSLHDSHK;
	if (m->flags & LMSFLG_INBOUND)
	{
		SSL_set_accept_state(s);
	}
	else if (m->flags & LMSFLG_OUTBOUND)
	{
		SSL_set_connect_state(s);
	}
	else
	{
	}
	lms_ssl_handshake(m);

	return(0);
}

/*
 * lms_ssl_closesock() sends the SSL shutdown string and such
 *
 * m = the socket
 *
 */
int lms_ssl_closesock(MSocket *m)
{
	int sav;
	int sdval;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_SSL))
	{
		return(0);
	}
	else if (!(m->flags & LMSFLG_SSLRDY) && !(m->flags & LMSFLG_SSLHDSHK))
	{
		return(0);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_CONNECTED))
	{
		errno = ENOTCONN;
		return(-1);
	}

	sav = SSL_shutdown(_lms_ssl_list[m->fd]);
	if (sav <= 0)
	{
		/* I don't care if this succeeded or failed, however the error queue on OpenSSL's end must be kept clear if it failed. */
		SSL_get_error(_lms_ssl_list[m->fd], sav);
	}
	sdval = SSL_get_shutdown(_lms_ssl_list[m->fd]);
	if (!(sdval & SSL_SENT_SHUTDOWN))
	{
		SSL_set_shutdown(_lms_ssl_list[m->fd], sdval|SSL_SENT_SHUTDOWN);
	}

	if (m->flags & LMSFLG_SSLRDY)
	{
		m->flags &= ~LMSFLG_SSLRDY;
	}
	if (m->flags & LMSFLG_SSLHDSHK)
	{
		m->flags &= ~LMSFLG_SSLHDSHK;
	}

	return(0);
}

/*
 * lms_ssl_stopsock() ends SSL on a socket, freeing all structures and removing list entries
 *
 * m = the socket
 *
 */
int lms_ssl_stopsock(MSocket *m)
{
	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_SSL))
	{
		return(0);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return(-1);
	}

	SSL_free(_lms_ssl_list[m->fd]);
	_lms_ssl_list[m->fd] = (SSL *)NULL;
	if (m->flags & LMSFLG_SSLRDY)
	{
		m->flags &= ~LMSFLG_SSLRDY;
	}
	if (m->flags & LMSFLG_SSLHDSHK)
	{
		m->flags &= ~LMSFLG_SSLHDSHK;
	}
	m->flags &= ~LMSFLG_SSL;

	return(0);
}

/*
 * lms_ssl_unclean() sets a socket as `unclean', meaning it will connect to an unknown (non-NGCast) remote application, and OpenSSL's 
 *                         broken-implementation-compatability bug options should be set.  
 *
 * m = the socket on which to set the unclean flag
 *
 */
int lms_ssl_unclean(MSocket *m)
{
	long curopts;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}

	m->opts |= LMSOPTION_UCREP;

	if (!(m->flags & LMSFLG_SSL))
	{
		/*
		 * This function can be called to set the flag before the socket is initialized,
		 * and lms_ssl_startsock() will call this function again to actually set the
		 * OpenSSL options on the (SSL *) pointer for the socket once it is initialized
		 * if that flag is set. 
		 */
		return(0);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return(-1);
	}

	SSL_set_ssl_method(_lms_ssl_list[m->fd], SSLv23_method());
	curopts = SSL_get_options(_lms_ssl_list[m->fd]);
#ifndef LMS_SSLV2
	if (!(curopts & SSL_OP_NO_SSLv2))
	{
		curopts |= SSL_OP_NO_SSLv2;
	}
#endif
	if ((m->flags & LMSFLG_OUTBOUND) && (curopts & SSL_OP_CIPHER_SERVER_PREFERENCE))
	{
		curopts &= ~SSL_OP_CIPHER_SERVER_PREFERENCE;
	}
	/* - SSL_OP_TLS_ROLLBACK_BUG ; Note that we don't set the MSIE/Microsoft related compatability bug-workarounds here. ;) */
	SSL_set_options(_lms_ssl_list[m->fd], curopts|SSL_OP_NETSCAPE_CHALLENGE_BUG|SSL_OP_TLS_D5_BUG|SSL_OP_TLS_BLOCK_PADDING_BUG|SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);

	return(1);
}

/*
 * lms_ssl_read() is the SSL connection input processor
 *
 * m = the socket on which to receive data
 *
 */
int lms_ssl_read(MSocket *m)
{
	unsigned char *c;
	int sav;
	int sav_x;
	size_t bufsz;
	unsigned char callpfunc;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_CONNECTED))
	{
		errno = ENOTCONN;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_SSL))
	{
		errno = EINVAL;
		return(-1);
	}
	else if ((m->type != LMSTYPE_STREAM4) && (m->type != LMSTYPE_STREAM6))
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!_lms_ssl_list[m->fd])
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
		sav = SSL_read(_lms_ssl_list[m->fd], c, bufsz);

		sav_x = SSL_get_shutdown(_lms_ssl_list[m->fd]);
		if (sav_x & SSL_RECEIVED_SHUTDOWN)
		{
			lms_ssl_closesock(m);
			m->func_e(m);
			return(0);
		}

		if (sav <= 0)
		{
			sav_x = SSL_get_error(_lms_ssl_list[m->fd], sav);
			if ((sav_x == SSL_ERROR_WANT_READ) || (sav_x == SSL_ERROR_WANT_WRITE))
			{
				break;
			}
			else if (sav_x == SSL_ERROR_ZERO_RETURN)
			{
				m->func_e(m);
				return(0);
			}
			else
			{
				if (sav_x == SSL_ERROR_SSL)
				{
					char *ssl_error_buf;

					ssl_error_buf = (char *)malloc(128);
					if (!ssl_error_buf)
					{
						return(-1);
					}
					memset(ssl_error_buf, 0, 128);
					ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
					free(ssl_error_buf);
				}

				m->func_e(m);
				return(0);
			}

			break;
                }
		else
		{
			lms_str_ocopy(c, m->recvQ, sav, m->recvQ_len);
			m->recvQ_len += sav;
			memset(c, 0, bufsz);
			callpfunc = 1;
			m->bytes_r += sav;
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

	m->last_recv = time(NULL);
	if (callpfunc)
	{
		m->func_p(m);
	}

	free(c);
	return(0);
}

/*
 * lms_ssl_handshake() is an input processor that deals with the handshake phase
 *
 * m = the socket on which to receive data
 *
 */
int lms_ssl_handshake(MSocket *m)
{
	int sav;
	int sav_x;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return(-1);
	}

	sav = SSL_do_handshake(_lms_ssl_list[m->fd]);

	if (sav <= 0)
	{
		sav_x = SSL_get_error(_lms_ssl_list[m->fd], sav);
		if ((sav_x == SSL_ERROR_WANT_READ) || (sav_x == SSL_ERROR_WANT_WRITE))
		{
			return(0);
		}
		else
		{
			if (sav_x == SSL_ERROR_SSL)
			{
				char *ssl_error_buf;

				ssl_error_buf = (char *)malloc(128);
				if (!ssl_error_buf)
				{
					return(-1);
				}
				memset(ssl_error_buf, 0, 128);
				ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
				free(ssl_error_buf);
			}

			lms_socket_destroy(m);
			return(0);
		}
	}
	else
	{
		/* Handshake completed successfully and we can move on */
		m->flags &= ~LMSFLG_SSLHDSHK;
		m->flags |= LMSFLG_SSLRDY;
		m->func_w = lms_ssl_flushq;
		m->func_r = lms_ssl_read;
		m->last_recv = time(NULL);
	}

	return(0);
}

/*
 * lms_ssl_flushq() flushes the sendQ of an SSL socket
 *
 * m = the socket to flush the sendQ of
 *
 */
int lms_ssl_flushq(MSocket *m)
{
	ssize_t rv;
	ssize_t sl;
	unsigned char *tmpptr;
	size_t tmplen;
	int sav_x;

	if (!m)
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_SSL))
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return(-1);
	}
	else if ((m->type != LMSTYPE_STREAM4) && (m->type != LMSTYPE_STREAM6) && (m->type != LMSTYPE_LOCALCLIENT))
	{
		errno = EINVAL;
		return(-1);
	}
	else if (!(m->flags & LMSFLG_CONNECTED))
	{
		errno = ENOTCONN;
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
		rv = SSL_write(_lms_ssl_list[m->fd], tmpptr, tmplen);
		if (rv <= 0)
		{
			sav_x = SSL_get_error(_lms_ssl_list[m->fd], rv);
			if ((sav_x == SSL_ERROR_WANT_READ) || (sav_x == SSL_ERROR_WANT_WRITE))
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
				if (sav_x == SSL_ERROR_SSL)
				{
					char *ssl_error_buf;

					ssl_error_buf = (char *)malloc(128);
					if (!ssl_error_buf)
					{
						return(-1);
					}
					memset(ssl_error_buf, 0, 128);
					ERR_error_string_n(ERR_get_error(), ssl_error_buf, 128);
					free(ssl_error_buf);
				}

				if (sl > 0)
				{
					lms_socket_clearsq(m, sl);
					m->sendQ_len -= sl;
					m->last_send = time(NULL);
				}
				return((int)sl);
			}
		}
		else
		{
			sl += rv;
			tmplen -= rv;
			if (tmplen > 0)
			{
				tmpptr += rv;
			}
			m->bytes_s += rv;
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
 * lms_ssl_getclientinfo() returns a pointer to a string containing a bunch of info about the client's SSL connectivity
 *                            + Allocates memory which must be later free()'d by the caller
 *
 * m = the socket about which to get SSL statistics and information
 *
 */
char *lms_ssl_getclientinfo(MSocket *m)
{
	char *infobuf;
	SSL_CIPHER *clientcipher;
	int useless_bits;

	if (!m)
	{
		errno = EINVAL;
		return((char *)NULL);
	}
	else if (!(m->flags & LMSFLG_SSL))
	{
		errno = EINVAL;
		return((char *)NULL);
	}
	else if (!_lms_ssl_list[m->fd])
	{
		errno = EINVAL;
		return((char *)NULL);
	}

	/* 48 bytes for this is a wee bit arbitrary, but seems like a good choice, the string is unlikely to be truncated, unless someone comes up with a cipher and gives it a really long name..... */
	infobuf = (char *)NULL;
#ifdef LMS_HARDCORE_ALLOC
	while (!infobuf)
	{
		infobuf = (char *)malloc(48);
	}
#else
	infobuf = (char *)malloc(48);
	if (!infobuf)
	{
		return((char *)NULL);
	}
#endif
	memset(infobuf, 0, 48);

	clientcipher = SSL_get_current_cipher(_lms_ssl_list[m->fd]);
	snprintf(infobuf, 48, "%s (%s/%i)", SSL_CIPHER_get_version(clientcipher), SSL_CIPHER_get_name(clientcipher), SSL_CIPHER_get_bits(clientcipher, &useless_bits));

	return(infobuf);
}

/*
 * _lms_ssl_loadfiles() loads a public/private keypair
 *
 * ca = the CA root cert
 * path = the path (will be appended with '.pub' for the public key, '.priv' for the private key, and '.crt' for the X509 certificate)
 *
 */
lms_ssl_store *_lms_ssl_loadfiles(X509 *ca, const char *path)
{
	char *pubpath;
	char *privpath;
	lms_ssl_store *ks;

	pubpath = (char *)malloc(MAXPATHLEN + 1);
	if (!pubpath)
	{
		return((lms_ssl_store *)NULL);
	}
	memset(pubpath, 0, (MAXPATHLEN + 1));
	snprintf(pubpath, MAXPATHLEN, "%s.pub", path);
	privpath = (char *)malloc(MAXPATHLEN + 1);
	if (!privpath)
	{
		free(pubpath);
		return((lms_ssl_store *)NULL);
	}
	memset(privpath, 0, (MAXPATHLEN + 1));
	snprintf(privpath, MAXPATHLEN, "%s.priv", path);

	ks = (lms_ssl_store *)NULL;
#ifdef LMS_HARDCORE_ALLOC
	while (!ks)
	{
		ks = (lms_ssl_store *)malloc(sizeof(lms_ssl_store));
	}
#else
	ks = (lms_ssl_store *)malloc(sizeof(lms_ssl_store));
	if (!ks)
	{
		free(pubpath);
		free(privpath);
		return((lms_ssl_store *)NULL);
	}
#endif
	memset(ks, 0, sizeof(lms_ssl_store));

	/*
	 * ca = CA cert
	 * crt = my cert
	 * privkey = RSA private key
	 * pubkey = RSA public key
	 * dhp = DH params
	 */

	memcpy(ks->ca, ca, sizeof(X509));

	ks->crt = X509_new();
	ks->privkey = RSA_new();
	ks->pubkey = RSA_new();
	ks->dhp = DH_new();

	return(ks);
}

/*
 * _lms_ssl_freestore() frees a key/cert store
 *
 * s = the lms_ssl_store object to free
 *
 */
int _lms_ssl_freestore(lms_ssl_store *s)
{
	if (!s)
	{
		errno = EINVAL;
		return(-1);
	}

	if (s->crt)
	{
		X509_free(s->crt);
	}
	if (s->ca)
	{
		X509_free(s->ca);
	}

	free(s);

	return(0);
}

/*
 * _lms_ssl_getbiofd() returns the fd of a BIO object
 *
 * b = the OpenSSL BIO object to return the fd of
 *
 */
int _lms_ssl_getbiofd(BIO *b)
{
	int fd;

	if (!b)
	{
		errno = EINVAL;
		return(-1);
	}

	fd = (int)BIO_ctrl(b, BIO_C_GET_FD, (long)0, (void *)NULL);

	return(fd);
}
