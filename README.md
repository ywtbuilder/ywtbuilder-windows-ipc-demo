# IPCForge — Windows 进程间通信四机制演示

> 使用 C++ 与 Win32 API 实现的 Windows IPC 四机制对比演示系统，涵盖 Socket、共享内存、命名管道、邮件槽。

![C++](https://img.shields.io/badge/C++-17-blue?logo=cplusplus)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![Winsock2](https://img.shields.io/badge/Network-Winsock2-green)
![MSVC](https://img.shields.io/badge/Build-MSVC-purple)

---

## Showcase

### 一句话价值

在统一框架中对比实现四类 Windows IPC 机制，突出底层通信与边界处理能力。

### 1分钟演示视频

- [demo.mp4](docs/showcase/lab_IPC四机制/demo.mp4)

### 3张关键截图

1. [shot-01.png（机制选择菜单）](docs/showcase/lab_IPC四机制/shot-01.png)
2. [shot-02.png（消息发送与接收）](docs/showcase/lab_IPC四机制/shot-02.png)
3. [shot-03.png（构建产物）](docs/showcase/lab_IPC四机制/shot-03.png)

### 一键运行命令（示例）

```powershell
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -Command 'Write-Host \"请使用 VS2022 打开 Test_v3.sln 并构建 Debug|x64\"'
```

### 核心技术决策

1. 抽离 `comm.lib`，让 Sender/Receiver 共用通信实现。
2. 共享内存链路增加命名事件与互斥，减少竞态与空读。
3. 命名管道和邮件槽补齐边界处理与可恢复交互。

### 性能/稳定性证据

- 证据页：[evidence.md](docs/showcase/lab_IPC四机制/evidence.md)
- 建议展示指标：四机制成功率、构建成功率、异常输入恢复能力。

### 面试可提问点

1. 四机制在吞吐、时延、可扩展性上的差异是什么？
2. 共享内存为何必须引入同步原语？
3. 这套实现如何迁移到跨机器通信场景？

---

## 功能特性

四种 Windows 进程间通信机制，通过交互式菜单选择：

| 机制 | 特点 | 适用场景 |
|------|------|---------|
| **Socket** | 网络透明，支持跨主机 | 分布式进程通信 |
| **Shared Memory** | 零拷贝，最高吞吐 | 大数据量同机通信 |
| **Named Pipe** | 半双工，有序传输 | 父子进程通信 |
| **Mailslot** | 单向广播，无连接 | 一对多消息推送 |

---

## 系统架构

```text
┌──────────────────────────┐    ┌──────────────────────────┐
│        Sender.exe         │    │       Receiver.exe        │
│                           │    │                           │
│   [1] Socket              │◄──►│   [1] Socket              │
│   [2] Shared Memory       │◄──►│   [2] Shared Memory       │
│   [3] Named Pipe          │◄──►│   [3] Named Pipe          │
│   [4] Mailslot            │───►│   [4] Mailslot            │
│                           │    │                           │
└──────────────────────────┘    └──────────────────────────┘
          │                                  │
          └──────────  comm.lib  ────────────┘
                  （公共通信静态库）
```

| 模块 | 说明 |
|------|------|
| `sender/sender.cpp` | 发送端主程序，交互菜单 + 调用 comm 库 |
| `Test_v3/receiver.cpp` | 接收端主程序，交互菜单 + 调用 comm 库 |
| `comm/comm.c` | 公共通信逻辑（四机制实现） |
| `comm/comm.h` | 接口头文件 |

---

## 本地运行（Visual Studio，推荐）

### 环境要求
- Windows 10+
- Visual Studio 2022（含 MSVC v143 工具链）

### 构建步骤（Debug|x64）
1. 使用 Visual Studio 打开 `Test_v3.sln`
2. 将解决方案配置切换为 `Debug | x64`
3. 执行 `Build Solution`
4. 确认生成 `receiver.exe`、`sender.exe`、`comm.lib`

### 运行步骤（交互菜单）
1. 先启动 `receiver` 项目（或 `receiver.exe`）
2. 在 Receiver 中选择通信机制编号（`1~4`）
3. 再启动 `sender` 项目（或 `sender.exe`）
4. 在 Sender 中选择同一机制编号并输入消息
5. Receiver 端应输出收到的消息

---

## 四机制推荐启动顺序与期望现象

### 1) Socket
- 顺序：Receiver 选 `1` -> Sender 选 `1` 并发送
- 期望：Receiver 打印 `Bytes received` 与 `Message`

### 2) Shared Memory
- 顺序：Receiver 选 `2` -> Sender 选 `2` 并发送
- 期望：Receiver 稳定打印 `Received message`
- 说明：使用命名事件 + 互斥量同步，避免空读和竞态

### 3) Named Pipe
- 顺序：Receiver 选 `3` -> Sender 选 `3` 并发送
- 期望：Receiver 打印 `Received message`
- 说明：处理了 `ERROR_PIPE_CONNECTED` 边界情况

### 4) Mailslot
- 顺序：Receiver 选 `4` -> Sender 选 `4` 并发送
- 期望：Receiver 接收一条消息后返回菜单
- 说明：不再无限循环阻塞

---

## 常见失败与排查

### 1) `comm.h` 找不到 / 构建失败
- 现象：`error C1083: cannot open include file: "comm.h"`
- 排查：
  - 确认打开的是 `Test_v3.sln`
  - 确认配置为 `Debug|x64`
  - 执行一次 `Rebuild Solution`
  - 检查 `sender.vcxproj` / `receiver.vcxproj` 的包含目录是否为 `$(SolutionDir)comm`

### 2) 程序看起来“卡住”
- 现象：Receiver 长时间无输出
- 排查：
  - 是否已在 Sender 与 Receiver 中选择了相同机制
  - 是否先启动 Receiver 再启动 Sender
  - Shared Memory / Mailslot 在未收到消息前会等待（有超时提示）

### 3) 机制选择不一致
- 现象：无消息、超时或连接失败
- 排查：
  - Sender 与 Receiver 必须选择同一个编号
  - 每轮通信完成后再进行下一轮选择

---

## 技术栈

| 技术 | 用途 |
|------|------|
| Win32 API | 共享内存 / 管道 / 邮件槽 |
| Winsock2 | TCP Socket 通信 |
| 静态库（`.lib`）| 通信逻辑与主程序解耦 |
| MSVC | 编译工具链 |

---

## License

MIT © 2026

