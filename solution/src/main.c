#include <errno.h>
#include <fcntl.h>

#include "elf.h"
#include "elf_processor.h"
#include "util.h"

int main(int argc, char** argv) {
	if (argc < 3)
		error_exit(EINVAL, "Required parameters: <source-elf64-file> <section-name>\n");

	char* source_elf = argv[1];
	char* section_name = argv[2];

	int fd = open(source_elf, O_RDONLY);
	if (fd <= 0)
		error_exit(ENOENT, "Error while openning file");

	Elf64_Ehdr elf_header;
	read_elf_header(fd, &elf_header);

	if (!header_valid(&elf_header)) {
		close(fd);
		error_exit(EINVAL, "Elf header is invalid!");
	}
	if (!contains_any_section(&elf_header)) {
		close(fd);
		error_exit(EINVAL, "Elf does not contain any section!");
	}

	Elf64_Off string_table_header_offset = get_string_table_offset(&elf_header);
	Elf64_Shdr string_table_header;
	read_shdr_with_offset(fd, string_table_header_offset, &string_table_header);

	Elf64_Off shdr_offset =
		get_shdr_offset(fd, &elf_header, section_name, string_table_header.sh_offset);

	Elf64_Shdr shdr;
	read_shdr_with_offset(fd, shdr_offset, &shdr);
	if (!(shdr.sh_flags & SHF_EXECINSTR)) {
		close(fd);
		error_exit(EINVAL, "Section is not executable");
	}

	load_sections(fd, &elf_header);

	start_section(&shdr);
	close(fd);
	return 0;
}
