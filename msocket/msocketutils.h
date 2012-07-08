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

#ifndef INCLUDED_MSOCKETUTILS_H
# define INCLUDED_MSOCKETUTILS_H     1

# define stringtoint(x)			(int)strtol((x), (char **)NULL, 10)
# define stringtolong(x)		strtol((x), (char **)NULL, 10)
# define stringtouint(x)		(unsigned int)strtoul((x), (char **)NULL, 10)
# define stringtoulong(x)		strtoul((x), (char **)NULL, 10)
# define stringtofloat(x)		strtof((x), (char **)NULL)

/* file.c */
extern size_t lms_file_readln(int fd, char *buf, size_t buf_len);
extern int lms_file_writepid(const char *fn, pid_t pid);
extern short lms_file_icanrw(struct stat *fs);
extern short lms_file_icanr(struct stat *fs);

/* str.c */
extern void utils_str_memnuke(volatile void *b, size_t sz);
extern int utils_str_splitchr(const char *str, int c, char **mstr);
extern int utils_str_splitnum(const char *str, int c, char **mstr, unsigned int num);
extern unsigned int utils_str_cntchr(const char *str, int c);
extern unsigned int utils_str_haschr(const char *str, int c);
extern size_t utils_str_arraycat(char **src, char *dst, size_t dst_len);
extern int utils_str_toupper(char *str);
extern int utils_str_tolower(char *str);
extern int utils_str_cnttochar(char *str, char c);
extern inline void utils_str_freearray(void **array, unsigned char fa);
extern void utils_str_copy(void *src, void *dst, size_t len);
extern void utils_str_ocopy(void *src, void *dst, size_t len, unsigned int offset);
extern unsigned int utils_str_ltochr(unsigned char *str, char chr, unsigned int maxlen);
extern size_t utils_str_getlen(void *str);
extern void free_array(void **ptr);

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
extern int lms_passwords_encodemulti(char *indata, utils_passwords_data *outdata);
extern int lms_passwords_checkmulti(char *chk, utils_passwords_data *real);
extern int lms_passwords_converttomulti(unsigned char *indata, utils_passwords_data *outdata, unsigned short is_b64);

#endif /* INCLUDED_MSOCKETUTILS_H */
