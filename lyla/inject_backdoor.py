from typing import Union

import random
import tempfile
import struct
import dataclasses

from lief.ELF import RELOCATION_X86_64 as RTYPE, DYNAMIC_FLAGS, SYMBOL_BINDINGS, SYMBOL_TYPES, SYMBOL_SECTION_INDEX

from pwnlib import asm, elf
from pwnlib.util.packing import p16, u16, p32, u32, p64, u64
from pwnlib.util.misc import align, align_down
from pwnlib.util import lists

asm.context.arch = "amd64"
asm.context.bits = 64

ELF64_RELA_SIZE = 24
PAGE_SIZE = 0x1000
RELOC_OBF_ADDEND = 0x16493F2103392E07
DECOY_START_TIME = 1677383997
ACTUAL_KEY = [0x85615CE70BA97239, 0xAF6F5627BC993A1E]
GARBAGE_CODE_SIZE = 4  # time() ptr and junk code

SYMBOL_0 = "__gmon_start__"
SYMBOL_1 = "_ITM_registerTMCloneTable"
SYMBOL_2 = "_ITM_deregisterTMCloneTable"

random.seed(0x6D617820696E74656E73697479)


class ELF(elf.ELF):
    def _populate_memory(self):
        super()._populate_memory()
        # pwntools.elf.ELF does not fill segment info properly for gaps at the end of segment.
        if self._fill_gaps:
            last_intv = None
            for intv in sorted(self.memory):
                if not intv.data and last_intv and align(PAGE_SIZE, last_intv.end) == intv.end:
                    self.memory.remove(intv)
                    self.memory.addi(intv.begin, intv.end, last_intv.data)
                last_intv = intv

    def read_offset(self, offset, size):
        return self.mmap[offset : offset + size]

    def write_offset(self, offset, data: bytes):
        """Similar to write() but write at file offset instead of vaddr."""
        self.mmap[offset : offset + len(data)] = data

    def _packing_read_maker(unp_func, size):
        return lambda self, offset: unp_func(self.read_offset(offset, size))

    u8_off = _packing_read_maker(lambda x: x[0], 1)
    u16_off = _packing_read_maker(u16, 2)
    u32_off = _packing_read_maker(u32, 4)
    u64_off = _packing_read_maker(u64, 8)

    def _packing_write_maker(pack_func):
        return lambda self, offset, value: self.write_offset(offset, pack_func(value))

    p8_off = _packing_write_maker(lambda x: bytes([x]))
    p16_off = _packing_write_maker(p16)
    p32_off = _packing_write_maker(p32)
    p64_off = _packing_write_maker(p64)

    def dynamic_value_vaddr_by_tag(self, tag):
        dynamic = self.get_section_by_name(".dynamic")
        for i, dyn in enumerate(dynamic.iter_tags()):
            if dyn["d_tag"] == tag:
                return dynamic.header.sh_addr + i * dynamic.header.sh_entsize + 8

    def modify_dynamic_value_by_tag(self, tag, new_value):
        assert self.dynamic_by_tag(tag)
        self.p64(self.dynamic_value_vaddr_by_tag(tag), new_value)

    def gap_after_load_segment(self, n):
        loadable_segments = [segment for segment in self.segments if segment.header.p_type == "PT_LOAD"]
        gap_vaddr = loadable_segments[n].header.p_vaddr + loadable_segments[n].header.p_filesz
        gap_file_offset = loadable_segments[n].header.p_offset + loadable_segments[n].header.p_filesz
        gap_bytes = loadable_segments[n + 1].header.p_offset - gap_file_offset
        if gap_bytes <= 0:
            raise ValueError(f"no gap between segment #{n} and #{n+1}")
        print(f"Usable gap between LOAD segment #{n} and #{n+1}: {gap_bytes} bytes")
        return (gap_vaddr, gap_bytes)

    def symtab(self, n: Union[int, str]):
        """Return the address of n-th symbol table entry"""
        dynsym = self.get_section_by_name(".dynsym")
        if isinstance(n, str):
            n = self.symidx[n]
        return dynsym.header.sh_addr + dynsym.header.sh_entsize * n

    @property
    def symidx(self):
        if not hasattr(self, "_symbol_name2idx"):
            dynsym = self.get_section_by_name(".dynsym")
            self._symbol_name2idx = {sym.name: i for i, sym in enumerate(dynsym.iter_symbols())}
        return self._symbol_name2idx

    @property
    def load_segments(self):
        return (segment for segment in self.segments if segment.header.p_type == "PT_LOAD")


class BufferAllocator:
    class Buffer:
        def __init__(self, parent, addr, size):
            self._alloc = parent
            self.address = addr
            self.size = size

        def write(self, data: bytes):
            assert len(data) <= self.size
            self._alloc._writer(self.address, data)
            return self

        def fill(self, value):
            self.write(bytes([value]) * self.size)
            return self

    def __init__(self, writer, name: str, base_address, size):
        self._writer = writer
        self._used = 0
        self._base_address = base_address
        self._size = size
        self._name = name
        self._layout = {}

    def __repr__(self):
        return f"<BufferAllocator {self._name} with {len(self._layout)} entries>"

    def align(self, alignment: int):
        cur = (self._base_address + self._used) % alignment
        if cur == 0:
            return
        self.skip(alignment - cur)

    def skip(self, size: int):
        assert self._used + size < self._size, f"{repr(self)}: insufficient space"
        self._used += size

    def alloc(self, size: int, usage_hint: str = "<not specified>") -> "BufferAllocator.Buffer":
        assert self._used + size < self._size, f"{repr(self)}: insufficient space"
        addr = self._base_address + self._used
        if size != 0:
            self._layout[self._used] = (usage_hint, size)
        self._used += size
        return self.Buffer(self, addr, size)

    def __str__(self):
        res = f"BufferAllocator {self._name}\n{len(self._layout)} entries, {self._size} bytes capacity, {self._used} bytes used\n--\n"
        for offset, (usage_hint, size) in self._layout.items():
            res += f"+{offset:#06x}: {usage_hint} ({size} bytes)\n"
        return res.strip()


@dataclasses.dataclass
class Relocation:
    offset: int
    type: RTYPE
    addend: int = 0
    symidx: int = 0

    @classmethod
    def parse(cls, data: bytes):
        off, info, addend = struct.unpack("<QQQ", data)
        type = RTYPE(info & 0xFFFFFFFF)
        symidx = info >> 32
        return cls(offset=off, type=type, addend=addend, symidx=symidx)

    def pack(self):
        return struct.pack("<QQQ", self.offset, (self.symidx << 32) | self.type.value, self.addend)


def Elf64SymMeta(st_name, st_info, st_other, st_shndx):
    return struct.unpack("<Q", struct.pack("<IBBH", st_name, st_info, st_other, st_shndx))[0]


def shuffled(x):
    x = x[::]
    random.shuffle(x)
    return x


def extend_and_shift_relocation(elf: ELF):
    load_segments = list(elf.load_segments)
    rela_dyn = elf.get_section_by_name(".rela.dyn")
    rela_plt = elf.get_section_by_name(".rela.plt")

    rela = elf.dynamic_value_by_tag("DT_RELA")
    rela_sz = elf.dynamic_value_by_tag("DT_RELASZ")
    jmprel = elf.dynamic_value_by_tag("DT_JMPREL")
    jmprel_sz = elf.dynamic_value_by_tag("DT_PLTRELSZ")

    # Assume that relocation data is located at the end of ELF header segment (first LOAD segment in the file).
    assert rela_plt.header.sh_addr + rela_plt.header.sh_size == jmprel + jmprel_sz
    assert load_segments[0].section_in_segment(rela_plt)
    assert load_segments[0].section_in_segment(rela_dyn)

    # Assume there is no gap between .rela.dyn and .rela.plt.
    assert rela + rela_sz == jmprel

    # Make RELA and JMPREL overlap: rela_sz = rela_sz + jmprel_sz
    elf.modify_dynamic_value_by_tag("DT_RELASZ", rela_sz + jmprel_sz)
    # Enlarge RELA by one record, move JMPREL forward; Makes the overlap off by one record, this is exactly what we want
    elf.modify_dynamic_value_by_tag("DT_JMPREL", jmprel + ELF64_RELA_SIZE)
    # We are going to inject one more IRELATIVE
    elf.modify_dynamic_value_by_tag("DT_RELACOUNT", elf.dynamic_value_by_tag("DT_RELACOUNT") + 1)
    # Set TEXTREL to make .text writable during relocation
    elf.modify_dynamic_value_by_tag("DT_FLAGS", elf.dynamic_value_by_tag("DT_FLAGS") | DYNAMIC_FLAGS.TEXTREL.value)

    # Adjust segment / section headers accordingly. This is not required for run, but we would like to produce innocent ELFs.
    # This looks horrible, but I can't find any strictly in-place ELF manipulation library in Python.
    # (LIEF adds a brand new LOAD segment even with no changes at all, crazy)
    seg0_file_offset = elf.header.e_phoff + elf.header.e_phentsize * elf.segments.index(load_segments[0])
    # loadable_segments[0].physical_size += ELF64_RELA_SIZE
    # loadable_segments[0].virtual_size += ELF64_RELA_SIZE

    def add(offset, addend):
        elf.p64_off(offset, elf.u64_off(offset) + addend)

    add(seg0_file_offset + 32, ELF64_RELA_SIZE)
    add(seg0_file_offset + 40, ELF64_RELA_SIZE)

    sec_rela_dyn_offset = elf.header.e_shoff + elf.header.e_shentsize * elf.sections.index(rela_dyn)
    # rela_dyn.size += ELF64_RELA_SIZE
    add(sec_rela_dyn_offset + 32, ELF64_RELA_SIZE)

    sec_rela_plt_offset = elf.header.e_shoff + elf.header.e_shentsize * elf.sections.index(rela_plt)
    # rela_plt.virtual_address += ELF64_RELA_SIZE
    # rela_plt.offset += ELF64_RELA_SIZE
    add(sec_rela_plt_offset + 16, ELF64_RELA_SIZE)
    add(sec_rela_plt_offset + 24, ELF64_RELA_SIZE)


def assemble_shellcode(path, **constants):
    with open(path, "rt") as fp:
        lines = [f".equ {key}, {value}" for key, value in constants.items()]
        lines += [line for line in fp.readlines() if not line.startswith(".equ ")]
        return asm.asm("\n".join(lines))


def pack_reloc(entries):
    def _flatten(stuff):
        for item in stuff:
            if isinstance(item, Relocation):
                yield item.pack()
            else:
                yield from _flatten(item)

    return b"".join(_flatten(entries))


def obfuscate_payload(data: bytes):
    input_size = len(data)
    pieces = lists.group(4, payload, "fill", b"\x00")
    acc = 0
    for i in range(len(pieces) - 1, 0, -1):
        old = u32(pieces[i])
        pieces[i] = p32(old ^ acc)
        acc = (acc + old) % 2**32
    return b"".join(pieces)[:input_size]


unstripped = ELF("lyla.clean.unstripped", checksec=False)

# Pass 1: Patch the program/section headers to enlarge .rela.dyn in-place.
stage1 = ELF("lyla.clean", checksec=False)
original_rela = stage1.section(".rela.dyn")
original_jmprel = stage1.section(".rela.plt")

extend_and_shift_relocation(stage1)

# Reload the file
with tempfile.NamedTemporaryFile() as tmpf:
    stage1.save(tmpf.name)
    stage2 = ELF(tmpf.name)

# Assume that the remaining of the page is unclaimed (next segments start at next page boundary and there are gap between first & second).
hdr_gap = BufferAllocator(stage2.write, "gap after header segment", *stage2.gap_after_load_segment(0))
# Make sure there are enough (executable) gap after .text for our code
assert list(stage2.load_segments)[1].section_in_segment(stage2.get_section_by_name(".text"))
text_gap = BufferAllocator(stage2.write, ".text gap", *stage2.gap_after_load_segment(1))

# Put shellcode into text gap
_dl_argv_ptr = text_gap.alloc(8, "&_dl_argv")
dest_addr_buf = text_gap.alloc(8, "dest for *dest = value")
value_buf = text_gap.alloc(8, "value for *dest = value")
# For size estimation; avoid zero in constants, it may change instruction encoding and thus size
prepare_sc = assemble_shellcode("check_inject_fini.s", PAYLOAD_SIZE_IN_WORDS=1, PAYLOAD_TIME_IMM_OFFSET=1)
assert len(prepare_sc) < 104, "Prepare shellcode must be shorter than 104 bytes"
prepare_sc_buf = text_gap.alloc(len(prepare_sc), "Shellcode for anti-gdb and hijacking fini")

time_ptr_buf = text_gap.alloc(8, "Pointer to time()")
payload_buf = text_gap.alloc(0)
payload = assemble_shellcode(
    "backdoor.s",
    PASSWORD_OFFSET=unstripped.symbols["g_password"] - payload_buf.address,
    REAL_FINI_OFFSET=unstripped.symbols["_fini"] - payload_buf.address,
    KEY0=ACTUAL_KEY[0],
    KEY1=ACTUAL_KEY[1],
    START_TIME=DECOY_START_TIME,
)
payload_buf = text_gap.alloc(len(payload), "Payload Shellcode").write(obfuscate_payload(payload))
prepare_sc = assemble_shellcode(
    "check_inject_fini.s",
    PAYLOAD_SIZE_IN_WORDS=(len(payload) + 3) // 4,
    PAYLOAD_TIME_IMM_OFFSET=payload.index(p32(DECOY_START_TIME)),
)
print(f"Inject Fini Shellcode Size: {len(prepare_sc)}")
print(f"Payload Size: {len(payload)}")
assert time_ptr_buf.address + time_ptr_buf.size == payload_buf.address

stage2.p8(stage2.symtab(SYMBOL_0) + 4, (SYMBOL_BINDINGS.LOCAL.value << 4) | SYMBOL_TYPES.NOTYPE.value)
stage2.p16(stage2.symtab(SYMBOL_0) + 6, SYMBOL_SECTION_INDEX.ABS.value)  # shndx
stage2.p64(stage2.symtab(SYMBOL_0) + 16, len(original_jmprel) - ELF64_RELA_SIZE)  # size
stage2.p64(stage2.symtab(SYMBOL_2) + 16, 8)  # size

backup_jmprel_buf = hdr_gap.alloc(len(original_jmprel), "backup jmprel").write(original_jmprel)
fake_fini_value_buf = hdr_gap.alloc(8, "fake fini value").write(p64(payload_buf.address + GARBAGE_CODE_SIZE))

rela_dyn = stage2.get_section_by_name(".rela.dyn")
rela_plt = stage2.get_section_by_name(".rela.plt")
strtab_vaddr = stage2.dynamic_value_by_tag("DT_STRTAB")
bss = stage2.get_section_by_name(".bss")
symbol_name_buffer = bss.header.sh_addr + bss.header.sh_size
ifunc_meta = Elf64SymMeta(0, 0x0A, 0, SYMBOL_SECTION_INDEX.ABS.value)
resolve_meta = Elf64SymMeta(symbol_name_buffer - strtab_vaddr, 0x12, 0, SYMBOL_SECTION_INDEX.UNDEF.value)
BACKDOOR_RELOC = [
    # Symbol 0 was setup for memcpy before entering backdoor reloc. However we need to fix size.
    Relocation(stage2.symtab(SYMBOL_0) + 16, RTYPE.SIZE64, addend=8),
    # Fetch DT_DEBUG _r_debug
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.RELATIVE, addend=stage2.dynamic_value_vaddr_by_tag("DT_DEBUG")),
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.COPY, symidx=stage2.symidx[SYMBOL_0]),
    # _r_debug + 8
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.R64, addend=8, symidx=stage2.symidx[SYMBOL_0]),
    # deref, get link_map
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.COPY, symidx=stage2.symidx[SYMBOL_0]),
    # link_map + 64 + 13 * 8
    Relocation(dest_addr_buf.address, RTYPE.R64, addend=64 + 13 * 8, symidx=stage2.symidx[SYMBOL_0]),
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.SIZE64, addend=RELOC_OBF_ADDEND),
]
WriteIMM64Obf = lambda addr, value: Relocation(
    addr, RTYPE.R64, addend=(value - RELOC_OBF_ADDEND) % 2**64, symidx=stage2.symidx[SYMBOL_0]
)
# These relocations are all for setup and can run in any order
BACKDOOR_RELOC += shuffled(
    [
        # Setup IFUNC call on symbol 1, symbol resolution on 2.
        Relocation(stage2.symtab(SYMBOL_1), RTYPE.SIZE64, addend=ifunc_meta),
        Relocation(stage2.symtab(SYMBOL_2), RTYPE.SIZE64, addend=resolve_meta),
        Relocation(value_buf.address, RTYPE.RELATIVE, addend=fake_fini_value_buf.address - 8),
        Relocation(stage2.symtab(SYMBOL_1) + 8, RTYPE.RELATIVE, addend=prepare_sc_buf.address),
        WriteIMM64Obf(symbol_name_buffer, u64(b"_dl_argv")),
    ]
    + [
        WriteIMM64Obf(prepare_sc_buf.address + i * 8, u64(piece))
        for i, piece in enumerate(lists.group(8, prepare_sc, "fill", b"\x00"))
    ]
)

BACKDOOR_RELOC += [
    # Resolve _dl_argv
    Relocation(_dl_argv_ptr.address, RTYPE.COPY, symidx=stage2.symidx[SYMBOL_2]),
    # Flush lookup cache: if we resolve the same symbol twice ld.so goes to cache instead of resolve it again.
    # We don't care about the symbol value, write to a soon-to-be-rewritten offset to discard it.
    Relocation(symbol_name_buffer, RTYPE.R64, symidx=stage2.symidx["__libc_start_main"]),
    # Resolve time
    WriteIMM64Obf(symbol_name_buffer, u64(b"time\x00\x00\x00\x00")),
    Relocation(time_ptr_buf.address, RTYPE.R64, symidx=stage2.symidx[SYMBOL_2]),
    # Call prepare_sc; write the return value (0) to the end of shellcode to clear the last "retn" instruction.
    Relocation(prepare_sc_buf.address + prepare_sc_buf.size - 8, RTYPE.R64, symidx=stage2.symidx[SYMBOL_1]),
    # Restore original JMPREL. We have carefully set up the DT_RELA/DT_JMPREL/DT_RELASZ so that ld.so would do
    # a second pass on JMPREL.
    Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.RELATIVE, addend=backup_jmprel_buf.address),
    Relocation(stage2.symtab(SYMBOL_0) + 16, RTYPE.SIZE64, addend=len(original_jmprel)),
    Relocation(rela_plt.header.sh_addr, RTYPE.COPY, symidx=stage2.symidx[SYMBOL_0]),
]

packed = pack_reloc(BACKDOOR_RELOC)
reloc_capacity = len(original_jmprel) // ELF64_RELA_SIZE - 1
print(f"Used relocation entries: {len(packed) // ELF64_RELA_SIZE} of {reloc_capacity}")
assert len(packed) < len(original_jmprel)
backdoor_reloc = hdr_gap.alloc(len(original_jmprel), "backdoor reloc").write(packed)

# --- First layer of decoding below

new_rela = [Relocation(stage2.symtab(SYMBOL_0) + 8, RTYPE.RELATIVE, addend=backdoor_reloc.address)]
new_rela.extend(map(Relocation.parse, lists.group(rela_dyn.header.sh_entsize, original_rela)))

for i, ent in enumerate(new_rela):
    if ent.symidx == stage2.symidx[SYMBOL_0]:
        remove_at = i
        break
new_rela.pop(remove_at)
for i, ent in enumerate(new_rela):
    if ent.type == RTYPE.COPY:
        first_copy = i
        break
new_rela.insert(
    first_copy,
    Relocation(
        offset=rela_plt.header.sh_addr,
        type=RTYPE.COPY,
        symidx=stage2.symidx[SYMBOL_0],
    ),
)
assert len(pack_reloc(new_rela)) == len(stage2.section(".rela.dyn"))

stage2.write_offset(rela_dyn._offset, pack_reloc(new_rela))
stage2.write_offset(rela_plt._offset, original_jmprel)

stage2.save("lyla")

print(hdr_gap)
print(text_gap)
