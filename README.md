# CryptoMagic

[Chinese](README_CN.md "中文版")                      [Instruction](https://blog.csdn.net/weixin_45993094/article/details/150761261?spm=1011.2415.3001.5331)

<div align = center><img src="./logo/logo.jpg" width="150px"> </div>

**CryptoMagic Privacy Computing Platform** merges "crypto" security with "magic" flexibility, enabling "data usable but not visible." As a dynamic hub, it integrates federated learning, secure multi-party computation, and homomorphic encryption into modular, plug-and-play tools.

### Supported cryptographic components：

---

    **Hash**：SHA256, Cuckoo Hash, Simple Hash.

    **Communication:** single round.

    **OPRF:** DH-based.

    **cryptoTools**: PRNG.

### Tools that need to be installed in advance

---

1. gcc && g++ (version >= 11, support C++20)
2. cmake (version >= 3.10)
3. OpenSSL (version >= 3.0)

### build

---

```
cd build
cmake .. && make
cd build/frontend/release
./CryptoMagic
```
