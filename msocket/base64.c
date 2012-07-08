/*
 * Copyright (c) 2007 - 2008
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
#include <sys/uio.h>
#include <fcntl.h>


static const char _lms_base64_b64[64] =		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char _lms_base64_dtbl[256] = 	{
						-3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
						52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
						-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
						15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
						-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
						41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
						-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 
						};


/*
 * lms_base64_encode() encodes a string in Base64 (for authen_wpmu's password storage type, etc)
 *
 * src = the string to be encoded
 * dst = a buffer to store the output
 * len = length
 *
 */
unsigned char *lms_base64_encode(unsigned char *src, unsigned char *dst, size_t len)
{
	unsigned char *p;
	size_t mod;
	unsigned long i;

	if (!src || !dst || !len)
	{
		errno = EINVAL;
		return((unsigned char *)NULL);
	}

	i = 0;
	mod = len % 3;
	p = dst;

	while (i < (len - mod))
	{
		*p++ = _lms_base64_b64[src[i++] >> 2];
		*p++ = _lms_base64_b64[((src[i - 1] << 4) | (src[i] >> 4)) & 0x3f];
		*p++ = _lms_base64_b64[((src[i] << 2) | (src[i + 1] >> 6)) & 0x3f];
		*p++ = _lms_base64_b64[src[i + 1] & 0x3f];
		i += 2;
	}

	if (!mod)
	{
		*p = 0;
	}
	else
	{
		*p++ = _lms_base64_b64[src[i++] >> 2];
		*p++ = _lms_base64_b64[((src[i - 1] << 4) | (src[i] >> 4)) & 0x3f];
		if (mod == 1)
		{
			*p++ = '=';
			*p++ = '=';
			*p = 0;
		}
		else
		{
			*p++ = _lms_base64_b64[(src[i] << 2) & 0x3f];
			*p++ = '=';
			*p = 0;
		}
	}

	return(dst);
}

/*
 * lms_base64_decode() decodes a Base64 string back to whatever it was before
 *
 * src = the null-terminated string to be decoded
 * dst = a buffer to store the output
 *
 */
unsigned char *lms_base64_decode(unsigned char *src, unsigned char *dst)
{
	unsigned int x;
	unsigned int result;
	unsigned char *buf;
	unsigned char *p;
	unsigned char pad;

	if (!src || !dst)
	{
		errno = EINVAL;
		return((unsigned char *)NULL);
	}

	pad = 0;
	p = src;
	buf = (unsigned char *)malloc(3);
	if (!buf)
	{
		return((unsigned char *)NULL);
	}
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;

	while (!pad)
	{
		switch ((x = _lms_base64_dtbl[*p++]))
		{
			case -3:
			{
				/* Null-terminating byte */
				if (((p - 1) - src) % 4)
				{
					free(buf);
					errno = EILSEQ;
					return((unsigned char *)NULL);
				}
				return(dst);
			}
			case -2:
			{
				/* Padding */
				if (((p - 1) - src) % 4 < 2)
				{
					free(buf);
					errno = EILSEQ;
					return((unsigned char *)NULL);
				}
				else if (((p - 1) - src) % 4 == 2)
				{
					if (*p != '=')
					{
						free(buf);
						errno = EILSEQ;
						return((unsigned char *)NULL);
					}

					buf[2] = 0;
					pad = 2;
					result++;
					break;
				}
				else
				{
					pad = 1;
					result += 2;
					break;
				}
				free(buf);
				return(dst);
			}
			case -1:
			{
				/* Invalid character. */
				break;
			}
			default:
			{
				switch (((p - 1) - src) % 4)
				{
					case 0:
					{
						buf[0] = x << 2;
						break;
					}
					case 1:
					{
						buf[0] |= (x >> 4);
						buf[1] = x << 4;
						break;
					}
					case 2:
					{
						buf[1] |= (x >> 2);
						buf[2] = x << 6;
						break;
					}
					case 3:
					{
						buf[2] |= x;
						result += 3;
						for (x = 0; x < 3 - pad; x++)
						{
							*dst++ = buf[x];
						}
						break;
					}
				}
				break;
			}
		}
	}

	for (x = 0; x < 3 - pad; x++)
	{
		*dst++ = buf[x];
	}

	free(buf);
	return(dst);
}
