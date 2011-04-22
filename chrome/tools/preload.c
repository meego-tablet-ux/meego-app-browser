/*
 * Copyright (c) 2011, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define _GNU_SOURCE
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE 4096

/* This program preloads elf format binaries designated in parameters */
int main(int argc, char *argv[])
{
	int i, j;
	char buf[BUF_SIZE];
	int fd;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Half phnum;
	size_t ra_end = 0;

	for (i = 1; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd == -1)
			continue;

		if (read(fd, buf, BUF_SIZE) <= 0)
			continue;

		ehdr = (Elf32_Ehdr *)buf;
		
		/* Check elf header to see if it's a valid ELF32 binary */
		if (memcmp(ehdr, ELFMAG, 4) || 
				(ehdr->e_ident[EI_CLASS] != ELFCLASS32))
			goto error;
		
		phnum = ehdr->e_phnum;

		/* Check if we have read enough data */
		if (ehdr->e_phoff + ehdr->e_phentsize * phnum > BUF_SIZE)
			goto error;

		phdr = (Elf32_Phdr *)&buf[ehdr->e_phoff];

		/*
		 * Only the segments with type PT_LOAD need to be preloaded.
		 * The advantage of this is some unnecessary section, such as
		 * debug, would not be read so that the total time consumption 
		 * can be reduced.
		 * Program header table is read to know range of these segments.
		 */
		for (j = 0; j < phnum; j++, phdr++) {
			if (phdr->p_type != PT_LOAD)
				continue;

			if (phdr->p_offset + phdr->p_filesz > ra_end)
				ra_end = phdr->p_offset + phdr->p_filesz;
		}

		if (ra_end > 0)
		    readahead(fd, 0, ra_end);

error:
		close(fd);
	}

	return 0;
}
