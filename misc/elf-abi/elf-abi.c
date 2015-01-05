/* Copyright 2015 Gentoo Foundation
 * Distributed under the terms of the GNU General Public License v2
 *
 * Compute a multilib ABI identifier, as discussed here:
 *
 * 	https://bugs.gentoo.org/show_bug.cgi?id=534206
 *
 * Supported identifiers:
 *
 *	alpha_{32,64}
 *	arm_{32,64}
 *	hppa_{32,64}
 *	ia_{32,64}
 *	m68k_{32,64}
 *	mips_{n32,n64,o32}
 *	ppc_{32,64}
 *	s390_{32,64}
 *	sh_{32,64}
 *	sparc_{32,64}
 *	x86_{32,x32,64}
 *
 * NOTES:
 *
 * * The ABIs referenced by some of the above *_32 and *_64 identifiers
 *   may be imaginary, but they are listed anyway, since the goal is to
 *   establish a naming convention that is as consistent and uniform as
 *   possible.
 *
 * * The Elf header's e_ident[EI_OSABI] byte is completely ignored,
 *   since OS-independence is one of the goals. The assumption is that,
 *   for given installation, we are only interested in tracking multilib
 *   ABIs for a single OS.
 *
 * REFERENCE:
 *
 *   http://www.sco.com/developers/gabi/2000-07-17/ch4.eheader.html
 *
 * Copyright 2015 Anthony G. Basile - <blueness@gentoo.org>
 * Copyright 2015 Zac Medico - <zmedico@gentoo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* We steal this from <elf.h> but don't include it so as to not increase our dependancies. */
#define	ELFMAG		"\177ELF"

#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */

#define EM_SPARC	 2		/* SUN SPARC */
#define EM_386		 3		/* Intel 80386 */
#define EM_68K		 4		/* Motorola m68k family */
#define EM_MIPS		 8		/* MIPS R3000 big-endian */
#define EM_MIPS_RS3_LE	10		/* MIPS R3000 little-endian */
#define EM_PARISC	15		/* HPPA */
#define EM_SPARC32PLUS	18		/* Sun's "v8plus" */
#define EM_PPC		20		/* PowerPC */
#define EM_PPC64	21		/* PowerPC 64-bit */
#define EM_S390		22		/* IBM S390 */
#define EM_ARM		40		/* ARM */
#define EM_FAKE_ALPHA	41		/* Digital Alpha */
#define EM_SH		42		/* Hitachi SH */
#define EM_SPARCV9	43		/* SPARC v9 64-bit */
#define EM_IA_64	50		/* Intel Merced */
#define EM_X86_64	62		/* AMD x86-64 architecture */
#define EM_AARCH64	183		/* ARM AARCH64 */
#define EM_ALPHA	0x9026

#define EF_MIPS_ABI2		32
#define EF_MIPS_ABI_ON32	64
/*
E_MIPS_ABI_O32     = 0x00001000
E_MIPS_ABI_O64     = 0x00002000
E_MIPS_ABI_EABI32  = 0x00003000
E_MIPS_ABI_EABI64  = 0x00004000
*/

#define EF_ARM_NEW_ABI		0x80
#define EF_ARM_OLD_ABI		0x100


int
get_wordsize(uint8_t ei_class)
{
	switch (ei_class) {
		case ELFCLASS32:
			return 32;
		case ELFCLASS64:
			return 64;
		default:
			return 0;
	}
}


char *
get_abi(uint16_t e_machine, int width, uint32_t e_flags)
{
	switch(e_machine) {
		case EM_ALPHA:
		case EM_FAKE_ALPHA:
			return "alpha";
		case EM_X86_64:
			return "amd64";
		case EM_ARM:
			return "arm";
		case EM_AARCH64:
			return "arm64";
		case EM_68K:
			return "m68k";
		case EM_MIPS_RS3_LE:
		case EM_MIPS:
			return "mips";
		case EM_IA_64:
			return "ia64";
		case EM_PARISC:
			return "hppa";
		case EM_PPC:
			return "ppc";
		case EM_PPC64:
			return "ppc64";
		case EM_S390:
			return "s390";
		case EM_SH:
			return "sh";
		case EM_SPARC32PLUS:
		case EM_SPARCV9:
		case EM_SPARC:
			return "sparc";
		case EM_386:
			return "x86";
		default:
			return "unknown_arch";
	}
}

int
main(int argc, char* argv[])
{
	int width;			/* Machine word size.  Either 32 or 64 bits.		*/
	char *abi;			/* Abi name from glibc's <bits/syscall.h>		*/

	int fd;				/* file descriptor for opened Elf object.		*/
	struct stat s;			/* stat on opened Elf object.				*/
	char magic[4];			/* magic number at the begining of the file		*/
	uint8_t ei_class;		/* ei_class is one byte of e_ident[]			*/
	uint16_t e_machine;		/* Size is Elf32_Half or Elf64_Half.  Both are 2 bytes.	*/
	uint32_t e_flags;		/* Size is Elf32_Word or Elf64_Word.  Both are 4 bytes.	*/
	uint64_t e_machine_offset, e_flags_offset;  /* Wide enough for either 32 or 64 bits.	*/

	/* Is the second parameter a regular file? */
	if (argc != 2)
		errx(1, "Usage: %s <file>", argv[0]);
	if (stat(argv[1], &s) == -1)
		errx(1, "%s does not exist", argv[1]);
	if (!S_ISREG(s.st_mode))
		errx(1, "%s is not a regular file", argv[1]);

	/* Can we read it and is it an ELF object? */
	if ((fd = open(argv[1], O_RDONLY)) == -1)
		err(1, "failed to open %s", argv[1]);
	if (read(fd, magic, 4) == -1)
		err(1, "read() magic failed");
	if (strncmp(magic, ELFMAG, 4) != 0)
		errx(1, "%s is not an ELF object", argv[1]);

	/* 32 or 64 bits? */
	if (read(fd, &ei_class, 1) == -1)
		err(1, "read() ei_class failed");
	width = get_wordsize(ei_class);

	/*
	All Elf files begin with the following Elf header:
	unsigned char   e_ident[EI_NIDENT];	- 16 bytes
	Elf32_Half      e_type;			- 2 bytes
	Elf32_Half      e_machine;		- 2 bytes
	Elf32_Word      e_version;		- 4 bytes
	Elf32_Addr      e_entry;		- 4 bytes or 8 bytes for Elf64
	Elf32_Off       e_phoff;		- 4 bytes or 8 bytes for Elf64
	Elf32_Off       e_shoff;		- 4 bytes or 8 bytes for Elf64
	Elf32_Word      e_flags;		- 4 bytes

	Seek to e_machine = 18 bytes.
	Seek to e_flags   = 36 bytes (Elf32) or 48 bytes (Elf64)
	*/
	e_machine_offset = 18;
	if (width == 32)
		e_flags_offset = 36;
	else
		e_flags_offset = 48;

	/* What is the abi? */
	if (lseek(fd, e_machine_offset, SEEK_SET) == -1)
		err(1, "lseek() e_machine failed");
	if (read(fd, &e_machine, 2) == -1)
		err(1, "read() e_machine failed");

	if (lseek(fd, e_flags_offset, SEEK_SET) == -1)
		err(1, "lseek() e_flags failed");
	if (read(fd, &e_flags, 2) == -1)
		err(1, "read() e_flags failed");

	abi = get_abi(e_machine, width, e_flags);

	printf("%s\n", abi);


/*
	if (!strcmp(arch, "mips")) ;
		switch (e_flags & EF_MIPS_ABI) {
			case E_MIPS_ABI_O32:
				abi = "o32";
				break;

		if mips_abi == E_MIPS_ABI_O32:
			name = "o32"
		elif mips_abi == E_MIPS_ABI_O64:
			name = "o64"
		elif mips_abi == E_MIPS_ABI_EABI32:
			name = "eabi32"
		elif mips_abi == E_MIPS_ABI_EABI64:
			name = "eabi64"
	elif elf_header.e_flags & EF_MIPS_ABI2:
		name = "n32"
	elif elf_header.ei_class == ELFCLASS64:
		name = "n64"
		}
*/
	close(fd);
	exit(EXIT_SUCCESS);
}
