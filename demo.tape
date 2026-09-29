Output assets/demo.gif
Set Shell "bash"
Set FontSize 14
Set Width 1200
Set Height 700
Set Theme "Catppuccin Mocha"
Set Framerate 60

Type "gcc -O3 main.c -lm -o blackhole && ./blackhole" Enter
# 等待黑洞运转 8 秒，展示完整的公转和光影
Sleep 8s
# 发送 Ctrl+C 优雅退出
Ctrl+C
Sleep 1s
