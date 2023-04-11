## dddddddd

最新版 V8，没有公开的洞，也不需要漏洞，用力阅读 C++ 代码即可。Patch 除了去掉了 D8 的一堆直接运行命令的功能以外，最主要的是把 ValueSerializer 里可以存 WasmModule 的功能搬回来了。

稍微观察下可以注意到存下来的 WasmModule 里有 TurboFan 编译好的代码，直接改成 Shellcode，反序列化回来，调用即可执行。

代码参考同目录下的 `solve.py` 和 `gen.js`。