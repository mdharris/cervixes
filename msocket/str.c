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


/*
 * lms_str_memnuke() clears a memory buffer for real
 *           cast b to (volatile) when calling
 *
 * b = pointer to memory buffer to be cleared
 * sz = size of buffer to be cleared
 *
 */
void lms_str_memnuke(volatile void *b, size_t sz)
{
	volatile char *xb;

	if (!b)
	{
		return;
	}

	for (xb = (volatile char *)b; sz; xb[--sz] = 0);
	return;
}

/*
 * lms_str_splitchr()
 *      + allocates memory for members of mstr and the pointers are stored in mstr which must be free()'d
 *
 * str = string to be split up
 * c = character on which to split the string
 * mstr = pointer to an array in which to store the strings after split
 *
 */
int lms_str_splitchr(const char *str, int c, char **mstr)
{
	register int i;
	int x;
	size_t offset;

	if (!str || !mstr)
	{
		return(-1);
	}

	offset = 0;
	x = 0;
	mstr[x] = (char *)malloc(strlen(str) + 1);
	if (!mstr[x])
	{
		return(-1);	/* we got nowhere... */
	}
	memset(mstr[x], 0, (strlen(str) + 1));
	for (i = 0; (str[i] != '\0'); ++i)
	{
		if (str[i] == c)
		{
			mstr[x][i - offset] = 0;
			offset += strlen(mstr[x]);
			offset += 1;
			x++;
			mstr[x] = (char *)malloc((strlen(str) - offset) + 1);
			if (!mstr[x])
			{
				mstr[x] = (char *)NULL;
				return(x - 1);
			}
			memset(mstr[x], 0, ((strlen(str) - offset) + 1));
		}
		else
		{
			mstr[x][i - offset] = str[i];
		}
	}
	mstr[x][(i - offset) + 1] = 0;
	mstr[x + 1] = (char *)NULL;

	return(x);
}

/*
 * lms_str_splitnum() acts similarly to lms_str_splitchr(), but has a limit on how many members it will roll out of ``str''
 *      + allocates memory for members of mstr and the pointers are stored in mstr which must be free()'d
 *
 * str = string to be split up
 * c = character on which to split the string
 * mstr = pointer to an array in which to store the strings after split
 * num = the maximum number of members to roll out of ``str''
 *
 */
int lms_str_splitnum(const char *str, int c, char **mstr, unsigned int num)
{
	register int i;
	int x;
	size_t offset;

	if (!str || !mstr || (num == 0))
	{
		return(-1);
	}

	offset = 0;
	x = 0;
	mstr[x] = (char *)malloc(strlen(str) + 1);
	if (!mstr[x])
	{
		return(-1);	/* we got nowhere... */
	}
	memset(mstr[x], 0, (strlen(str) + 1));
	for (i = 0; (str[i] != '\0'); ++i)
	{
		if (str[i] == c)
		{
			mstr[x][i - offset] = 0;
			offset += strlen(mstr[x]);
			offset += 1;
			x++;
			if (x >= num)
			{
				break;
			}
			mstr[x] = (char *)malloc((strlen(str) - offset) + 1);
			if (!mstr[x])
			{
				mstr[x] = (char *)NULL;
				return(x - 1);
			}
			memset(mstr[x], 0, ((strlen(str) - offset) + 1));
		}
		else
		{
			mstr[x][i - offset] = str[i];
		}
	}
	mstr[x][(i - offset) + 1] = 0;
	mstr[x + 1] = (char *)NULL;

	return(x);
}

/*
 * lms_str_cntchr()
 *
 * str = the string in which to count the characters
 * c = the character to count the instances of
 *
 */
unsigned int lms_str_cntchr(const char *str, int c)
{
	register unsigned int i;
	unsigned int cnt;

	if (!str)
	{
		return(0);
	}

	cnt = 0;
	for (i = 0; (str[i] != '\0'); ++i)
	{
		if (str[i] == c)
		{
			cnt++;
		}
	}

	return(cnt);
}

/*
 * lms_str_haschr()
 *
 * str = the string in which to count the characters
 * c = the character to count the instances of
 *
 */
unsigned int lms_str_haschr(const char *str, int c)
{
	register unsigned int i;

	if (!str)
	{
		return(0);
	}

	for (i = 0; (str[i] != '\0'); ++i)
	{
		if (str[i] == c)
		{
			return(1);
		}
	}

	return(0);
}

/*
 * lms_str_arraycat()
 *
 * src = array to concatenate
 * dst = destination buffer
 * dst_len = the size of dst
 *
 */
size_t lms_str_arraycat(char **src, char *dst, size_t dst_len)
{
	unsigned int i;
	size_t tlen;

	if (!src || !dst)
	{
		return(0);
	}

	tlen = 0;
	for (i = 0; src[i]; ++i)
	{
		if ((tlen + strlen(src[i])) >= dst_len)
		{
			break;
		}
		strncat(dst, src[i], strlen(src[i]));
		tlen += strlen(src[i]);
	}

	return(tlen);
}

/*
 * lms_str_toupper() converts a string to all upper-case
 *
 * str = the string to convert
 *
 */
int lms_str_toupper(char *str)
{
	register unsigned int i;
	char c;

	if (!str)
	{
		errno = EINVAL;
		return(-1);
	}

	for (i = 0; str[i]; ++i)
	{
		/*
		** 32=" " 33="!" 34=""" 35="#" 36="$" 37="%" 38="&" 39="'" 40="(" 41=")" 42="*" 43="+" 44="," 45="-" 46="." 47="/" 
		** 48="0" 49="1" 50="2" 51="3" 52="4" 53="5" 54="6" 55="7" 56="8" 57="9" 58=":" 59=";" 60="<" 61="=" 62=">" 63="?" 64="@" 
		** 65="A" 66="B" 67="C" 68="D" 69="E" 70="F" 71="G" 72="H" 73="I" 74="J" 75="K" 76="L" 77="M" 78="N" 79="O" 80="P" 81="Q" 82="R" 83="S" 84="T" 85="U" 86="V" 87="W" 88="X" 89="Y" 90="Z" 
		** 91="[" 92="\" 93="]" 94="^" 95="_" 96="`" 
		** 97="a" 98="b" 99="c" 100="d" 101="e" 102="f" 103="g" 104="h" 105="i" 106="j" 107="k" 108="l" 109="m" 110="n" 111="o" 112="p" 113="q" 114="r" 115="s" 116="t" 117="u" 118="v" 119="w" 120="x" 121="y" 122="z" 
		** 123="{" 124="|" 125="}" 126="~"
		*/
		if ((str[i] >= 97) && (str[i] <= 122))
		{
			c = str[i];
			str[i] = toupper(c);
		}
	}

	return(0);
}

/*
 * lms_str_tolower() converts a string to all lower-case
 *
 * str = the string to convert
 *
 */
int lms_str_tolower(char *str)
{
	register unsigned int i;
	char c;

	if (!str)
	{
		errno = EINVAL;
		return(-1);
	}

	for (i = 0; str[i]; ++i)
	{
		/*
		** 32=" " 33="!" 34=""" 35="#" 36="$" 37="%" 38="&" 39="'" 40="(" 41=")" 42="*" 43="+" 44="," 45="-" 46="." 47="/" 
		** 48="0" 49="1" 50="2" 51="3" 52="4" 53="5" 54="6" 55="7" 56="8" 57="9" 58=":" 59=";" 60="<" 61="=" 62=">" 63="?" 64="@" 
		** 65="A" 66="B" 67="C" 68="D" 69="E" 70="F" 71="G" 72="H" 73="I" 74="J" 75="K" 76="L" 77="M" 78="N" 79="O" 80="P" 81="Q" 82="R" 83="S" 84="T" 85="U" 86="V" 87="W" 88="X" 89="Y" 90="Z" 
		** 91="[" 92="\" 93="]" 94="^" 95="_" 96="`" 
		** 97="a" 98="b" 99="c" 100="d" 101="e" 102="f" 103="g" 104="h" 105="i" 106="j" 107="k" 108="l" 109="m" 110="n" 111="o" 112="p" 113="q" 114="r" 115="s" 116="t" 117="u" 118="v" 119="w" 120="x" 121="y" 122="z" 
		** 123="{" 124="|" 125="}" 126="~"
		*/
		if ((str[i] >= 65) && (str[i] <= 90))
		{
			c = str[i];
			str[i] = tolower(c);
		}
	}

	return(0);
}

/*
 * lms_str_cnttochar() counts and returns the number of bytes before a specific character is found
 *
 * str = the string to count within
 * c = the character to stop at
 *
 */
int lms_str_cnttochar(char *str, char c)
{
	int i;

	if (!str)
	{
		errno = EINVAL;
		return(-1);
	}

	for (i = 0; str[i]; ++i)
	{
		if (str[i] == c)
		{
			return(i);
		}
	}

	return(0);
}

/*
 * lms_str_freearray() frees an array including all members.  the array must be null-terminated.
 *
 * array = the array to free
 * fa = bool variable to free the array itself or not
 *
 */
inline void lms_str_freearray(void **array, unsigned char fa)
{
	unsigned int i;

	for (i = 0; array[i]; ++i)
	{
		free(array[i]);
	}

	if (fa == 1)
	{
		free(array);
	}
}

/*
 * lms_str_copy() copies ``len'' bytes from src to dst
 *
 * src = the source memory buffer
 * dst = the destination memory buffer
 * len = the exact number of bytes to copy
 *
 */
void lms_str_copy(void *src, void *dst, size_t len)
{
	char *x_src;
	char *x_dst;
	register unsigned int i;

	/* Wait... isn't this what f'ing memcpy() does? */
	if (!src || !dst || (len == 0) || (src == dst))
	{
		return;
	}

	x_src = src;
	x_dst = dst;

	for (i = 0; i < len; ++i)
	{
		x_dst[i] = x_src[i];
	}
}

/*
 * lms_str_ocopy() copies ``len'' bytes from src to dst, starting at ``offset'' into src
 *
 * src = the source memory buffer
 * dst = the destination memory buffer
 * len = the exact number of bytes to copy
 * offset = the byte in src at which to begin copying
 *
 */
void lms_str_ocopy(void *src, void *dst, size_t len, unsigned int offset)
{
	char *x_src;
	char *x_dst;
	register unsigned int i;

	if (!src || !dst || (len == 0) || (src == dst))
	{
		return;
	}

	x_src = src;
	x_dst = dst;

	for (i = offset; i < len; ++i)
	{
		x_dst[i - offset] = x_src[i];
	}
}

/*
 *
 *
 *
 *
 *
 *
 */
unsigned int lms_str_ltochr(unsigned char *str, char chr, unsigned int maxlen)
{
	unsigned int cnt;
	unsigned char *strptr;

	if (!str)
	{
		return(0);
	}
	if (maxlen == 0)
	{
		return(0);
	}

	cnt = 0;
	strptr = str;
	while (cnt <= maxlen)
	{
		if (*strptr == chr)
		{
			break;
		}
		strptr++;
		cnt++;
	}

	return(cnt);
}

/*
 * lms_str_getlen() is a type-independent replacement for strlen()
 *
 * str = the string to return the length of, which must be null-terminated
 *
 */
size_t lms_str_getlen(void *str)
{
	char *x_str;
	size_t i;

	if (!str)
	{
		return(0);
	}

	x_str = str;

	i = 0;
	while (1)
	{
		/* If you pass a string that isn't null-terminated into this function it may run for... a while. */
		if (x_str[i] == 0)
		{
			break;
		}
		++i;
	}

	return(i);
}

/*
 * free_array() frees an array, including all members.  
 *
 * ptr = pointer to the array, which must be null-terminated.
 *
 */
void free_array(void **ptr)
{
	register unsigned int i;

	if (!ptr)
	{
		return;
	}

	for (i = 0; ptr[i]; ++i)
	{
		free(ptr[i]);
	}
	free(ptr);
}
