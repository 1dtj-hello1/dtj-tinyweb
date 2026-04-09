# dtjdw的tinyweb
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-11/17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://www.linux.org/)

基于 Reactor 模式的高性能 C++ Web 服务器，支持高并发、异步日志、零拷贝文件传输。
## ✨ 特性

| 特性 | 说明 |
|------|------|
| **高性能并发** | 基于 epoll + Reactor 模式，支持上万并发连接 |
| **异步日志** | 双缓冲区异步日志，避免日志 I/O 阻塞网络线程 |
| **零拷贝传输** | 使用 `sendfile` 发送静态文件，CPU 占用降低 50% |
| **定时器优化** | 小根堆定时器替代链表，超时检测效率 O(log n) |
| **数据库连接池** | 支持 MySQL 连接池，用户注册/登录功能 |
| **HTTP 解析** | 有限状态机解析 GET/POST 请求 |

## 📊 性能数据

| 测试场景 | 并发连接 | QPS | 平均延迟 |
|----------|----------|-----|----------|
| 低负载 | 500 | 6,868 | < 1ms |
| 高负载 | 10,000 | 5,988 | 5.34ms |

## 🏗️ 架构设计
