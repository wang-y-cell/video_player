# Video Player

这是一个基于 C++17 开发的跨平台视频播放器。项目核心利用 **FFmpeg** 进行音视频解封装与解码，并使用 **SDL3** 进行音视频渲染与播放。

项目采用 CMake 作为构建系统，并针对不同操作系统采用了不同的包管理策略，以实现真正的跨平台开发：
- **Windows**: 使用本机安装的 FFmpeg/SDL3（通过 CMake 变量配置路径）。
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

在 Windows 平台上，需要你预先安装 FFmpeg 与 SDL3，并在 CMake 配置阶段传入它们的路径。
```CMakeLists.txt
if(NOT DEFINED FFMPEG_ROOT)
          set(FFMPEG_ROOT "F:\\wy\\ffmpeg-master-latest-win64-gpl-shared") <-------修改路径
     endif()
     if(NOT DEFINED SDL3_ROOT)
          set(SDL3_ROOT "F:\\wy\\SDL3") <---------修改路径
     endif()

```
### 1. 准备依赖

需要准备两个目录（示例结构如下）：

- `FFMPEG_ROOT`
  - `include/`
  - `lib/`
  - `bin/`
- `SDL3_ROOT`
  - `include/`
  - `lib/`
  - `bin/`

本项目的 Windows 配置会读取 `FFMPEG_ROOT` 与 `SDL3_ROOT` 变量（见 [CMakeLists.txt](file:///f:/wy/code/vscode/c++/video_player/CMakeLists.txt)），你可以在运行 CMake 时用 `-DFFMPEG_ROOT=... -DSDL3_ROOT=...` 传入。

### 2. 配置与编译
在项目根目录下打开 PowerShell 终端，执行以下命令：

```powershell
# 1. 配置项目（请把路径替换为你本机实际安装位置）
cmake -B build -S . -G "MinGW Makefiles" `
  -DFFMPEG_ROOT="F:\wy\ffmpeg-master-latest-win64-gpl-shared" `
  -DSDL3_ROOT="F:\wy\SDL3"

# 2. 编译项目
cmake --build build
```

编译成功后，可执行文件 `player.exe` 将输出到 `bin/` 目录。

运行与调试时请确保以下目录在 `PATH` 中，否则可能出现 `0xc0000135`（缺少 DLL）：

- `${FFMPEG_ROOT}\bin`
- `${SDL3_ROOT}\bin`
- `MinGW\bin`（例如 `F:\mingw\mingw64\bin`）

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
.\bin\player.exe
```

**Linux:**
```bash
./bin/player
```
