/* The following constants will be overriden during build. */
.equ PASSWORD_OFFSET, 0
.equ REAL_FINI_OFFSET, 0
.equ KEY0, 0
.equ KEY1, 0
.equ DT_DEBUG_OFFSET, 0

.macro speckround x, y, k
    ror \x, 8
    add \x, \y
    xor \x, \k
    rol \y, 3
    xor \y, \x
.endm

#define time_value (_begin-8)

_begin:
    xor ecx, ecx
    .byte 0x48
    .byte 0xba
    call fetchtime

    /* Check time: this is an obfuscated version of checking time_value / 119 - edx == 3 */ 
    mov rdi, rdx
    mov eax, [rip+time_value]
    rol rdx, 2
    sub rax, rdx
    mov edx, 0x61f2a4bb
    mul rdx
    sub rax, rdi
    cmp eax, 0x68a04739
    jnz fail

    /* Password must be filled till the end */
    cmp dword ptr [rip+_begin+PASSWORD_OFFSET+252], 0
    jz fail
    jmp okay

sfail:
    add rsp, 8
fail:
    push 0
    pop rdx
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
    mov r8, rdi
    and rdi, ~0xFFF
    mov esi, 0x1000
    mov edx, 7
    syscall
    call r8

fetchtime:
    /* Find out where is VDSO */
    lea rax, [rip+1]
    and rax, ~0xFFF
    add rax, DT_DEBUG_OFFSET
    /* _r_debug */
    mov rax, [rax]
    test rax, rax
    jz sfail
    /* link_map */
    mov rax, [rax+8]
    test rax, rax
    jz sfail
    /* l_next */
    mov rax, [rax+24]
    test rax, rax
    jz sfail
    /* 
      assuming this is linux-vdso.so.1's link_map, don't validate, instead just
      fail if we can't find clock_gettime.
    */
    /* l_addr */
    mov rdx, [rax]
    /* fail if vdso located below stack (happens only when randomize_va_space=0)*/
    cmp rdx, rsp
    jb sfail

    /* HASH */
    mov rcx, [rax+64+4*8]
    test rcx, rcx
    jz sfail
    mov rcx, [rcx+8]
    add rcx, rdx
    mov ecx, [rcx+4]    
    cmp ecx, 0x7f
    ja sfail

    /* STRTAB */
    mov rdi, [rax+64+5*8]
    test rdi, rdi
    jz sfail
    mov rdi, [rdi+8]
    add rdi, rdx

    /* STRSZ */
    mov r8, [rax+64+0xa*8]
    test r8, r8
    jz sfail
    mov r8, [r8+8]
    cmp r8, 0x400
    ja sfail

    /* SYMTAB */
    mov rsi, [rax+64+6*8]
    test rsi, rsi
    jz sfail
    mov rsi, [rsi+8]
    add rsi, rdx

    /* find clock_gettime */
    mov r9, [rax+8]
    test r9, r9
    jz sfail
    mov r9, [r9]
find_loop:
    mov eax, [rsi]
    cmp rax, r8
    jae sfail
    add rax, rdi
    mov r11, r9
hash_loop:
    cmp byte ptr [rax], 0
    jz hash_loop_end
    imul r11, r11, 33
    xor r11b, byte ptr [rax]
    inc rax
    jmp hash_loop
hash_loop_end:
    cmp r11d, 0x45576f89 /* for __vdso_clock_gettime */
    jz found
    add rsi, 24
    loop find_loop
    jmp sfail
found:
    mov rax, [rsi+8]
    add rax, rdx
    xor edi, edi
    sub rsp, 16
    mov rsi, rsp
    cmp byte ptr [rax], 0xcc
    jz sfail
    call rax
    pop rdx
    pop rdi
    test eax, eax
    jnz sfail
    push 0
    push 0
    add rsp, 16
    ret

/* Our encoder can't encode last DWORD and instead use last dword as key */
junk: .long 2322560982
