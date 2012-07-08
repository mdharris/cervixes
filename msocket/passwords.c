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

#define LMS_PASSWORDS_CURVER		1
#define LMS_PASSWORDS_PRTVER		'1'


static int _lms_passwords_hash(char *indata, unsigned char *outdata);
static unsigned int _lms_passwords_getsalt(unsigned char *ptr, unsigned char *s);


/*
 * lms_passwords_encode()
 *
 * returns -1 on error, 0 on success
 *
 * indata = pointer to the unencoded null-terminated password string
 * outdata = a 44 byte long (null-terminated) memory buffer in which to store the result IF use_b64 is 0
 *        OR an 84 byte long (null-terminated) memory buffer in which to store the result IF use_b64 is >0
 *
 */
int lms_passwords_encode(char *indata, char *outdata, unsigned short use_b64)
{
	char *prehash;
	unsigned char *hashed;
	unsigned char *salt;
	unsigned char *ver;
	size_t prelen;

	if (!indata || !outdata)
	{
		errno = EINVAL;
		return(-1);
	}

	ver = (unsigned char *)malloc(2);
	if (!ver)
	{
		return(-1);
	}
	if (use_b64 > 0)
	{
		ver[0] = LMS_PASSWORDS_PRTVER;
	}
	else
	{
		ver[0] = LMS_PASSWORDS_CURVER;
	}
	ver[1] = 0;

	prelen = strlen(indata) + 9;				/* length of string + 8 byte salt + null-terminating byte */
	prehash = (char *)malloc(prelen);
	if (!prehash)
	{
		free(ver);
		return(-1);
	}
	memset(prehash, 0, prelen);

	salt = (unsigned char *)malloc(9);
	if (!salt)
	{
		free(ver);
		free(prehash);
		return(-1);
	}
	memset(salt, 0, 9);
	lms_rand_get(8, salt);

	hashed = (unsigned char *)malloc(33);
	if (!hashed)
	{
		free(ver);
		free(prehash);
		free(salt);
		return(-1);
	}
	memset(hashed, 0, 33);

	snprintf(prehash, prelen, "%s%s", salt, indata);
	_lms_passwords_hash(prehash, hashed);
	free(prehash);

	if (use_b64 > 0)
	{
		char *b64hash;
		size_t b64hash_len;
		char *b64salt;
		size_t b64salt_len;

		ver[0] = LMS_PASSWORDS_PRTVER;

		b64hash_len = 65;				/* (sha256=32 * 2) + 1 for null-terminating byte */
		b64hash = (char *)malloc(b64hash_len);
		if (!b64hash)
		{
			free(hashed);
			free(salt);
			free(ver);
			return(-1);
		}
		memset(b64hash, 0, b64hash_len);
		lms_base64_encode(hashed, (unsigned char *)b64hash, 32);

		b64salt_len = 17;				/* (8 * 2) + 1 for null-terminating byte */
		b64salt = (char *)malloc(b64salt_len);
		if (!b64salt)
		{
			free(b64hash);
			free(hashed);
			free(salt);
			free(ver);
			return(-1);
		}
		memset(b64salt, 0, b64salt_len);
		lms_base64_encode(salt, (unsigned char *)b64salt, 8);

		/* ?|saltxxxx********|passwordxxxxxxxxpasswordxxxxxxxx******************************** = 83 bytes total */
		snprintf(outdata, 84, "%c|%s|%s", ver[0], b64salt, b64hash);

		free(b64hash);
		free(b64salt);
	}
	else
	{
		ver[0] = LMS_PASSWORDS_CURVER;

		/* ?|saltxxxx|passwordxxxxxxxxpasswordxxxxxxxx = 43 bytes total */
		snprintf(outdata, 44, "%c|%s|%s", ver[0], salt, hashed);
	}

	free(hashed);
	free(salt);
	free(ver);
	return(0);
}

/*
 * lms_passwords_len()
 *
 * returns the size a buffer must be to store an encoded string
 *
 * use_b64 = 0 if we are not base64 encoding, 1 if we are
 *
 */
size_t lms_passwords_len(unsigned short use_b64)
{
	if (use_b64 > 0)
	{
		/* This is a maximum, not an actual value.  Varies somewhat by how much padding is needed, etc. */
		return(84);
	}
	else
	{
		/* This is an actual value. */
		return(44);
	}
}

/*
 * lms_passwords_check()
 *
 * returns -1 on error, 0 if check fails, 1 if check succeeds
 *
 * chk = a null-terminated password string containing the password to check against the ``real''
 * real = the string in lms_passwords_encode() format for checking against
 *
 */
int lms_passwords_check(char *chk, const char *real, unsigned short is_b64)
{
	char *ver;
	unsigned char *salt;
	char *prehash;
	unsigned char *hash;
	unsigned char *rhash;
	size_t prelen;

	if (!chk || !real)
	{
		errno = EINVAL;
		return(-1);
	}

	ver = (char *)malloc(2);
	if (!ver)
	{
		return(-1);
	}
	salt = (unsigned char *)malloc(9);
	if (!salt)
	{
		free(ver);
		return(-1);
	}

	if (!is_b64)
	{
		ver[0] = real[0];
		ver[1] = 0;
		if (ver[0] > LMS_PASSWORDS_CURVER)
		{
			return(0);
		}
		else if (ver[0] < LMS_PASSWORDS_CURVER)
		{
			/* Backwards compatability goes here */
			switch (ver[0])
			{
				case 0:
				{
					break;
				}
				default:
				{
					break;
				}
			}

			return(0);
		}

		if ((real[1] != '|') || (real[10] != '|'))
		{
			errno = EINVAL;
			return(-1);
		}

		salt[0] = real[2];
		salt[1] = real[3];
		salt[2] = real[4];
		salt[3] = real[5];
		salt[4] = real[6];
		salt[5] = real[7];
		salt[6] = real[8];
		salt[7] = real[9];
		salt[8] = 0;

		prelen = strlen(chk) + 9;				/* length of string + 8 byte salt + null-terminating byte */
		prehash = (char *)malloc(prelen);
		if (!prehash)
		{
			free(ver);
			free(salt);
			return(-1);
		}
		memset(prehash, 0, prelen);
		hash = (unsigned char *)malloc(33);
		if (!hash)
		{
			free(ver);
			free(salt);
			free(prehash);
			return(-1);
		}
		memset(hash, 0, 33);

		snprintf(prehash, prelen, "%s%s", salt, chk);
		_lms_passwords_hash(prehash, hash);
		free(prehash);

		rhash = (unsigned char *)(real + 11);
		if (!rhash)
		{
			return(-1);
		}

		if (memcmp(hash, rhash, 32) == 0)
		{
			free(ver);
			free(salt);
			free(hash);
			return(1);
		}
		else
		{
			free(ver);
			free(salt);
			free(hash);
			return(0);
		}
	}
	else
	{
		unsigned int b64_salt_length;
		unsigned char *b64_salt;
		unsigned char *x;
		char *s;

		x = (unsigned char *)(real + 2);

		s = (char *)malloc(2);
		if (!s)
		{
			free(ver);
			free(salt);
			return(-1);
		}
		s[0] = 0;
		s[1] = 0;
		memcpy(s, real, 1);
		ver[0] = stringtoint(s);
		free(s);

		if (ver[0] > LMS_PASSWORDS_CURVER)
		{
			free(ver);
			free(salt);
			return(0);
		}
		else if (ver[0] < LMS_PASSWORDS_CURVER)
		{
			/* Backwards compatability goes here */
			switch (ver[0])
			{
				case 0:
				{
					break;
				}
				default:
				{
					break;
				}
			}

			return(0);
		}

		b64_salt = (unsigned char *)malloc(17);
		if (!b64_salt)
		{
			free(ver);
			free(salt);
			return(-1);
		}
		memset(b64_salt, 0, 17);
		b64_salt_length = _lms_passwords_getsalt(x, b64_salt);
		if (!lms_base64_decode(b64_salt, salt))
		{
			free(ver);
			free(salt);
			return(-1);
		}
		x += b64_salt_length;
		while (*x == '|')
		{
			/* This ought only happen once if at all, but just in case... */
			++x;
		}

		prelen = strlen(chk) + 9;				/* length of string + 8 byte salt + null-terminating byte */
		prehash = (char *)malloc(prelen);
		if (!prehash)
		{
			free(ver);
			free(salt);
			return(-1);
		}
		memset(prehash, 0, prelen);
		hash = (unsigned char *)malloc(33);
		if (!hash)
		{
			free(ver);
			free(salt);
			free(prehash);
			return(-1);
		}
		memset(hash, 0, 33);

		snprintf(prehash, prelen, "%s%s", salt, chk);
		_lms_passwords_hash(prehash, hash);
		free(prehash);

		rhash = (unsigned char *)malloc(lms_base64_dlen(strlen((char *)x)));
		if (!rhash)
		{
			free(ver);
			free(salt);
			free(hash);
			return(-1);
		}
		if (!lms_base64_decode(x, rhash))
		{
			free(ver);
			free(salt);
			free(hash);
			free(rhash);
			return(-1);
		}

		if (memcmp(hash, rhash, 32) == 0)
		{
			free(ver);
			free(salt);
			free(hash);
			free(rhash);
			return(1);
		}
		else
		{
			free(ver);
			free(salt);
			free(hash);
			free(rhash);
			return(0);
		}
	}

	return(0);
}

/*
 * lms_passwords_encodemulti()
 *
 * returns -1 on error, 0 on success
 *
 * indata = pointer to the unencoded null-terminated password string
 * outdata = an authentication data structure
 *
 */
int lms_passwords_encodemulti(char *indata, lms_passwords_data *outdata)
{
	char *prehash;
	unsigned char *hashed;
	unsigned char *salt;
	size_t prelen;
	char *b64hash;
	size_t b64hash_len;
	char *b64salt;
	size_t b64salt_len;

	if (!indata || !outdata)
	{
		errno = EINVAL;
		return(-1);
	}

	outdata->version = LMS_PASSWORDS_CURVER;

	prelen = strlen(indata) + 9;				/* length of string + 8 byte salt + null-terminating byte */
	prehash = (char *)malloc(prelen);
	if (!prehash)
	{
		return(-1);
	}
	memset(prehash, 0, prelen);

	salt = (unsigned char *)malloc(9);
	if (!salt)
	{
		free(prehash);
		return(-1);
	}
	memset(salt, 0, 9);
	lms_rand_get(8, salt);
	memcpy(outdata->salt, salt, 8);

	hashed = (unsigned char *)malloc(33);
	if (!hashed)
	{
		free(prehash);
		free(salt);
		return(-1);
	}
	memset(hashed, 0, 33);

	snprintf(prehash, prelen, "%s%s", salt, indata);
	_lms_passwords_hash(prehash, hashed);
	free(prehash);
	memcpy(outdata->hash, hashed, 32);

	b64hash_len = 65;				/* (sha256=32 * 2) + 1 for null-terminating byte */
	b64hash = (char *)malloc(b64hash_len);
	if (!b64hash)
	{
		free(hashed);
		free(salt);
		return(-1);
	}
	memset(b64hash, 0, b64hash_len);
	lms_base64_encode(hashed, (unsigned char *)b64hash, 32);
	strncpy(outdata->hash_b64, b64hash, 64);
	free(b64hash);

	b64salt_len = 17;				/* (8 * 2) + 1 for null-terminating byte */
	b64salt = (char *)malloc(b64salt_len);
	if (!b64salt)
	{
		free(b64hash);
		free(hashed);
		free(salt);
		return(-1);
	}
	memset(b64salt, 0, b64salt_len);
	lms_base64_encode(salt, (unsigned char *)b64salt, 8);
	strncpy(outdata->salt_b64, b64salt, 16);
	free(b64salt);

	free(hashed);
	free(salt);
	return(0);
}

/*
 * lms_passwords_checkmulti()
 *
 * returns -1 on error, 0 if check fails, 1 if check succeeds
 *
 * chk = a null-terminated password string containing the password to check against the ``real''
 * real = the string in lms_passwords_data structure format for checking against
 *
 */
int lms_passwords_checkmulti(char *chk, lms_passwords_data *real)
{
	char *prehash;
	unsigned char *hash;
	size_t prelen;

	if (!chk || !real)
	{
		errno = EINVAL;
		return(-1);
	}

	if (real->version > LMS_PASSWORDS_CURVER)
	{
		return(0);
	}
	else if (real->version < LMS_PASSWORDS_CURVER)
	{
		/* Backwards compatability goes here */
		switch (real->version)
		{
			case 0:
			{
				break;
			}
			default:
			{
				break;
			}
		}

		return(0);
	}

	prelen = strlen(chk) + 9;				/* length of string + 8 byte salt + null-terminating byte */
	prehash = (char *)malloc(prelen);
	if (!prehash)
	{
		return(-1);
	}
	memset(prehash, 0, prelen);
	hash = (unsigned char *)malloc(33);
	if (!hash)
	{
		free(prehash);
		return(-1);
	}
	memset(hash, 0, 33);

	snprintf(prehash, prelen, "%s%s", real->salt, chk);
	_lms_passwords_hash(prehash, hash);
	free(prehash);

	if (memcmp(hash, real->hash, 32) == 0)
	{
		free(hash);
		return(1);
	}
	else
	{
		free(hash);
		return(0);
	}

	return(0);
}

/*
 * lms_passwords_converttomulti()
 *
 * returns -1 on error, 0 on success
 *
 * indata = pointer to the password string (if it is Base64 encoded, it must be null-terminated)
 * outdata = an authentication data structure
 * is_b64 = 1 if indata is Base64 encoded, 0 if it is not
 *
 */
int lms_passwords_converttomulti(unsigned char *indata, lms_passwords_data *outdata, unsigned short is_b64)
{
	unsigned char *x;
	size_t b64hash_len;
	size_t b64salt_len;
	char *iverbuf;

	if (!indata || !outdata)
	{
		errno = EINVAL;
		return(-1);
	}

	if (is_b64)
	{
		x = indata;

		iverbuf = (char *)malloc(2);
		memcpy(iverbuf, x, 1);
		iverbuf[1] = 0;
		outdata->version = (unsigned char)stringtouint(iverbuf);
		free(iverbuf);

		if (outdata->version > LMS_PASSWORDS_CURVER)
		{
			return(0);
		}
		else if (outdata->version < LMS_PASSWORDS_CURVER)
		{
			/* Backwards compatability goes here */
			switch (outdata->version)
			{
				case 0:
				{
					break;
				}
				default:
				{
					break;
				}
			}

			return(0);
		}

		b64salt_len = _lms_passwords_getsalt(x, (unsigned char *)outdata->salt_b64);
		memset(outdata->salt_b64, 0, 17);
		if (!lms_base64_decode((unsigned char *)outdata->salt_b64, outdata->salt))
		{
			return(-1);
		}
		x += b64salt_len;
		while (*x == '|')
		{
			/* This ought only happen once if at all, but just in case... */
			++x;
		}
		memset(outdata->hash_b64, 0, 65);
		memcpy(outdata->hash_b64, x, 65);
		if (!lms_base64_decode((unsigned char *)outdata->hash_b64, outdata->hash))
		{
			return(-1);
		}
	}
	else
	{
		int r;

		x = indata;

		if ((indata[1] != '|') || (indata[10] != '|'))
		{
			errno = EINVAL;
			return(-1);
		}

		outdata->version = *x;

		if (outdata->version > LMS_PASSWORDS_CURVER)
		{
			return(0);
		}
		else if (outdata->version < LMS_PASSWORDS_CURVER)
		{
			/* Backwards compatability goes here */
			switch (outdata->version)
			{
				case 0:
				{
					break;
				}
				default:
				{
					break;
				}
			}

			return(0);
		}

		++x;
		while (*x == '|')
		{
			++x;
		}

		memcpy(outdata->salt, x, 8);

		r = 0;
		while (*x != '|')
		{
			++x;
			r++;
			if (r > 8)
			{
				/* Bad indata string */
				break;
			}
		}
		r = 0;
		while (*x == '|')
		{
			++x;
		}

		memcpy(outdata->hash, x, 32);

		b64salt_len = 17;				/* (8 * 2) + 1 for null-terminating byte */
		memset(outdata->salt_b64, 0, b64salt_len);
		lms_base64_encode(outdata->salt, (unsigned char *)outdata->salt_b64, 8);

		b64hash_len = 65;				/* (sha256=32 * 2) + 1 for null-terminating byte */
		memset(outdata->hash_b64, 0, b64hash_len);
		lms_base64_encode(outdata->hash, (unsigned char *)outdata->hash_b64, 32);

	}

	return(0);
}

/*
 * _lms_passwords_hash() creates a hash from indata and stores it in outdata
 *
 * indata = the data to hash
 * outdata = a buffer to store the result in !!assumed to be large enough!!
 *
 */
int _lms_passwords_hash(char *indata, unsigned char *outdata)
{
	EVP_MD_CTX *sha256_ctx;
	unsigned int zero;

	if (!indata || !outdata)
	{
		errno = EINVAL;
		return(-1);
	}

	sha256_ctx = EVP_MD_CTX_create();
	if (!sha256_ctx)
	{
		return(-1);
	}

	zero = 0;
	if (EVP_DigestInit_ex(sha256_ctx, EVP_sha256(), (ENGINE *)NULL) < 0) { EVP_MD_CTX_destroy(sha256_ctx); return(-1); }
	if (EVP_DigestUpdate(sha256_ctx, indata, strlen(indata)) < 0) { EVP_MD_CTX_destroy(sha256_ctx); return(-1); }
	if (EVP_DigestFinal_ex(sha256_ctx, outdata, &zero) < 0) { EVP_MD_CTX_destroy(sha256_ctx); return(-1); }

	EVP_MD_CTX_destroy(sha256_ctx);
	return(0);
}

/*
 * _lms_passwords_getsalt() pulls the base64 encoded salt out of the string.  Note: only useful when Base64 encoded!
 *
 * ptr = the buffer we're copying from
 * s = a buffer in which to store the result
 *
 */
unsigned int _lms_passwords_getsalt(unsigned char *ptr, unsigned char *s)
{
	unsigned int cnt;

	if (!ptr || !s)
	{
		errno = EINVAL;
		return(0);
	}

	for (cnt = 0; ptr[cnt] != '|'; ++cnt)
	{
		s[cnt] = ptr[cnt];
	}
	++cnt;

	return(cnt);
}
