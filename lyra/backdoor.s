/* The following constants will be overriden during build. */
.equ PASSWORD_OFFSET, 0
.equ REAL_FINI_OFFSET, 0
.equ KEY0, 0
.equ KEY1, 0
.equ START_TIME, 0

.macro speckround x, y, k
    ror \x, 8
    add \x, \y
    xor \x, \k
    rol \y, 3
    xor \y, \x
.endm

#define time _begin-8

_begin:
    ret
    .byte 0x0f
    .byte 0x1f
    .byte 0x84

    // Check time
    xor edi, edi
    call [rip+time]
    sub eax, 3
    // We have to use a movabs (i.e. 64-bit immediate as some ld.so, including the one shipped by Ubuntu 22.04)
    // does not properly support R_X86_64_32. It would corrupt the code and cause segfault.
    movabs rdi, START_TIME /* startup_time, overwrite this in reloc */
    cmp edi, eax
    jnz fail

    // Password must be filled till the end
    cmp dword ptr [rip+_begin+PASSWORD_OFFSET+252], 0
    jz fail
    jmp okay

fail:
    xor eax, eax
    xor edi, edi
    xor esi, esi
    xor r8, r8
    xor r9, r9
    jmp _begin+REAL_FINI_OFFSET

okay:
    movabs r8, KEY0
    movabs r9, KEY1
    mov rdi, [rip+_begin+PASSWORD_OFFSET+128]
    mov rsi, [rip+_begin+PASSWORD_OFFSET+136]
    xor ecx, ecx
    encbody:
        speckround rsi, rdi, r8
        speckround r9, r8, rcx
        inc ecx
        cmp ecx, 32
        jnz encbody
    test rsi, rsi
    jnz fail
    test rdi, rdi
    jnz fail

win:
    mov eax, SYS_mprotect
    lea rdi, [rip+_begin+PASSWORD_OFFSET]
    mov rbx, rdi
    and rdi, ~0xFFF
    mov esi, 0x1000
    mov edx, 7
    syscall
    jmp rbx

// Our encoder can't encode last DWORD and instead use last dword as key
junk: .long 2322560982
