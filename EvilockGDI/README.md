# EvilockGDI：给小朋友的 Windows 画板特效库（C++20 / GDI）

这是一个“像 Python 一样好上手”的 Windows 画板库：你可以用**画笔**画线、画多边形、画圆；也可以用 **PixelCanvas** 做 **旋转 / 缩放 / 平移 / 真透视（像 3D 翻转）/ 鱼眼** 这些特效。

> 安全提示：有些特效会闪烁/移动/弹窗，请在家长或老师陪同下使用，不要在上课或重要场合乱跑。

---

## 1）儿童模式（推荐）：`kids.hpp` —— 像 Python 一样写

儿童模式的目标：**你不需要懂消息循环、线程等待、HDC**。你只要记住 4 个词：

- `open(...)`：打开画板窗口
- `present()`：把你画的东西“显示出来”
- `wait(ms)`：等一下（让动画有时间动起来）
- `alive()`：窗口还在不在（如果被你关掉了就停止）

### 1.1 最小可运行示例（完整 `main()`，直接复制就能跑）

把你的 `test.cpp`（或新建的 `main.cpp`）改成下面这样：

```cpp
#define NOMINMAX
#include <Windows.h>

#include "kids.hpp"
using namespace evgdi::kids;

int main()
{
  open(800, 520, L"我的第一个画板");
  clear(RGB(255, 255, 255));

  pensize(3);
  speed(0); // 0 = 最快（不延迟）

  // 1) 画一个彩色小螺旋（边画边显示）
  for (int i = 0; i < 240 && alive(); ++i) {
    pencolor(RGB((i * 7) % 256, (i * 3) % 256, (i * 11) % 256));
    forward(3 + i / 10);
    right(12);
    present();
    wait(16); // 约 60 帧/秒
  }

  // 2) 现在试试特效：真透视（像 3D 翻卡片）+ 鱼眼
  effects_on();          // 开启 PixelCanvas 特效模式（默认是关的）
  perspective_on(0.0022f);
  fisheye_on(0.75f, 220.0f);

  for (int t = 0; t < 360 && alive(); ++t) {
    rotate_x(18.0f * sinf(t * 0.03f));
    rotate_y(25.0f * cosf(t * 0.025f));
    rotate_z(t * 0.5f);

    // 鱼眼中心在画板里来回跑
    fisheye_at(400.0f + sinf(t * 0.04f) * 120.0f, 260.0f + cosf(t * 0.03f) * 80.0f);

    present();
    wait(16);
  }

  close();
  return 0;
}
```

### 1.2 `kids.hpp` 的函数说明（给小朋友看的）

#### A）窗口与时间
- `open(w, h, title)`：打开一个画板窗口（宽 `w`、高 `h`、标题 `title`）。
- `close()`：关掉画板窗口。
- `alive()`：窗口还活着吗？（你点右上角 X 以后会变成 `false`）
- `wait(ms)`：等 `ms` 毫秒（常用 `16`，表示“一小会儿”）
- `clear(color)`：把画板涂成一种颜色（默认白色）
- `present()`：把“当前画板内容”显示到屏幕上（做动画一定要不停调用它）

#### B）画笔（像海龟一样）
- `penup()` / `pendown()`：抬笔/落笔（抬笔移动不画线）
- `speed(ms)`：画笔速度（越小越快，`0` 最快）
- `pensize(w)`：线条粗细（像素）
- `pencolor(RGB(r,g,b))`：线条颜色
- `home()`：回到“家”（画板中心）
- `goto_xy(x, y)`：瞬移到坐标（x 向右变大，y 向下变大）
- `forward(d)` / `backward(d)`：向前/向后走 `d` 像素
- `left(deg)` / `right(deg)`：左转/右转 `deg` 度
- `circle(r)`：画一个圆（半径 `r`）
- `polygon(sides, len)`：画正多边形（`sides` 边数，`len` 每条边长度）

#### C）特效（PixelCanvas：默认使用“状态机制”，你也可以关掉）
> 小朋友可以先不管原理：你只要知道这些会把整个画板“变形”。

- `effects_on()`：开启特效模式（之后 `present()` 会显示特效后的画面）
- `effects_off()`：关闭特效模式（`present()` 直接显示原画面）
- `effects_reset()`：把特效状态清零（回到“没变形”的样子）
- `fast(true/false)`：快速模式（更快但可能更糊）
- `state(true/false)`：状态机制开关（默认是开；关掉就需要你自己每次把参数传全）
- `perspective_on(strength)` / `perspective_off()`：真透视投影开/关（`strength` 越大越“3D”）
- `rotate_x(deg)` / `rotate_y(deg)` / `rotate_z(deg)`：绕 X/Y/Z 轴旋转（单位：度）
- `move(dx, dy)`：平移（单位：像素）
- `zoom(s)`：缩放（`1.0` 不变，`2.0` 变大一倍，`0.5` 变小一半）
- `push_z(dz)`：往镜头前后推（配合透视更明显）
- `fisheye_on(strength, radius)` / `fisheye_off()`：鱼眼开/关
- `fisheye_at(cx, cy, strength, radius)`：指定鱼眼中心（更好玩）

#### D）弹窗波浪（不用懂 hook）
> 你只要把它当成“会动的消息框”。

- `wavebox(text, caption, durationMs, ...)`：一键播放波浪弹窗（最简单）
- `wavebox_circle(text, durationMs, ...)`：沿圆形轨迹跑（很酷）
- `wavebox_rect(text, durationMs, ...)`：沿矩形轨迹跑（像巡逻队）

最短示例（完整 `main()`）：
```cpp
#include "kids.hpp"
using namespace evgdi::kids;

int main() {
  wavebox_circle(L"我在绕圈跑！", 4000);
  wavebox_rect(L"我在绕边巡逻！", 4000);
}
```

---

## 2）进阶模式（想当“魔法师”再看）

如果你想自己控制更多细节，可以直接用这些头文件（每个文件里都有中文注释）：

- `draw.hpp`：`Pen` 画笔（画线、画圆、画多边形、画文字点阵等）
- `PixelCanvas.hpp`：像素画布（旋转/缩放/平移/真透视/鱼眼，默认“状态机制”）
- `LayeredTextout.hpp`：分层文字特效（波浪、鱼眼、3D 变化等）
- `LayeredWindowGdi.hpp`：分层窗口/位图呈现
- `MessageBoxWave.hpp` / `MessageBoxWindow.hpp`：弹窗特效
- `BorderedWindowGDI.hpp`：带边框的窗口演示

仓库里自带全功能演示入口：`test.cpp`（会依次演示多种效果）。

---

## 3）常见问题（看这里就不慌）

1. **窗口不动/看不到动画？**  
   做动画时，循环里要反复调用：`present(); wait(16);`

2. **看到中文乱码？**  
   - 如果是 VS 里：把文件“另存为 UTF-8（带签名/BOM）”再打开；  
   - 如果是命令行里：PowerShell 有时会把 UTF-8 当成 ANSI 来显示，这是显示问题，不影响编译。

3. **特效太卡？**  
   先试试：`fast(true);`，或者把窗口开小一点（比如 640×360）。
