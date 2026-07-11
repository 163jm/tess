# libmpv 开发包获取与放置说明

阶段 1 使用**现成的预编译 libmpv 开发包**，不自己编译 mpv。

## 下载

1. 打开 <https://github.com/shinchiro/mpv-winbuild-cmake/releases>
2. 找到最新的 release，下载文件名类似：
   `mpv-dev-x86_64-YYYYMMDD-git-xxxxxxx.7z`
   （注意选 **mpv-dev**，不是 mpv-x86_64（那个是播放器本体，不含头文件/库）)
3. 解压后你会看到大致这样的结构：
   ```
   include/
     mpv/
       client.h
       render.h
       render_gl.h
       stream_cb.h
       ...
   libmpv.dll.a      <-- mingw 导入库
   mpv-2.dll (或 libmpv-2.dll)   <-- 运行时 DLL
   ```

## 放置到本项目

把解压出来的内容对应放到：

```
third_party/libmpv/
├── include/
│   └── mpv/
│       ├── client.h
│       ├── render.h
│       ├── render_gl.h
│       └── ...
└── lib/
    ├── libmpv-2.dll        <-- 运行时 DLL（如果解压出来叫 mpv-2.dll，改名为 libmpv-2.dll，或修改 CMakeLists.txt 里的 LIBMPV_DLL 变量）
    └── libmpv.dll.a         <-- 可选，若用 MSVC 直接链接见下方说明
```

## 关于 MSVC 链接 mingw 产物的说明

`libmpv.dll.a` 是 **mingw 格式**的导入库，MSVC 的链接器（link.exe）**无法直接使用**它。
本项目采用更稳妥的方式：**运行时动态加载**（`LoadLibraryW` + `GetProcAddress`），
不需要在链接期依赖 `.dll.a` 或生成 `.lib`。这样只要有 `client.h` 头文件和
`libmpv-2.dll` 运行时文件即可，规避了 ABI 链接问题。

`Core/MpvLoader.h/.cpp` 里实现了这个动态加载封装，所有 `mpv_*` 函数都通过
函数指针调用。你只需确保：

1. `third_party/libmpv/include/mpv/client.h`（以及 `render.h`、`render_gl.h`）存在，供编译期使用其中的类型/宏定义
2. `third_party/libmpv/lib/libmpv-2.dll` 存在，构建脚本会在编译后自动拷贝到
   可执行文件所在目录（见 `App/CMakeLists.txt` 里的 `add_custom_command`）

## 验证

放置完成后，目录里至少应该能看到这两个文件：

```
third_party/libmpv/include/mpv/client.h
third_party/libmpv/lib/libmpv-2.dll
```

CMake 配置阶段若缺少 `client.h` 会打印警告（不会中断配置，但编译 Core 会失败）。
