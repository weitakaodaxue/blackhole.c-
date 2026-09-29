# Blackhole Raymarching Renderer
纯 C 语言实现的黑洞射线追踪渲染器，利用**测地线积分**模拟黑洞附近时空弯曲，还原引力透镜效应与吸积盘动态光影。

## ✨ 特性
- 基于测地线积分，实时模拟光线在弯曲时空中的传播与引力透镜效果
- 可自定义渲染参数：帧率、积分步数、观测距离、ISCO最内稳定圆轨道等
- 动态吸积盘与公转动画，终端 ASCII 字符渲染
- 内置 VHS 自动化录制脚本，一键导出 60fps 高清演示 GIF

## 🚀 快速开始
> 克隆仓库后，只需 GCC 编译器即可直接运行黑洞渲染，**无需任何第三方图形库**
### 1. 编译并运行黑洞渲染
```bash
# 编译（-O3开启最高优化，-lm链接数学库，不可省略）
gcc -O3 main.c -lm -o blackhole
# 启动终端黑洞动画，Ctrl+C 退出程序
./blackhole
```
**环境依赖**
- Windows：推荐 Git Bash / MinGW / WSL（PowerShell 不推荐直接运行）
- Linux / macOS：系统自带 GCC，开箱即用

### 2. 自动生成演示 GIF（可选）
> 仅当你需要制作项目预览动图时才需要安装 VHS；单纯运行黑洞动画不需要这个工具。
`demo.tape` 是 [VHS](https://github.com/charmbracelet/vhs) 终端录制脚本，自动录制终端画面输出 GIF。
```bash
# 安装 VHS，参考官方仓库：https://github.com/charmbracelet/vhs
# 执行录制脚本，输出 60fps GIF
vhs demo.tape
```
生成文件路径：`assets/demo.gif`

> ⚠️ Windows 提示：脚本指定 `Set Shell "bash"`，必须在 Git Bash / WSL 内执行，原生 PowerShell 会执行失败。

## 🔭 参数调校
在 `main.c` 头部修改下述参数，调整黑洞渲染效果与性能：
| 参数 | 默认值 | 描述 |
| --- | --- | --- |
| `FPS_TARGET` | `60.0` | 渲染目标帧率 |
| `MAX_STEPS` | `420` | 单条光线测地线积分最大步数（数值越高精度越高，渲染越慢） |
| `DPHI` | `0.030f` | 角积分步长，单位 rad（弧度） |
| `CAM_DIST` | `26.0 * RS` | 观测相机距离黑洞奇点的距离 |
| `R_ISCO` | `3.0 * RS` | 黑洞最内稳定圆轨道半径 |

## 📦 编译参数说明
- `-O3`：GCC最高级别编译优化，大幅提升渲染速度
- `-lm`：链接数学库，项目大量使用三角函数与浮点运算，**该参数不可省略，否则编译报错**

## 📁 项目结构
```
├── main.c          # 黑洞射线追踪主代码（测地线积分核心）
├── demo.tape       # VHS 终端录制脚本，用于生成演示GIF
├── LICENSE         # MIT开源许可证
├── assets/         # 存放 VHS 导出的GIF文件
│   └── .gitkeep    # Git占位文件，保留空目录
└── .gitignore      # Git忽略配置，过滤编译产物与生成的GIF
```

## 📜 License
This project is released under the MIT License.
欢迎自由使用、修改、分发、二次开发。
完整协议请看 [LICENSE](./LICENSE) 文件。
