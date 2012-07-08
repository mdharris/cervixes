/*
 * Copyright (c) 2002 - 2008
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
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>


/*
 * lms_file_readln() reads a line from a file
 *                     may return >0 despite not reading a full line if read() encounters a fatal error or buf_len was not long enough for the line
 *
 * fd = the file to read from
 * buf = the buffer to store the line in
 * buf_len = the size of ``buf''
 *
 */
size_t lms_file_readln(int fd, char *buf, size_t buf_len)
{
	short rbytes;
	size_t tlen;
	char *tmpbuf;
	register unsigned int i;

	if (!buf)
	{
		return(0);
	}

	tmpbuf = (char *)malloc(2);
	if (!tmpbuf)
	{
		return(0);
	}
	tmpbuf[0] = 0;
	tmpbuf[1] = 0;

	i = 0;
	tlen = 0;
	while (tlen < buf_len)
	{
		rbytes = read(fd, tmpbuf, 1);
		if (rbytes < 0)
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				continue;
			}
			else
			{
				break;
			}
		}
		else if (rbytes == 0)
		{
			buf[i] = 0;
			i++;
			break;
		}

		if (tmpbuf[0] == '\n')
		{
			buf[i] = 0;
			i++;
			++tlen;
			break;
		}
		if (tmpbuf[0] == '\r')
		{
			continue;
		}
		else
		{
			buf[i] = tmpbuf[0];
			i++;
		}

		++tlen;
	}

	free(tmpbuf);

	return(tlen);
}

/*
 * lms_file_writepid() = Write pid to a file.
 *
 * fn = file name to write
 * pid = process id to write
 *
 */
int lms_file_writepid(const char *fn, pid_t pid)
{
	int f;
	char *pid_str;

	if (!fn)
	{
		return(-1);
	}

	pid_str = (char *)malloc(8);
	if (!pid_str)
	{
		return(-1);
	}
	memset(pid_str, 0, 8);
	snprintf(pid_str, 8, "%i\n", pid);
	f = open(fn, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if (f < 0)
	{
		free(pid_str);
		return(-1);
	}
	write(f, pid_str, strlen(pid_str));
	close(f);
	free(pid_str);
	return(0);
}

/*
 * lms_file_icanrw() = Returns 1 if I can read and write to a file, 0 if not, -1 on error.
 *
 * fs = stat structure for the file I'm checking
 *
 */
short lms_file_icanrw(struct stat *fs)
{
	uid_t myu;
	gid_t myg;
	gid_t *glist;
	gid_t l;
	register unsigned int i;
	int grpcnt;

	if (!fs)
	{
		return(-1);
	}

	myu = geteuid();

	if (myu == 0)
	{
		/* I'm [running as] root */
		return(1);
	}

	if (fs->st_uid == myu)
	{
		/* I own the file */
		if ((fs->st_mode & S_IRUSR) && (fs->st_mode & S_IWUSR))
		{
			return(1);
		}
	}

	if ((fs->st_mode & S_IROTH) && (fs->st_mode & S_IWOTH))
	{
		/* World writable */
		return(1);
	}

	myg = getegid();

	if ((fs->st_mode & S_IRGRP) && (fs->st_mode & S_IWGRP))
	{
		/* Group writable */

		if (fs->st_gid == myg)
		{
			/* My effective group owns it */
			return(1);
		}

		glist = (gid_t *)calloc(NGROUPS + 1, sizeof(gid_t));
		if (!glist)
		{
			return(-1);
		}

		if ((grpcnt = getgroups((NGROUPS + 1), glist)) < 0)
		{
			free(glist);
			return(-1);
		}
		l = -1;
		for (i = 0; i < grpcnt; ++i)
		{
			if (glist[i] == l)
			{
				continue;
			}
			if (fs->st_gid == glist[i])
			{
				free(glist);
				return(1);
			}
		}

		free(glist);
	}

	return(0);
}

/*
 * lms_file_icanr() = Returns 1 if I can read from a file, 0 if not, -1 on error.
 *
 * fs = stat structure for the file I'm checking
 *
 */
short lms_file_icanr(struct stat *fs)
{
	uid_t myu;
	gid_t myg;
	gid_t *glist;
	gid_t l;
	register unsigned int i;
	int grpcnt;

	if (!fs)
	{
		return(-1);
	}

	myu = geteuid();

	if (myu == 0)
	{
		/* I'm [running as] root */
		return(1);
	}

	if (fs->st_uid == myu)
	{
		/* I own the file */
		if (fs->st_mode & S_IRUSR)
		{
			return(1);
		}
	}

	if (fs->st_mode & S_IROTH)
	{
		/* World readable */
		return(1);
	}

	myg = getegid();

	if (fs->st_mode & S_IRGRP)
	{
		/* Group readable */

		if (fs->st_gid == myg)
		{
			/* My effective group owns it */
			return(1);
		}

		glist = (gid_t *)calloc(NGROUPS + 1, sizeof(gid_t));
		if (!glist)
		{
			return(-1);
		}

		if ((grpcnt = getgroups((NGROUPS + 1), glist)) < 0)
		{
			free(glist);
			return(-1);
		}
		l = -1;
		for (i = 0; i < grpcnt; ++i)
		{
			if (glist[i] == l)
			{
				continue;
			}
			if (fs->st_gid == glist[i])
			{
				free(glist);
				return(1);
			}
		}

		free(glist);
	}

	return(0);
}
