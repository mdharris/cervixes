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
#include <sys/uio.h>
#include <fcntl.h>


/*
 * lms_rand_get()
 *
 * returns -1 on error, 0 on success
 *
 * bytes = number of bytes of entropy to obtain
 * dst = memory buffer in which to store the entropy obtained
 *
 */
int lms_rand_get(size_t bytes, unsigned char *dst)
{
	int fd;
	int fl;
	char *p;
	int pl;
	size_t len;

	if (!dst)
	{
		errno = EINVAL;
		return(-1);
	}

	fd = open(LMS_ENTROPYSOURCE, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
	{
		return(-1);
	}

	fl = fcntl(fd, F_GETFL);
	if (fcntl(fd, F_SETFL, fl|O_NONBLOCK) < 0)
	{
		close(fd);
		return(-1);
	}

	p = (char *)malloc(bytes + 1);
	if (!p)
	{
		close(fd);
		return(-1);
	}
	memset(p, 0, (bytes + 1));

	len = 0;

	while (len < bytes)
	{
		pl = read(fd, p, bytes);
		if (pl < 0)
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				continue;
			}
			else
			{
				close(fd);
				free(p);
				return(-1);
			}
		}

		if ((len + pl) <= bytes)
		{
			strncat((char *)dst, p, pl);
		}
		else
		{
			strncat((char *)dst, p, (bytes - len));
		}
		len += pl;
	}

	close(fd);
	free(p);
	return(0);
}
