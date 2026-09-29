# Blackhole Raymarching Renderer

纯 C 语言实现的黑洞光线追踪渲染器，基于测地线积分模拟黑洞周围的时空弯曲与吸积盘光影效果。

## ✨ 特性

- 测地线积分实时模拟光线在黑洞附近时空中的弯曲
- 可调参数（帧率、积分步数、观察距离、最内稳定轨道等）
- 吸积盘光影与公转动画
- 附带 VHS 脚本，一键生成 60 帧无损演示 GIF

## 🚀 快速开始（克隆后直接运行黑洞渲染）

### 1. 运行黑洞射线追踪渲染程序

```bash
# 编译
gcc -O3 main.c -lm -o blackhole

# 运行（启动黑洞渲染动画，按 Ctrl+C 退出）
./blackhole
```

> **依赖环境**：需要 gcc 编译器。
> - Windows：推荐使用 **Git Bash / MinGW / WSL**
> - Linux / macOS：系统自带 gcc，直接执行即可

### 2. 生成演示 GIF（需要额外安装 VHS）

`demo.tape` 是 [VHS](https://github.com/charmbracelet/vhs) 的录制脚本，用于自动录制终端画面并输出 GIF。**如果你只想看黑洞渲染动画，不需要安装 VHS，直接执行上面的编译运行即可。**

```bash
# 安装 VHS（一次性）
# 参考：https://github.com/charmbracelet/vhs

# 执行录制脚本，生成 60 帧 GIF
vhs demo.tape
```

生成的 GIF 输出到 `assets/demo.gif`。

> ⚠️ **Windows 注意**：`demo.tape` 中使用了 `Set Shell "bash"`，请使用 **Git Bash 或 WSL** 环境运行，PowerShell 直接执行会报错。

## 🔭 控制与调校

参数可以在可调变量节 `main.c` 中修改：

| 参数 | 默认值 | 描述 |
| --- | --- | --- |
| `FPS_TARGET` | `60.0` | 目标帧率 |
| `MAX_STEPS` | `420` | 每射线测地线积分步数 |
| `DPHI` | `0.030f` | 角积分步（rad） |
| `CAM_DIST` | `26.0 * RS` | 观察者距离奇点的距离 |
| `R_ISCO` | `3.0 * RS` | 最内稳定的圆形轨道 |

## 📦 编译说明

- `-O3`：最高级别优化，提升渲染速度
- `-lm`：链接数学库（射线追踪大量使用数学函数，**必须加**，否则编译失败）

## 📁 项目结构

```
├── main.c          # 黑洞射线追踪渲染主程序
├── demo.tape       # VHS 录制脚本（生成演示 GIF）
├── LICENSE         # MIT 许可证
├── assets/         # 存放生成的 GIF
└── .gitignore      # Git 忽略规则
```

## 📜 许可

本项目基于 MIT 许可证发布，欢迎随时使用、修改和分享。

详见 [LICENSE](./LICENSE)。
