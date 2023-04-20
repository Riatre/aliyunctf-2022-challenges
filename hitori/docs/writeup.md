## hitori

有两个预期的 bug：

1. Resize 的时候没有初始化新的画布 buffer，这里有一个未初始化，恰当的按摩堆之后可以泄漏出堆地址和 libc 地址。
2. 插件系统里面大量使用了以下形式的单例：

```cpp
class EdgeDetector {
 public:
  static EdgeDetector& GetInstance() {
    static EdgeDetector inst;
    return inst;
  }
```

对于头文件中的这种 `inline static` 符号，GCC 会将其符号类型置为 `STB_GNU_UNIQUE`，这种类型的符号有一些[有趣的特点](https://maskray.me/blog/2021-04-25-weak-symbol#:~:text=is%20another%20binding-,STB_GNU_UNIQUE,-%2C%20which%20is%20like)。ld.so 当且仅当在当前 .so 中有新出现的 `STB_GNU_UNIQUE` 时才会把对应 so 设为 NODELETE。只要 so 中没有新出现的 `STB_GNU_UNIQUE`，即使其引用了其他的 `STB_GNU_UNIQUE` 符号，这个 so 也是可以被卸载掉的。卸载时会触发所有在本 so 内注册的 atexit handler，从而会触发所有在本 .so 内初始化的 static 变量，即使该 static 变量实际由于 `STB_GNU_UNIQUE` 被解析到了其他 so 中。这会在其他 so 后续使用该单例的时候引发 UAF。

发现这个 UAF 和搞清楚怎么触发之后，剩下的就都是体力活了。具体利用见 solve.py。
