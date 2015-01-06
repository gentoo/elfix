/* Copyright 2015 Gentoo Foundation
 * Distributed under the terms of the GNU General Public License v2
 *
 * Compute a multilib ABI identifier, as discussed here:
 *
 * 	https://bugs.gentoo.org/show_bug.cgi?id=534206
 *
 * Supported identifiers:
 *
 *	alpha_64
 *	arm_{oabi,eabi}
 *	arm_64
 *	hppa_32
 *	ia_64
 *	m68k_32
 *	mips_{n32,n64,o32}
 *	ppc_{32,64}
 *	s390_{32,64}
 *	sh_32
 *	sparc_{32,64}
 *	x86_{32,x32,64}
 *
 * NOTES:
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

/* For a mapping between common macine names and EM_ number see <elf.h>.  For a more
 * complete mapping, see elfutil's machines[] defined in libebl/eblopenbackend.c.
 */
#define	ELFMAG		"\177ELF"

#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */
#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */

#define EM_ALPHA	0x9026		/* alpha */
#define EM_ARM		40		/* arm */
#define EM_AARCH64	183		/* arm64 */
#define EM_PARISC	15		/* hppa */
#define EM_IA_64	50		/* ia64 */
#define EM_68K		4		/* m68k */
#define EM_MIPS		8		/* mips */
#define EM_PPC		20		/* ppc */
#define EM_PPC64	21		/* ppc64 */
#define EM_S390		22		/* s390 */
#define EM_SH		42		/* sh */
#define EM_SPARC32PLUS	18		/* sparc (32-bit) */
#define EM_SPARCV9	43		/* sparc (64-bit) */
#define EM_386		3		/* x86 */
#define EM_X86_64	62		/* amd64 */

#define EF_MIPS_ABI2		0x00000020	/* Mask for mips n32 ABI */
#define EF_ARM_EABIMASK		0XFF000000	/* Mask for arm EABI - we dont' destinguish versions */

int
get_wordsize(uint8_t ei_class)
{
	switch (ei_class) {
		case ELFCLASS32:
			return 32;
		case ELFCLASS64:
			return 64;
		default:
			errx(1, "Unknown machine word size.");
	}
}

int
get_endian(uint8_t ei_data)
{
	switch (ei_data) {
		case ELFDATA2LSB:
			return 0;
		case ELFDATA2MSB:
			return 1;
		default:
			errx(1, "Unknown endian.");
	}
}


char *
get_abi(uint16_t e_machine, int width, uint32_t e_flags)
{
	/* The following arrives at the abstract ABI name by a process of elimination based on the assumption
	 * that we are only interested in the ABIs supported in Gentoo.  If a new ABIs is added, you need to
	 * rethink the logic to avoid false positives/negatives.
	 */
	switch(e_machine) {

		/* alpha: We support only one 64-bit ABI. */
		case EM_ALPHA:
			return "alpha_64";

		/* arm: We support two 32-bit ABIs, eabi and oabi. */
		case EM_ARM:
			if (e_flags & EF_ARM_EABIMASK)
				return "arm_eabi";
			else
				return "arm_oabi";

		/* arm64: We support only one 64-bit ABI. */
		case EM_AARCH64:
			return "arm_64";

		/* m68k: We support only one 32-bit ABI. */
		case EM_68K:
			return "m68k_32";

		/* mips: We support o32, n32 and n64.  The first is 32-bits and the
		 * latter two are 64-bit ABIs.
		 */
		case EM_MIPS:
			if (width == 64)
				return "mips_n64";
			else
				if (e_flags & EF_MIPS_ABI2)
					return "mips_n32";
				else
					return "mips_o32";


		/* ia64: We support only one 64-bit ABI. */
		case EM_IA_64:
			return "ia_64";

		/* hppa: We support only one 32-bit ABI. */
		case EM_PARISC:
			return "hppa_32";

		/* ppc: We support only one 32-bit ABI. */
		case EM_PPC:
			return "ppc_32";

		/* ppc64: We support only one 64-bit ABI. */
		case EM_PPC64:
			return "ppc_64";

		/* s390: We support one 32-bit and one 64-bit ABI. */
		case EM_S390:
			if (width == 64)
				return "s390_64";
			else
				return "s390_32";

		/* sh: We support only one 32-bit ABI. */
		case EM_SH:
			return "sh_32";

		/* sparc: We support one 32-bit and one 64-bit ABI. */
		case EM_SPARC32PLUS:
			return "sparc_32";
		case EM_SPARCV9:
			return "sparc_64";

		/* amd64 + x86: We support X86-64, X86-X32, and X86-32 ABI. The first
		 * two are 64-bit ABIs and the third is 32-bit.  All three will run on
		 * amd64 architecture, but only the 32-bit will run on the x86 family.
		 */
		case EM_386:
			return "x86_32";
		case EM_X86_64:
			if (width == 64)
				return "x86_64";
			else
				return "x86_x32";

		default:
			return "unknown";
	}
}


/* Sublte point about read():  If you run elf-abi from a little endian machine on an	*/
/* Elf object on a big endian (eg. if you are cross compiling), then you get the wrong	*/
/* byte order.  If howerver, you read it natively, you get it right.  We'll wrap read()	*/
/* with our own version which reads one byte at a time and corrects this.		*/
uint64_t
read_endian(int fd, size_t count, int endian)
{
	ssize_t i;
	uint8_t data;
	uint64_t value = 0;

	if (count > 8)
		errx(1, "Max width exceeded in read_endian()");

	for(i = 0; i < count; i++) {
		if (read(fd, &data, 1) == -1)
			errx(1, "read() in read_endian() failed");
		if (endian)
			value += data << 8 * (count-i-1);
		else
			value += data << 8 * i;
	}

	return value;
}


int
main(int argc, char* argv[])
{
	int width;			/* Machine word size.  Either 32 or 64 bits.		*/
	int endian;			/* Endian, 0 = little, 1 = big				*/
	char *abi;			/* Abi name from glibc's <bits/syscall.h>		*/

	int fd;				/* file descriptor for opened Elf object.		*/
	struct stat s;			/* stat on opened Elf object.				*/
	char magic[4];			/* magic number at the begining of the file		*/
	uint8_t ei_class;		/* ei_class is one byte of e_ident[]			*/
	uint8_t ei_data;		/* ei_data is one byte of e_ident[]			*/
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
		errx(1, "failed to open %s", argv[1]);
	if (read(fd, magic, 4) == -1)
		errx(1, "read() magic failed");
	if (strncmp(magic, ELFMAG, 4))
		errx(1, "%s is not an ELF object", argv[1]);

	/* 32 or 64 bits machine word size? */
	if (read(fd, &ei_class, 1) == -1)
		errx(1, "read() ei_class failed");
	width = get_wordsize(ei_class);

	/* Little or Big Endian? */
	if (read(fd, &ei_data, 1) == -1)
		errx(1, "read() ei_data failed");
	endian = get_endian(ei_data);

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
		errx(1, "lseek() e_machine failed");
	e_machine = (uint16_t)read_endian(fd, 2, endian);

	if (lseek(fd, e_flags_offset, SEEK_SET) == -1)
		errx(1, "lseek() e_flags failed");
	e_flags = (uint32_t)read_endian(fd, 4, endian);

	abi = get_abi(e_machine, width, e_flags);
	printf("%s\n", abi);

	close(fd);
	exit(EXIT_SUCCESS);
}
