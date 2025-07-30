# include <stdint.h>
 
typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	// Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

# define ELF_NIDENT	16
 
typedef struct {
	uint8_t		e_ident[ELF_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;


enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};
 
# define ELFMAG0	0x7F // e_ident[EI_MAG0]
# define ELFMAG1	'E'  // e_ident[EI_MAG1]
# define ELFMAG2	'L'  // e_ident[EI_MAG2]
# define ELFMAG3	'F'  // e_ident[EI_MAG3]
 
# define ELFDATA2LSB	(1)  // Little Endian
# define ELFCLASS32	(1)  // 32-bit Architecture


enum Elf_Type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2, // Executable File
	ET_DYN		= 3 //Shared library
};
 
# define EM_386		(3)  // x86 Machine Type
# define EM_486		(6)
# define EV_CURRENT	(1)  // ELF Current Version


typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;


# define SHN_UNDEF	(0x00) // Undefined/Not present
# define PT_NULL (0)
# define PT_LOAD (1)
# define PT_DYNAMIC (2)


enum ShT_Types {
	SHT_NULL	= 0,   // Null section
	SHT_PROGBITS	= 1,   // Program information
	SHT_SYMTAB	= 2,   // Symbol table
	SHT_STRTAB	= 3,   // String table
	SHT_RELA	= 4,   // Relocation (w/ addend)
	SHT_NOBITS	= 8,   // Not present in file
	SHT_REL		= 9,   // Relocation (no addend)
};
 
enum ShT_Attributes {
	SHF_WRITE	= 0x01, // Writable section
	SHF_ALLOC	= 0x02  // Exists in memory
};

typedef struct {
    uint32_t   p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    uint32_t   p_filesz;
    uint32_t   p_memsz;
    uint32_t   p_flags;
    uint32_t   p_align;
} Elf32_Phdr;


#define PF_X 1
#define PF_R 4
#define PF_W 2


typedef struct {
        Elf32_Sword d_tag;
        union {
                Elf32_Word      d_val;
                Elf32_Addr      d_ptr;
                Elf32_Off       d_off;
        } d_un;
} Elf32_Dyn;

#define DT_NULL (0)
#define DT_SYMINFO (0x6ffffeff)
#define DT_SYMINENT (0x6ffffdff)
#define DT_SYMTAB (6)
#define DT_SYMENT (11)
#define DT_STRTAB (5)
#define DT_STRSZ (10)
#define DT_HASH (4) //Standard hashtable for non Linux?
#define DT_GNU_HASH (0x6ffffef5) //Hashtable for Linux?
#define DT_JMPREL (23)
#define DT_PLTRELSZ (2)
#define DT_PLTGOT (3)
#define DT_PLTREL (20)

typedef struct {
        Elf32_Word      st_name;
        Elf32_Addr      st_value;
        Elf32_Word      st_size;
        unsigned char   st_info;
        unsigned char   st_other;
        Elf32_Half      st_shndx; //pretty useless for program header use I guess
} Elf32_Sym;


struct gnu_hash_table {
    uint32_t nbuckets;
    uint32_t symoffset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
    uint32_t data[]; //data like shown below
    //uint32_t bloom[bloom_size]; /* uint64_t for 64-bit binaries */
    //uint32_t buckets[nbuckets];
    //uint32_t chain[];
};

struct elf_hash_table {
    uint32_t nbuckets;
    uint32_t nchain;
    uint32_t data[]; //data like shown below
	//uint32_t bucket[nbucket];
    //uint32_t chain[nchain];
};

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */
#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* Weak symbol */
#define	STB_NUM		3		/* Number of defined types.  */
#define STB_LOOS	10		/* Start of OS-specific */
#define STB_GNU_UNIQUE	10		/* Unique symbol.  */
#define STB_HIOS	12		/* End of OS-specific */
#define STB_LOPROC	13		/* Start of processor-specific */
#define STB_HIPROC	15		/* End of processor-specific */


/* Legal values for ST_TYPE subfield of st_info (symbol type).  */
#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STT_FILE	4		/* Symbol's name is file name */
#define STT_COMMON	5		/* Symbol is a common data object */
#define STT_TLS		6		/* Symbol is thread-local data object*/
#define	STT_NUM		7		/* Number of defined types.  */
#define STT_LOOS	10		/* Start of OS-specific */
#define STT_GNU_IFUNC	10		/* Symbol is indirect code object */
#define STT_HIOS	12		/* End of OS-specific */
#define STT_LOPROC	13		/* Start of processor-specific */
#define STT_HIPROC	15		/* End of processor-specific */


#define STV_DEFAULT (0)
#define STV_INTERNAL (1)
#define STV_HIDDEN (2)
#define STV_PROTECTED (3)


#define ELF32_ST_BIND(info)          ((info) >> 4)
#define ELF32_ST_TYPE(info)          ((info) & 0xf)
#define ELF32_ST_VISIBILITY(other)       ((other)&0x3)


typedef struct elf32_rel {
  Elf32_Addr	r_offset;
  Elf32_Word	r_info;
} Elf32_Rel;

#define ELF32_R_SYM(info) ((info) >> 8)
#define ELF32_R_TYPE(info) ((info) & 0xff)

#define ELF_R_386_NONE                  0
#define ELF_R_386_32                    1
#define ELF_R_386_PC32                  2
#define ELF_R_386_GOT32                 3
#define ELF_R_386_PLT32                 4
#define ELF_R_386_COPY                  5
#define ELF_R_386_GLOB_DAT              6
#define ELF_R_386_JMP_SLOT              7
#define ELF_R_386_RELATIVE              8
#define ELF_R_386_GOTOFF                9
#define ELF_R_386_GOTPC                 10