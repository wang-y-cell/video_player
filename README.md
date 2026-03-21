# Video Player

这是一个基于 C++17 开发的跨平台视频播放器。项目核心利用 **FFmpeg** 进行音视频解封装与解码，并使用 **SDL3** 进行音视频渲染与播放。

项目采用 CMake 作为构建系统，并针对不同操作系统采用了不同的包管理策略，以实现真正的跨平台开发：
- **Windows**: 使用 [vcpkg](https://vcpkg.io/) (Manifest 模式) 自动管理和编译依赖。
- **Linux**: 使用原生的 `pkg-config` 寻找系统级依赖。

---

## 依赖要求

在编译本项目之前，请确保您的系统满足以下基本要求：
- **C++ 编译器**: 支持 C++17 标准 (如 MSVC, GCC 8+, Clang 8+)
- **CMake**: 3.12 或更高版本
- **核心库**:
  - FFmpeg (包含 avformat, avcodec, avutil, swscale, swresample)
  - SDL3
  - Threads

---

## 编译教程：Windows

在 Windows 平台上，我们推荐使用 `vcpkg` 配合 CMake 来自动处理依赖下载与编译。

### 1. 准备 vcpkg
确保您已经安装了 vcpkg，并执行了全局集成：
```powershell
# 在您的 vcpkg 安装目录下执行
.\vcpkg integrate install
```

### 2. 处理网络问题（可选但强烈建议）
由于 vcpkg 下载源码时底层使用 `curl`，在国内网络环境下（尤其是使用代理时）极易出现 `SSL connect error` (Error 35)。
在配置 CMake 之前，**请务必在终端中设置好代理环境变量**，强制指定 `http://` 协议：
```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"  # 请将 7897 替换为您的实际代理端口
$env:HTTPS_PROXY="http://127.0.0.1:7897"
```

### 3. 配置与编译
在项目根目录下打开 PowerShell 终端，执行以下命令：

```powershell
# 1. 配置项目（vcpkg 将在此阶段自动下载并编译 FFmpeg 和 SDL3，可能需要较长时间）
# 注意：请将 CMAKE_TOOLCHAIN_FILE 的路径替换为您本机 vcpkg 的实际路径
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="<您的vcpkg路径>/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows

# 2. 编译项目
cmake --build build --config Debug
```

编译成功后，可执行文件 `player.exe` 以及自动拷贝过来的依赖 `.dll` 文件将位于 `bin/Debug/` 目录下。

---

## 编译教程：Linux (Ubuntu/Debian 示例)

在 Linux 平台上，推荐直接使用系统包管理器（如 `apt`）安装预编译的开发库，并通过 `pkg-config` 让 CMake 找到它们。

### 1. 安装系统依赖
打开终端，更新包列表并安装所需的开发包：
```bash
sudo apt update

# 安装编译工具和 CMake
sudo apt install build-essential cmake pkg-config

# 安装 FFmpeg 开发库
sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev

# 安装 SDL3 开发库
# 注意：如果您的软件源中尚未提供 SDL3，可能需要从源码编译安装 SDL3
sudo apt install libsdl3-dev 
```

### 2. 配置与编译
在项目根目录下执行标准 CMake 构建流程：

```bash
# 1. 生成构建目录
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# 2. 编译项目
cmake --build build -j$(nproc)
```

编译成功后，可执行文件 `player` 将输出在根目录的 `bin/` 文件夹下。

---

## 运行程序

在相应的系统下，进入存放可执行文件的目录，直接运行播放器即可：

**Windows:**
```powershell
.\bin\Debug\player.exe
```

**Linux:**
```bash
./bin/player
```