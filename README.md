
<div align="center">

# 🕳️ Blackhole · Gargantua

**单文件 · 零依赖 · 纯 C 语言实现的 60 FPS 终端史瓦西黑洞射线追踪器**

[![Language](https://img.shields.io/badge/Language-C99-00599C?logo=c&logoColor=white)](#)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](./LICENSE)
[![Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen.svg)](#)
[![Performance](https://img.shields.io/badge/Framerate-60_FPS_Flicker--Free-orange.svg)](#)
[![Color](https://img.shields.io/badge/Color-ANSI_24--bit_TrueColor-blueviolet.svg)](#)

<br />

<!-- 建议在此处展示录制好的演示动图 -->
<img src="assets/demo.gif" alt="Blackhole Raymarching Demo" width="90%" style="border-radius: 8px; box-shadow: 0 4px 20px rgba(0,0,0,0.5);" />

<p align="center">
  <em>“光子绝非贴图伪造：每一道光线皆沿史瓦西时空的零测地线精确数值积分。”</em>
</p>

</div>

---

## 📖 项目简介

`blackhole` 是一个单文件、零依赖的纯 C 语言终端天体物理仿真渲染器。无需任何 OpenGL、DirectX 或 ncurses 图形库支持，仅凭标准库与严谨的数学推演，即可在字符终端中以 60 FPS 实时呈现《星际穿越》般的史瓦西黑洞与炽热吸积盘。

引力透镜弯折光线、光子球、爱因斯坦环，以及“黑洞上下方同时看到吸积盘”的时空奇观，皆是广义相对论测地线方程自然涌现的物理真理。

---

## ✨ 核心特性

### 🌌 硬核天体物理仿真
* **真实测地线数值积分**：基于史瓦西度规求解零测地线运动方程：
  $$\frac{d^2u}{d\phi^2} = -u + \frac{3}{2} R_s u^2 \quad (u = 1/r)$$
* **相对论多普勒集束效应（Doppler Beaming）**：吸积盘朝向观测者运动的一侧剧烈蓝移增亮，远离的一侧红移变暗。
* **引力红移（Gravitational Redshift）**：模拟光子爬出极端引力势阱时的能量衰减。
* **Shakura-Sunyaev 盘温模型**：复现标准薄盘辐射特性，内环边缘（ISCO）炽热发光，外环呈极光蓝渐变延伸。

### ⚡ 极致底层工程优化
* **单文件零依赖（Zero-Dependency）**：仅需 C99 标准库与数学库（`-lm`），开箱即用。
* **绝对零闪烁架构**：全帧字符与 ANSI 转义序列在内存堆栈中双缓冲组装，每帧单次 `fwrite` 输出，摒弃低效的逐格清屏。
* **热循环免三角函数**：测地线步进循环中通过增量基底旋转消除高昂的 `sin/cos` 运算，普通 CPU 轻松跑满 60 帧。
* **24-bit TrueColor 真彩色**：结合自适应灰度字符密度集（`.:-=+*#%@`），呈现平滑细腻的高动态光影。
* **终端窗口动态自适应**：监听 `TIOCGWINSZ`（类 Unix）与控制台事件（Windows），拖动缩放窗口实时重绘。

---

## 🚀 快速开始

只需标准 GCC 编译器，**无需配置任何构建工具或环境依赖**。

### 1. 编译并运行

```bash
# 1. 克隆本仓库
git clone https://github.com/yourusername/blackhole.git
cd blackhole

# 2. 编译（-O3 开启最高级优化，-lm 链接数学库不可省略）
gcc -O3 main.c -lm -o blackhole

# 3. 运行（推荐放大终端窗口或全屏体验）
./blackhole

[!TIP] 退出方式：程序已注册 SIGINT 信号监听，随时按下 Ctrl + C 即可安全退出，终端光标与色彩将自动完美恢复。

2. 终端环境建议

本项目使用 ANSI 24-bit TrueColor 进行色彩映射，请确保在支持真彩色的现代终端中运行：

  - Windows：Windows Terminal、VS Code 内置终端、MSYS2 / MinGW
  - macOS：iTerm2、Terminal.app、Alacritty、Kitty
  - Linux：GNOME Terminal、Alacritty、Kitty、WezTerm 等

🔭 参数调校指南

所有物理与性能参数均在 main.c 顶部的 Tunables 宏定义区，可按需微调：

| 参数名称         | 默认值          | 作用解析与调优建议                               |
| :----------- | :----------- | :-------------------------------------- |
| `FPS_TARGET` | `60.0`       | **目标渲染帧率**（可降至 `30.0` 进一步节省低功耗设备资源）     |
| `MAX_STEPS`  | `420`        | **单条光线最大积分步数**（调高增加极端引力透镜精细度，调低提高帧率）    |
| `DPHI`       | `0.030f`     | **角积分步长（rad）**（决定数值积分的平滑粒度）             |
| `CAM_DIST`   | `26.0f * RS` | **观测相机与奇点的距离**（调节视场远近）                  |
| `R_ISCO`     | `3.0f * RS`  | **最内稳定圆轨道（ISCO）**（吸积盘物理内边缘，低于此范围物质坠入视界） |
| `R_OUT`      | `13.0f * RS` | **吸积盘外边缘半径**                            |

🎥 录制高清演示动图（可选）

项目内置了针对 VHS (Charm) 的自动化录制脚本 demo.tape，可一键复现 60 FPS 无损像素级终端动图。

# 安装 VHS（如已安装可跳过）
# macOS: brew install vhs
# Linux: 包管理器安装详见 VHS 官方仓库

# 执行自动化无损录制脚本
vhs demo.tape

[!NOTE] 导出的 GIF 文件将自动保存在 assets/demo.gif。由于 demo.tape 声明使用 Bash Shell，Windows
用户请在 Git Bash 或 WSL 环境下调用 vhs。

📁 目录结构

.
├── main.c          # 史瓦西黑洞射线追踪主程序（集成测地线积分引擎）
├── demo.tape       # VHS 终端自动化 60 FPS 录屏脚本
├── LICENSE         # MIT 开源授权协议
└── assets/         # 存放 README 封面图与动态演示物料
    └── demo.gif    # 生成的黑洞动态演示动图

📜 许可证 (License)

本项目遵循 MIT 许可证 开源。您可以完全自由地用于个人学习、魔改创作、学术演示或商业项目。欢迎提交 Issue 或 Pull Request
探讨相对论渲染算法！

