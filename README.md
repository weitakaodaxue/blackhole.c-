# blackhole.c-
 gargantua.c 是一个单文件、零依赖的纯 C 语言终端天体物理渲染器。无需任何图形库，仅凭标准库与纯数学推演，即可在字符终端中以 60 FPS 实时呈现《星际穿越》般的史瓦西黑洞。 它拒绝“贴图伪造”：每条光线均严格沿着广义相对论的零测地线方程进行数值积分。引力透镜弯折光线、光子球、爱因斯坦环，以及“黑洞上下方同时看到吸积盘”的奇观，皆是物理定律的自然涌现；结合 Shakura-Sunyaev 模型、相对论多普勒效应与引力红移，呈现出瑰丽的 ANSI 24 位真彩渐变。 底层采用双缓冲技术彻底杜绝终端闪烁，并支持窗口实时缩放。只需一行 gcc -O3 main.c -lm，即可在纯黑的命令行中见证属于程序员的时空浪漫。


# 🕳️ Schwarzschild Black Hole in Terminal (Pure C)

[![Language](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Dependencies](https://img.shields.io/badge/dependencies-zero-brightgreen.svg)](#)
[![FPS](https://img.shields.io/badge/framerate-60%20FPS%20Flicker--Free-orange.svg)](#)

A single-file, zero-dependency, real-time raytraced Schwarzschild black hole rendered inside your text terminal using ANSI 24-bit TrueColor.

![Schwarzschild Black Hole Demo](assets/demo.gif)

> *"Photons are not faked: every ray is integrated along a null geodesic of the Schwarzschild metric."*

---

## ✨ Highlights

- **Pure General Relativity**: Solves the null geodesic equation of motion:
  $$\frac{d^2u}{d\phi^2} = -u + \frac{3}{2} R_s u^2 \quad (u = 1/r)$$
  The event-horizon shadow, Einstein ring, photon ring, and the iconic accretion disk appearing both above and below the horizon emerge purely from spacetime curvature.
- **Relativistic Astrophysics**:
  - **Shakura-Sunyaev** accretion disk temperature profile ($T(r) \propto r^{-3/4}$).
  - **Relativistic Doppler Beaming**: Disk rotation causes the approaching side to flare bright cyan/white, while the receding side reddens and dims.
  - **Gravitational Redshift**: Light climbing out of the gravitational well loses energy.
- **Extreme Terminal Optimization**:
  - **Zero Flicker**: Employs double buffering; builds the full frame in heap memory and pushes it in a single `fwrite()` per frame.
  - **Zero Trig in Hot Loops**: Eliminates per-step `sin()` and `cos()` calls through incremental basis rotation.
  - **No `sprintf` Overhead**: Custom fast integer-to-decimal encoder (`put_u8`) for ANSI escape formatting.
  - **Color Deduplication**: Reuses escape sequences for identical consecutive cells, drastically reducing I/O throughput.
  - **Live Window Resize**: Real-time geometry tracking via `TIOCGWINSZ` (POSIX) / `GetConsoleScreenBufferInfo` (Win32).
  - **Clean Signal Handling**: Gracefully restores cursor and color palette on `Ctrl+C`.

---

## 🚀 Quick Start

### Requirements
- A terminal supporting **24-bit TrueColor** (Windows Terminal, iTerm2, Alacritty, Kitty, VS Code Terminal, WezTerm, etc.).
- Standard C compiler (`gcc`, `clang`, or `MSVC`).

### Build & Run

**Linux / macOS / MSYS2:**
```bash
gcc -O3 main.c -lm -o blackhole
./blackhole
