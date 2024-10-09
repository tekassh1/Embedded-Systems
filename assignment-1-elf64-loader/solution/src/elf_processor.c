#include <elf.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "util.h"

void read_elf_header(int fd, Elf64_Ehdr* header) {
	if (fd <= 0)
		return;

	ssize_t bytes_am = read(fd, header, sizeof(Elf64_Ehdr));
	if (bytes_am < sizeof(Elf64_Ehdr)) {
		close(fd);
		error_exit(EINVAL, "Error while reading elf file (header).");
	}
}

bool header_valid(Elf64_Ehdr* header) {
	return header->e_ident[EI_MAG0] == ELFMAG0 && header->e_ident[EI_MAG1] == ELFMAG1 &&
		   header->e_ident[EI_MAG2] == ELFMAG2 && header->e_ident[EI_MAG3] == ELFMAG3;
}

bool contains_any_section(Elf64_Ehdr* header) {
	return header->e_shnum > 0;
}

bool string_table_idx_valid(Elf64_Ehdr* header) {
	return header->e_shstrndx != SHN_UNDEF;
}

Elf64_Off get_string_table_offset(Elf64_Ehdr* header) {
	if (!string_table_idx_valid(header))
		error_exit(EINVAL, "String table index not exist.");
	return header->e_shoff + (header->e_shstrndx * header->e_shentsize);
}

void read_shdr_with_offset(int fd, Elf64_Off s_offset, Elf64_Shdr* section_header) {
	if (fd <= 0) {
		close(fd);
		error_exit(EINVAL, "File descriptor error!");
	}

	if ((lseek(fd, (off_t)s_offset, 0) < 0) ||
		(read(fd, section_header, sizeof(Elf64_Shdr)) < sizeof(Elf64_Shdr))) {
		close(fd);
		error_exit(EIO, "Error reading section!");
	}
}

void read_shdr(int fd, Elf64_Shdr* section_header, Elf64_Half entry_size) {
	if (fd <= 0) {
		close(fd);
		error_exit(EINVAL, "File descriptor error!");
	}

	if ((read(fd, section_header, entry_size) < entry_size)) {
		close(fd);
		error_exit(EIO, "Error reading section!");
	}
}

bool compare_names(int fd, char* name) {  // maybe err
	char c;
	size_t idx = 0;

	while (true) {
		if (read(fd, &c, 1) < 1) {
			close(fd);
			error_exit(EIO, "Error reading section name");
		} else if (c == name[idx] && c == '\0')
			return true;
		else if (c != name[idx])
			return false;

		idx++;
	}
	return false;
}

Elf64_Off get_shdr_offset(int fd, Elf64_Ehdr* header, char* name, Elf64_Off string_table_off) {
	Elf64_Shdr sh;

	for (size_t i = 0; i < header->e_shnum; i++) {
		if ((lseek(fd, (off_t)(header->e_shoff + i * header->e_shentsize), 0) < 0)) {
			close(fd);
			error_exit(EIO, "Error reading section table!");
		}

		read_shdr(fd, &sh, header->e_shentsize);

		if ((lseek(fd, (off_t)(string_table_off + sh.sh_name), 0) < 0)) {
			close(fd);
			error_exit(EIO, "Error reading section name");
		}

		if (compare_names(fd, name)) {
			return header->e_shoff + i * header->e_shentsize;
		}
	}
	close(fd);
	error_exit(EINVAL, "Section with given name does not exist!");
	return 0;
}

void load_sections(int fd, Elf64_Ehdr* header) {
	Elf64_Phdr ph;
	Elf64_Addr addr;
	Elf64_Off offset;
	size_t memsz;

	const size_t pagesz = sysconf(_SC_PAGE_SIZE);

	size_t index = 0;
	if (lseek(fd, (off_t)header->e_phoff, 0) < 0) {
		close(fd);
		error_exit(EIO, "Error when read program header");
	}
	while (index < header->e_phnum) {
		if (read(fd, &ph, header->e_phentsize) < header->e_phentsize) {
			close(fd);
			error_exit(EIO, "Error when read program header");
		}

		addr = ph.p_vaddr;
		offset = ph.p_offset;
		if (ph.p_type == PT_LOAD) {
			addr -= addr % pagesz;
			offset -= offset % pagesz;
			memsz = ph.p_memsz + (ph.p_vaddr - addr);

			if (mmap((void*)addr,  // NOLINT
					 memsz,
					 (ph.p_flags & PF_X ? PROT_EXEC : 0) | (ph.p_flags & PF_W ? PROT_WRITE : 0) |
						 (ph.p_flags & PF_R ? PROT_READ : 0),
					 MAP_PRIVATE | MAP_FIXED | MAP_FIXED_NOREPLACE, fd,
					 (off_t)offset) == MAP_FAILED) {
				close(fd);
				error_exit(EIO, "Section map fail");
			}
		}
		index++;
	}
}

void start_section(Elf64_Shdr* sh) {
	typedef void (*section_start_fn)(void);

	section_start_fn start = (section_start_fn)sh->sh_addr;	 // NOLINT

	start();
}
