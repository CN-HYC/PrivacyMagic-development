# 密算魔方

<div align = center><img src="./logo/logo.jpg" width="150px"> </div>

**"密算魔方"隐私计算平台**将安全性与灵活性相结合，实现“数据可用但不可见”。作为一个安全框架，它将联邦学习、安全多方计算和同态加密整合为模块化、即插即用的工具。

### 支持的密码学组件：

---

    **哈希工具**：SHA256、布谷鸟哈希、朴素哈希。
    **通信**：单轮通信。
    **不经意伪随机函数**：基于DH的OPRF。
### 构建

---

构建项目

```
  cd build
  cmake .. && make
  cd build/frontend/release
  ./CryptoMagic
```
