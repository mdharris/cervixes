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
#include <sys/time.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <evdns.h>


#if defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0)
static lms_DNSCache *_lms_dns_findinstatictable(const char *h, unsigned short type);
#endif /* defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0) */
static lms_DNSCache *_lms_dns_findincache(const char *h, unsigned short type);
static int _lms_dns_addcache(unsigned short type, char *ip, char *hostname, time_t toexp);
static int _lms_dns_remcache(lms_DNSCache *c, unsigned char rfl);
static unsigned int _lms_dns_getcacheslot(void);

extern void lms_dns_recv(int result, char type, int count, int ttl, void *addresses, void *arg);
extern void lms_dns_recvrevA(int result, char type, int count, int ttl, void *addresses, void *arg);
extern void lms_dns_recvrevB(int result, char type, int count, int ttl, void *addresses, void *arg);

#if defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0)
static lms_DNSCache **_lms_dns_statictable;
#endif /* defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0) */
static lms_DNSCache **_lms_dns_cache;


unsigned int lms_dns_activequeries;


/*
 * lms_dns_init() initializes the resolver and all caches, it must be called before any other dns functions
 *
 */
int lms_dns_init()
{
	unsigned int i;
	unsigned char *buffer;

	lms_dns_activequeries = 0;

#if defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0)
	_lms_dns_statictable = (lms_DNSCache **)calloc(LMS_MAXDNSSTATIC, sizeof(lms_DNSCache));
	if (!_lms_dns_statictable)
	{
		return(-1);
	}
	for (i = 0; i < LMS_MAXDNSSTATIC; ++i)
	{
		_lms_dns_statictable[i] = (lms_DNSCache *)NULL;
	}
#endif /* defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0) */

#ifndef LMS_NODNSCACHE
	_lms_dns_cache = (lms_DNSCache **)calloc(LMS_MAXDNSCACHE, sizeof(lms_DNSCache));
	if (!_lms_dns_cache)
	{
		return(-1);
	}
	for (i = 0; i < LMS_MAXDNSCACHE; ++i)
	{
		_lms_dns_cache[i] = (lms_DNSCache *)NULL;
	}
#endif

	while (RAND_status() != 1)
	{
		buffer = (unsigned char *)malloc(LMS_SSL_SEEDLEN);
		if (!buffer)
		{
			return(-1);
		}
		memset(buffer, 0, LMS_SSL_SEEDLEN);

		if (lms_rand_get(LMS_SSL_SEEDLEN, buffer) < 0)
		{
			return(-1);
		}

		RAND_seed(buffer, LMS_SSL_SEEDLEN);

		free(buffer);
	}

	evdns_resolv_conf_parse(DNS_OPTION_NAMESERVERS|DNS_OPTION_MISC, "/etc/resolv.conf");

	return(0);
}

/*
 * lms_dns_cleancache() is called from a timer and simply cleans the cache of expired entries
 *
 */
void lms_dns_cleancache()
{
	register unsigned int i;
	time_t now;

#ifdef LMS_NODNSCACHE
	return;
#endif

	now = time(NULL);

	for (i = 0; i < LMS_MAXDNSCACHE; ++i)
	{
		if (!_lms_dns_cache[i])
		{
			continue;
		}

		if (_lms_dns_cache[i]->expiry <= now)
		{
			_lms_dns_remcache(_lms_dns_cache[i], 0);
			_lms_dns_cache[i] = (lms_DNSCache *)NULL;
		}
	}
}

/* 
 * lms_dns_lookup() performs an A record lookup for a host.
 *                     Returns 1 if a cached entry is available immediately, 0 if waiting, or -1 on error.
 *
 * h = the host to lookup
 * a = an abstract object to hang on to
 *
 */
int lms_dns_lookup(const char *h, Abstract *a)
{
	lms_DNSCache *cacheent;

	cacheent = _lms_dns_findincache(h, LMS_DNS_TYPE_A);
	if (cacheent)
	{
		if (a->what == ABSTRACT_DNSREQUEST)
		{
			unsigned char *stringptr;

			stringptr = (unsigned char *)a->where;
			strncpy((char *)stringptr, cacheent->ip, LMS_LEN_V4ADDR);
		}
		else if (a->what == ABSTRACT_MSOCKET)
		{
			MSocket *msptr;

			msptr = (MSocket *)a->where;
			strncpy(msptr->remotehost, cacheent->ip, LMS_LEN_V4ADDR);
		}
		else if (a->what == ABSTRACT_CALLBACK)
		{
			a->how(a);
		}
		else
		{
			return(-1);
		}

		return(1);
	}
#if defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0)
	else if (((cacheent = _lms_dns_findinstatictable(h)) != (lms_DNSCache *)NULL))
	{
	}
#endif /* defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0) */
	else
	{
		if (evdns_resolve_ipv4(h, DNS_QUERY_NO_SEARCH, lms_dns_recv, (void *)a) == 0)
		{
			lms_dns_activequeries++;
		}
		else
		{
			return(-1);
		}
	}

	return(0);
}

/*
 * lms_dns_recv() is a callback for evdns
 *
 * result = DNS_ERR_NONE if successful, otherwise DNS_ERR_*
 * type = DNS_IPv4_A || DNS_PTR || DNS_IPv6_AAAA (should always be DNS_IPv4_A here)
 * count = number of records found
 * ttl = expiration for cache entry
 * addresses = the addr(s) found for the query
 * arg = the MSocket which we were dealing with if there was one, or (void *)NULL
 *
 */
void lms_dns_recv(int result, char type, int count, int ttl, void *addresses, void *arg)
{
	Abstract *aptr;
	uint32_t *raddrS;
	struct in_addr raddr;
	char *ipstr;

	lms_dns_activequeries--;

	if (!arg)
	{
		return;
	}

	aptr = (Abstract *)arg;

	if (result != DNS_ERR_NONE)
	{
		if (result == DNS_ERR_NOTEXIST)
		{
			/* Add negative cache entry */
			if (aptr->what == ABSTRACT_DNSREQUEST)
			{
				char *stringptr;

				stringptr = (char *)aptr->where;
				_lms_dns_addcache(LMS_DNS_TYPE_A, (char *)NULL, stringptr, 600);
			}
			else if (aptr->what == ABSTRACT_MSOCKET)
			{
				MSocket *msptr;

				msptr = (MSocket *)aptr->where;
				_lms_dns_addcache(LMS_DNS_TYPE_A, (char *)NULL, msptr->remotedns, ttl);
			}
			else if (aptr->what == ABSTRACT_CALLBACK)
			{
				MSocket *msptr;

				msptr = (MSocket *)aptr->where;
				_lms_dns_addcache(LMS_DNS_TYPE_A, (char *)NULL, msptr->remotedns, ttl);

				aptr->how(aptr);
			}
		}
		else
		{
			if (aptr->what == ABSTRACT_CALLBACK)
			{
				aptr->how(aptr);
			}
		}

		return;
	}

	if (type != DNS_IPv4_A)
	{
		return;
	}

	ipstr = (char *)malloc(16);
	if (!ipstr)
	{
		return;
	}
	memset(ipstr, 0, 16);

	raddrS = (uint32_t *)addresses;
	raddr.s_addr = raddrS[0];
	snprintf(ipstr, 16, "%s", inet_ntoa(raddr));

	if (aptr->what == ABSTRACT_DNSREQUEST)
	{
		char *stringptr;

		stringptr = (char *)aptr->where;
		_lms_dns_addcache(LMS_DNS_TYPE_A, ipstr, stringptr, ttl);
	}
	else if (aptr->what == ABSTRACT_MSOCKET)
	{
		MSocket *msptr;

		msptr = (MSocket *)aptr->where;
		strncpy(msptr->remotehost, ipstr, LMS_LEN_V4ADDR);
		_lms_dns_addcache(LMS_DNS_TYPE_A, ipstr, msptr->remotedns, ttl);
	}
	else if (aptr->what == ABSTRACT_CALLBACK)
	{
		MSocket *msptr;

		msptr = (MSocket *)aptr->where;
		_lms_dns_addcache(LMS_DNS_TYPE_A, ipstr, msptr->remotedns, ttl);

		aptr->how(aptr);
	}
	/* Add support for more Abstract types here so that we can do more complicated things with DNS responses */
}

/*
 * lms_dns_findrev() finds the reverse DNS of the IP which connected to us (lms_dns_recvrevA()) and validates it against an A record (lms_dns_recvrevB())
 *                      before finally having (m->remotedns) set to the DNS name of the remote IP addr
 *                      Return codes: 0 = waiting; 1 = cached positive; 2 = cached negative; 3 = cached mismatch
 *
 * m = the MSocket for which to find the reverse DNS of (m->remotehost)
 *
 */
int lms_dns_findrev(MSocket *m)
{
	lms_DNSCache *cacheent;

	m->flags |= LMSFLG_WAITDNS;

	cacheent = _lms_dns_findincache(m->remotehost, LMS_DNS_TYPE_PTR);
	if (cacheent)
	{
		lms_DNSCache *c;

		if (cacheent->negative)
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			if (m->possible_revdns)
			{
				free(m->possible_revdns);
				m->possible_revdns = (char *)NULL;
			}
			return(2);
		}

		c = _lms_dns_findincache(cacheent->host, LMS_DNS_TYPE_A);
		if (c)
		{
			if (c->negative)
			{
				m->flags &= ~LMSFLG_WAITDNS;
				m->remotedns = (char *)NULL;
				if (m->possible_revdns)
				{
					free(m->possible_revdns);
					m->possible_revdns = (char *)NULL;
				}
				return(3);
			}
			if (strcmp(c->ip, m->remotehost) == 0)
			{
				m->flags &= ~LMSFLG_WAITDNS;
				m->remotedns = (char *)malloc(strlen(cacheent->host) + 1);
				if (!m->remotedns)
				{
					return(-1);
				}
				memset(m->remotedns, 0, (strlen(cacheent->host) + 1));
				strncpy(m->remotedns, cacheent->host, strlen(cacheent->host));

				if (m->possible_revdns)
				{
					free(m->possible_revdns);
					m->possible_revdns = (char *)NULL;
				}

				return(1);
			}
			else
			{
				m->flags &= ~LMSFLG_WAITDNS;
				m->remotedns = (char *)NULL;
				if (m->possible_revdns)
				{
					free(m->possible_revdns);
					m->possible_revdns = (char *)NULL;
				}
				return(3);
			}
		}
		else
		{
			m->possible_revdns = (char *)malloc(strlen(cacheent->host) + 1);
			if (!m->possible_revdns)
			{
				return(-1);
			}
			memset(m->possible_revdns, 0, (strlen(cacheent->host) + 1));
			strncpy(m->possible_revdns, cacheent->host, strlen(cacheent->host));
			lms_dns_activequeries++;
			evdns_resolve_ipv4(cacheent->host, DNS_QUERY_NO_SEARCH, lms_dns_recvrevB, (void *)m);
		}
	}
	else
	{
		lms_dns_activequeries++;
		evdns_resolve_reverse(m->addr, DNS_QUERY_NO_SEARCH, lms_dns_recvrevA, (void *)m);
	}

	return(0);
}

/*
 * lms_dns_recvrevA() is a callback for evdns
 *
 * result = DNS_ERR_NONE if successful, otherwise DNS_ERR_*
 * type = DNS_IPv4_A || DNS_PTR || DNS_IPv6_AAAA (should always be DNS_PTR here)
 * count = number of records found
 * ttl = expiration for cache entry
 * addresses = the addr(s) found for the query
 * arg = the MSocket which we were dealing with
 *
 */
void lms_dns_recvrevA(int result, char type, int count, int ttl, void *addresses, void *arg)
{
	MSocket *m;
	char *ret_addr;
	lms_DNSCache *c;

	lms_dns_activequeries--;

	if (!arg)
	{
		return;
	}

	m = (MSocket *)arg;

	if (result != DNS_ERR_NONE)
	{
		if ((result == DNS_ERR_SERVERFAILED) || (result == DNS_ERR_NOTEXIST) || (result == DNS_ERR_UNKNOWN))
		{
			if (result == DNS_ERR_NOTEXIST)
			{
				/* Add negative cache entry */
				_lms_dns_addcache(LMS_DNS_TYPE_PTR, m->remotehost, (char *)NULL, 3600);
			}
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			m->possible_revdns = (char *)NULL;
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
		else
		{
			/* Another type of error occured, mayhaps try again? */
			if (m->retries == 0)
			{
				m->retries++;
				lms_dns_findrev(m);
				return;
			}
			else
			{
				m->flags &= ~LMSFLG_WAITDNS;
				m->remotedns = (char *)NULL;
				m->possible_revdns = (char *)NULL;
				if (m->retries > 0)
				{
					m->retries = 0;
				}
				return;
			}
		}
	}

	if (type != DNS_PTR)
	{
		if (m->retries == 0)
		{
			m->retries++;
			lms_dns_findrev(m);
			return;
		}
		else
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			m->possible_revdns = (char *)NULL;
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
	}

	ret_addr = (char *)addresses;	/* searched for PTR, should be a hostname if it exists */
	m->possible_revdns = (char *)malloc(strlen(ret_addr) + 1);
	if (!m->possible_revdns)
	{
		return;
	}
	memset(m->possible_revdns, 0, (strlen(ret_addr) + 1));
	strncpy(m->possible_revdns, ret_addr, strlen(ret_addr));

	_lms_dns_addcache(LMS_DNS_TYPE_PTR, m->remotehost, m->possible_revdns, ttl);

	if (m->flags & LMSFLG_WAITDESTROY)
	{
		/*
		 * We want to get the result into the cache, but any additional processing
		 * is just a waste of time for a corpse. 
		 */
		m->flags &= ~LMSFLG_WAITDNS;
		return;
	}

	c = _lms_dns_findincache(ret_addr, LMS_DNS_TYPE_A);
	if (c)
	{
		if (c->negative)
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			if (m->possible_revdns)
			{
				free(m->possible_revdns);
				m->possible_revdns = (char *)NULL;
			}
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}

		if (strcmp(c->ip, m->remotehost) == 0)
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = m->possible_revdns;
			m->possible_revdns = (char *)NULL;
			return;
		}
		else
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			if (m->possible_revdns)
			{
				free(m->possible_revdns);
				m->possible_revdns = (char *)NULL;
			}
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
	}
	else
	{
		lms_dns_activequeries++;
		evdns_resolve_ipv4(m->possible_revdns, DNS_QUERY_NO_SEARCH, lms_dns_recvrevB, (void *)m);
	}
}

/*
 * lms_dns_recvrevB() is a callback for evdns
 *
 * result = DNS_ERR_NONE if successful, otherwise DNS_ERR_*
 * type = DNS_IPv4_A || DNS_PTR || DNS_IPv6_AAAA (should always be DNS_IPv4_A here)
 * count = number of records found
 * ttl = expiration for cache entry
 * addresses = the addr(s) found for the query
 * arg = the MSocket which we were dealing with
 *
 */
void lms_dns_recvrevB(int result, char type, int count, int ttl, void *addresses, void *arg)
{
	MSocket *m;
	uint32_t *raddrS;
	struct in_addr raddr;
	char *ipstr;

	lms_dns_activequeries--;

	if (!arg)
	{
		return;
	}

	m = (MSocket *)arg;

	if (!m->possible_revdns)
	{
		return;
	}

	if (result != DNS_ERR_NONE)
	{
		if ((result == DNS_ERR_SERVERFAILED) || (result == DNS_ERR_NOTEXIST) || (result == DNS_ERR_UNKNOWN))
		{
			if (result == DNS_ERR_NOTEXIST)
			{
				/* Add negative cache entry */
				_lms_dns_addcache(LMS_DNS_TYPE_A, (char *)NULL, m->possible_revdns, 600);
			}
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			free(m->possible_revdns);
			m->possible_revdns = (char *)NULL;
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
		else
		{
			/* Another type of error occured, mayhaps try again? */
			if (m->retries == 0)
			{
				free(m->possible_revdns);
				m->possible_revdns = (char *)NULL;
				m->retries++;
				lms_dns_findrev(m);
				return;
			}
		}
	}

	if (type != DNS_IPv4_A)
	{
		free(m->possible_revdns);
		m->possible_revdns = (char *)NULL;

		if (m->retries == 0)
		{
			m->retries++;
			lms_dns_findrev(m);
			return;
		}
		else
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
	}

	if (count != 1)
	{
		free(m->possible_revdns);
		m->possible_revdns = (char *)NULL;

		if (m->retries == 0)
		{
			m->retries++;
			lms_dns_findrev(m);
			return;
		}
		else
		{
			m->flags &= ~LMSFLG_WAITDNS;
			m->remotedns = (char *)NULL;
			if (m->retries > 0)
			{
				m->retries = 0;
			}
			return;
		}
	}

	/* Convert the IP in addresses to a string, then compare it against m->remotehost, and if it matches, set positive cache and m->remotedns */
	ipstr = (char *)malloc(16);
	if (!ipstr)
	{
		return;
	}
	memset(ipstr, 0, 16);

	raddrS = (uint32_t *)addresses;
	raddr.s_addr = raddrS[0];
	snprintf(ipstr, 16, "%s", inet_ntoa(raddr));

	_lms_dns_addcache(LMS_DNS_TYPE_A, ipstr, m->possible_revdns, ttl);

	if (m->flags & LMSFLG_WAITDESTROY)
	{
		/*
		 * We want to get the result into the cache, but any additional processing
		 * is just a waste of time for a corpse. 
		 */
		m->flags &= ~LMSFLG_WAITDNS;
		return;
	}

	if (strcmp(ipstr, m->possible_revdns) == 0)
	{
		m->flags &= ~LMSFLG_WAITDNS;
		m->remotedns = m->possible_revdns;
		m->possible_revdns = (char *)NULL;
	}
	else
	{
		m->flags &= ~LMSFLG_WAITDNS;
		free(m->possible_revdns);
		m->possible_revdns = (char *)NULL;
		m->remotedns = (char *)NULL;
	}

	if (m->retries > 0)
	{
		m->retries = 0;
	}
}

#if defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0)
/*
 * _lms_dns_findinstatictable() searches the static DNS table and returns a lms_DNSCache object if the specified record exists
 *
 * h = the IP or hostname to look for in the static table
 * type = the type of record to look for
 *
 */
lms_DNSCache *_lms_dns_findinstatictable(const char *h, unsigned short type)
{
	return((lms_DNSCache *)NULL);
}
#endif /* defined(LMS_MAXDNSSTATIC) && (LMS_MAXDNSSTATIC > 0) */

/*
 * _lms_dns_findincache() returns a lms_DNSCache object if the specified record is currently cached
 *
 * h = the IP or hostname to look for in the cache
 * type = the type of record to look for
 *
 */
lms_DNSCache *_lms_dns_findincache(const char *h, unsigned short type)
{
	register unsigned int i;

#ifdef LMS_NODNSCACHE
	return((lms_DNSCache *)NULL);
#endif

	for (i = 0; i < LMS_MAXDNSCACHE; ++i)
	{
		if (!_lms_dns_cache[i])
		{
			continue;
		}
		if (_lms_dns_cache[i]->type != type)
		{
			continue;
		}

		if (type == LMS_DNS_TYPE_PTR)
		{
			if (strcmp(_lms_dns_cache[i]->ip, h) == 0)
			{
				return(_lms_dns_cache[i]);
			}
		}
		if (type == LMS_DNS_TYPE_A)
		{
			if (strcasecmp(_lms_dns_cache[i]->host, h) == 0)
			{
				return(_lms_dns_cache[i]);
			}
		}
	}

	return((lms_DNSCache *)NULL);
}

/*
 * _lms_dns_addcache() adds an entry to the DNS cache
 *
 * type = the type of entry, currently only LMS_DNS_TYPE_A and LMS_DNS_TYPE_PTR are supported
 * ip = the ip address of the host (null-terminated)
 * hostname = the dns name of the host (null-terminated)
 *
 */
int _lms_dns_addcache(unsigned short type, char *ip, char *hostname, time_t toexp)
{
	lms_DNSCache *cache;
	unsigned int slot;
	unsigned short neg;

#ifdef LMS_NODNSCACHE
	return(0);
#endif

	if ((type != LMS_DNS_TYPE_A) && (type != LMS_DNS_TYPE_PTR))
	{
		errno = EPFNOSUPPORT;
		return(-2);
	}

	if (!ip && !hostname)
	{
		errno = EINVAL;
		return(-1);
	}
	else if ((ip && !hostname) && (type == LMS_DNS_TYPE_PTR))
	{
		neg = 1;
	}
	else if ((!ip && hostname) && (type == LMS_DNS_TYPE_A))
	{
		neg = 1;
	}
	else
	{
		neg = 0;
	}

	cache = (lms_DNSCache *)malloc(sizeof(lms_DNSCache));
	if (!cache)
	{
		return(-1);
	}
	memset(cache, 0, sizeof(lms_DNSCache));

	slot = _lms_dns_getcacheslot();
	if (slot == LMS_MAXDNSCACHE)
	{
		free(cache);
		return(-4);
	}

	cache->type = type;
	cache->host = (char *)malloc(strlen(hostname) + 1);
	if (!cache->host)
	{
		free(cache);
		return(-1);
	}
	memset(cache->host, 0, (strlen(hostname) + 1));
	if (hostname)
	{
		strncpy(cache->host, hostname, strlen(hostname));
	}

	memset(cache->ip, 0, 16);
	if (ip)
	{
		strncpy(cache->ip, ip, 16);
	}

	cache->negative = neg;

	if (toexp <= 0)
	{
		/* No ttl, or something weird like that.  Keep the cache entry for 10 minutes. */
		cache->expiry = (time(NULL) + 600);
	}
	else if (toexp <= 299)
	{
		/* Give myself at least 5 minutes to use the response. */
		cache->expiry = (time(NULL) + 300);
	}
	else if (toexp <= 86400)
	{
		cache->expiry = (time(NULL) + toexp);
	}
	else
	{
		/* > 86400  - I don't care to keep cache entries that damned long. */
		cache->expiry = (time(NULL) + 86400);
	}

	_lms_dns_cache[slot] = cache;

	return(0);
}

/*
 * _lms_dns_remcache() removes a DNS cache entry, for example if it is superceded or expires
 *
 * c = the cache entry to remove
 *
 */
int _lms_dns_remcache(lms_DNSCache *c, unsigned char rfl)
{
#ifdef LMS_NODNSCACHE
	return(0);
#endif

	if (rfl > 0)
	{
		register unsigned int i;

		for (i = 0; i < LMS_MAXDNSCACHE; ++i)
		{
			if (!_lms_dns_cache[i])
			{
				continue;
			}
			if (_lms_dns_cache[i] == c)
			{
				_lms_dns_cache[i] = (lms_DNSCache *)NULL;
				break;
			}
		}
	}

	if (c->host)
	{
		free(c->host);
	}
	free(c);

	return(0);
}

/*
 * _lms_dns_getcacheslot() returns an available cache slot in the DNS cache array
 *
 */
unsigned int _lms_dns_getcacheslot()
{
	register unsigned int i;
	unsigned int rv;

	for (i = 0; i < LMS_MAXDNSCACHE; ++i)
	{
		if (!_lms_dns_cache[i])
		{
			rv = i;
			return(rv);
		}
	}

	/* This specifies failure; do not add to the cache */
	return(LMS_MAXDNSCACHE);
}

/*
 * lms_dns_getip() returns the IP address from the cache which the host refers to
 *
 * host = the hostname to get the address of
 * buf = a buffer (16 bytes is good) in which to store the ip address, or (char *)NULL
 * buf_len = the size of ``buf''
 *
 */
int lms_dns_getip(const char *host, char *buf, size_t buf_len)
{
	lms_DNSCache *c;

	if (!host)
	{
		errno = EINVAL;
		return(-1);
	}

	c = _lms_dns_findincache(host, LMS_DNS_TYPE_A);
	if (!c)
	{
		return(-1);
	}
	if (c->negative)
	{
		return(0);
	}

	if (buf && (buf_len >= 16))
	{
		strncpy(buf, c->ip, buf_len);
	}

	return(1);
}

/*
 * lms_dns_gethost() returns the host from the cache which the ip refers to
 *
 * ip = the ip address to get the hostname of
 * buf = a buffer (256 bytes is good; evdns dumps any more than that before we even see it) in which to store the ip address, or (char *)NULL
 * buf_len = the size of ``buf''
 *
 */
int lms_dns_gethost(const char *ip, char *buf, size_t buf_len)
{
	lms_DNSCache *c;

	if (!ip)
	{
		errno = EINVAL;
		return(-1);
	}

	c = _lms_dns_findincache(ip, LMS_DNS_TYPE_PTR);
	if (!c)
	{
		return(-1);
	}
	if (c->negative)
	{
		return(0);
	}

	if (buf)
	{
		strncpy(buf, c->host, buf_len);
	}

	return(1);
}
