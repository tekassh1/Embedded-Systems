#ifndef ELF_PROCESSOR
#define ELF_PROCESSOR

#include <elf.h>
#include <stdbool.h>

void read_elf_header(int fd, Elf64_Ehdr* header);
bool header_valid(Elf64_Ehdr* header);
bool contains_any_section(Elf64_Ehdr* header);
Elf64_Off get_string_table_offset(Elf64_Ehdr* header);
void read_shdr_with_offset(int fd, Elf64_Off s_offset, Elf64_Shdr* section_header);
void read_shdr(int fd, Elf64_Shdr* section_header, Elf64_Half entry_size);
Elf64_Off get_shdr_offset(int fd, Elf64_Ehdr* header, char* name, Elf64_Off string_table_off);
void load_sections(int fd, Elf64_Ehdr* header);
void start_section(Elf64_Shdr* sh);

#endif
