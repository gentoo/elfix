/* Copyright 2015 Gentoo Foundation
 * Distributed under the terms of the GNU General Public License v2
 *
 * Compute a multilib ABI identifier, as discussed here:
 *
 * 	https://bugs.gentoo.org/show_bug.cgi?id=534206
 *
 * Supported identifiers:
 *
 *	alpha_32
 *	arm_{oabi32,32,64}
 *	hppa_32,64
 *	ia_64
 *	m68k_32
 *	mips_{n32,n64,o32}
 *	ppc_{32,64}
 *	s390_32
 *	sh_32
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
#include <endian.h>

/* We steal this from <elf.h> but don't include it so as to not increase our dependancies. */
#define	ELFMAG		"\177ELF"

#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */
#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */

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


/*
def compute_suffix_mips(elf_header):
	name = None
	mips_abi = elf_header.e_flags & EF_MIPS_ABI
	if mips_abi:
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

	return name

def compute_multilib_id(elf_header):
	prefix = machine_prefix_map.get(elf_header.e_machine)
	suffix = None

	if prefix == "mips":
		suffix = compute_suffix_mips(elf_header)
	elif elf_header.ei_class == ELFCLASS64:
		suffix = "64"
	elif elf_header.ei_class == ELFCLASS32:
		if elf_header.e_machine == EM_X86_64:
			suffix = "x32"
		else:
			suffix = "32"

	if prefix is None or suffix is None:
		multilib_id = None
	else:
		multilib_id = "%s_%s" % (prefix, suffix)

	return multilib_id
*/


#define bswap_16(x) ((uint16_t)((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

/*
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
*/

#define MAX_IDENT	32

int
main(int argc, char* argv[])
{
	char ident[MAX_IDENT];		/* The Gentoo standard arch_abi identifier.		*/
	char *arch;			/* The Gentoo architecture identifier.			*/
	int fd;
	struct stat s;
	char magic[4];			/* magic number at the begining of the file		*/
	uint8_t ei_class, width;	/* width = 8 bytes for 32-bits, 16 bytes for 64-bits.	*/
	uint8_t ei_data, endian;	/* endian = 0 for little, 1 for big endian.		*/
	uint16_t e_machine;		/* Size is Elf32_Half or Elf64_Half.  Both are 2 bytes.	*/
	uint32_t e_flags;		/* Size is Elf32_Word or Elf64_Word.  Both are 4 bytes.	*/
	uint64_t e_flags_offset;	/* Wide enough for either 32 or 64 bits.		*/

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
	switch (ei_class) {
		case ELFCLASS32:
			width = 4;
			printf("32 bit\n");
			break;
		case ELFCLASS64:
			width = 8;
			printf("64 bit\n");
			break;
		default:
			width = 0;
			printf("Unknown bit\n");
	}

	/* Little or Big Endian */
	if (read(fd, &ei_data, 1) == -1)
		err(1, "read() ei_data failed");
	switch (ei_data) {
		case ELFDATA2LSB:
			endian = 0;
			printf("Little Endian\n");
			break;
		case ELFDATA2MSB:
			endian = 1;
			printf("Big Endian\n");
			break;
		default:
			endian = -1;
			printf("Unknown Endian\n");
	}

	/* seek to e_macine = 16 bytes (e_ident[])) + 2 bytes (e_type which is Elf32_Half/Elf64_Half) */
	if (lseek(fd, 18, SEEK_SET) == -1)
		err(1, "lseek() e_machine failed");

	/* What is the arch? */
	if (read(fd, &e_machine, 2) == -1)
		err(1, "read() e_machine failed");
	//if (endian == 0)
	//	e_machine = bswap_16(e_machine);
	printf("Machine =%d\n", e_machine);

	switch(e_machine) {
		case EM_ALPHA:
		case EM_FAKE_ALPHA:
			arch = "alpha";
			break;
		case EM_X86_64:
			arch = "amd64";
			break;
		case EM_ARM:
			arch = "arm";
			break;
		case EM_AARCH64:
			arch = "arm64";
			break;
		case EM_68K:
			arch = "m68k";
			break;
		case EM_MIPS_RS3_LE:
		case EM_MIPS:
			arch = "mips";
			break;
		case EM_IA_64:
			arch = "ia64";
			break;
		case EM_PARISC:
			arch = "hppa";
			break;
		case EM_PPC:
			arch = "ppc";
			break;
		case EM_PPC64:
			arch = "ppc64";
			break;
		case EM_S390:
			arch = "s390";
			break;
		case EM_SH:
			arch = "sh";
			break;
		case EM_SPARC32PLUS:
		case EM_SPARCV9:
		case EM_SPARC:
			arch = "sparc";
			break;
		case EM_386:
			arch = "x86";
			break;
		default:
			arch = "unknown";
	}

	printf("%s\n", arch);


	/*
	e_data_offset = 
	if (lseek(fd, 18, SEEK_SET) == -1)
		err(1, "lseek() e_machine failed");
	if (read(fd, &e_flags, 2) == -1)
		err(1, "read() e_machine failed");
	*/

	memset(ident, 0, MAX_IDENT);

	close(fd);
	exit(EXIT_SUCCESS);
}

/*
	# E_ENTRY + 3 * sizeof(uintN)
	e_flags_offset = E_ENTRY + 3 * width // 8
	f.seek(e_flags_offset)
	e_flags = uint32(f.read(4))

	return _elf_header(ei_class, ei_data, e_machine, e_flags)

	multilib_id = compute_multilib_id(elf_header)
*/
