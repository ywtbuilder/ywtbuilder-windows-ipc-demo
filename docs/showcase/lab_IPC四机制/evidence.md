# lab_IPC四机制 展示证据页

## 一句话价值

用同一套 Sender/Receiver 框架对比实现 Windows 四种 IPC 机制，突出系统编程与边界处理能力。

## 1 分钟演示视频

- 文件：`docs/showcase/lab_IPC四机制/demo.mp4`
- 建议镜头：依次切换 Socket/共享内存/命名管道/邮件槽并发送同一消息

## 3 张关键截图

1. `shot-01.png`：Receiver 选择机制菜单
2. `shot-02.png`：Sender 发送消息与 Receiver 接收结果
3. `shot-03.png`：构建成功产物（`comm.lib`/`sender.exe`/`receiver.exe`）

## 一键运行命令（示例）

```powershell
cd lab_IPC四机制/Test_v3-master
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -Command 'Write-Host \"请使用 VS2022 打开 Test_v3.sln 并以 Debug|x64 构建\"'
```

## 核心技术决策

1. 通信库抽离：四机制共用 `comm.lib`，减少重复代码。
2. 同步治理：共享内存引入命名事件 + 互斥同步，避免竞态。
3. 边界处理：命名管道处理 `ERROR_PIPE_CONNECTED`，邮件槽避免无限阻塞。

## 性能/稳定性证据

| 指标 | 目标 | 当前结果 | 说明 |
|---|---:|---:|---|
| 四机制可运行率 | 100% | 待填充 | 每机制至少 3 轮 |
| 构建成功率 | 100% | 待填充 | `Debug|x64` |
| 异常恢复能力 | 可恢复 | 待填充 | 非法输入后回菜单 |

## 面试可提问点

1. 四种 IPC 的性能与适用场景差异是什么？
2. 为什么共享内存需要额外同步原语？
3. 命名管道为何会出现连接边界问题？
4. 邮件槽的一对多优势与限制是什么？
5. 如何把这套实现扩展到跨机器通信？

