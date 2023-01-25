## lyra

解题脚本见 solve.py。

题目表面上的逻辑是一个无解题：经典的“输入密码，检查密码是否正确，正确则读取并输出 flag 文件内容”。但验证逻辑为“固定 32 字节的输入，使用输入作为 key 运行 Speck (128/128) 加密算法，要求结果等于另一固定值”，该验证逻辑在该密码算法不被攻破的情况下是不可能被解出的。

程序里藏有常见逆向工具（测试了 IDA Pro、objdump、readelf、Ghidra）不会直接显示的后门，后门主体代码编码后放置在 .text section 所属的 segment 结尾的填充数据中，ELF 头不会将这段数据标记为需要加载，但 Linux 实际映射文件内容进内存的时候是按页对齐的，是可以访问到的。后门代码的解码和触发逻辑藏在了重定位表里，重定位表的结构这里不再赘述，感兴趣的同学可以自行翻阅 glibc 源码中的 `glibc/sysdeps/x86_64/dl-machine.h`。

本题利用了以下几个重定位类型：

* `R_X86_64_SIZE64` - 指定 symbol index = 0，则该重定位项的实际效果为写 `*(uint64_t*)(elf_base+addr) = (addend+0);`
* `R_X86_64_COPY` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `memcpy(elf_base+addr, symbol_value, symbol_size);`
* `R_X86_64_RELATIVE` - `*(uint64_t*)(elf_base+addr) = (elf_base+addend);`
* ``
