# EvilockGDI：给小朋友的 Windows 画板特效库（GDI）

这是一个在 Windows 上用 **GDI（画图工具）** 做特效的小库。你可以把它当成“会动的画笔/画板”，像学海龟绘图一样，几行代码就能画出图形、文字特效、甚至把屏幕/窗口像素拿来做“花屏”特效。

> 温馨提示：有些特效会闪烁、会弹窗、会在屏幕上动来动去。请在家长/老师陪同下使用，不要在上课或重要场合乱跑它。

---

## 你能做什么（功能总览）

### 1）画笔 `Pen`（像海龟一样画图）
文件：`draw.hpp`

你可以像这样控制画笔：
- `penup()` / `pendown()`：抬笔/落笔（抬笔移动不画线）
- `forward(n)` / `backward(n)`：前进/后退
- `left(deg)` / `right(deg)`：左转/右转
- `goto_xy(x, y)`：瞬移到某个位置
- `home()`：回到“家”（默认窗口中心）
- `drawCircle(r)`：画圆
- `drawArc(r, deg, CW/CCW)`：画圆弧
- `drawPolygon(sides, len)`：画多边形
- `drawTextWithIcons(...)`：用小图标拼出文字（支持中文）
- `beginFill(...)` / `endFill()`：把封闭图形用图标“填满”

**小知识：坐标系**
- 画板左上角是 `(0,0)`
- 往右是 x 变大，往下是 y 变大
- 角度 0 度默认指向右边

---

### 2）像素画板 `PixelCanvas`（抓一帧→改像素→贴回去）
文件：`PixelCanvas.hpp`

这个功能就是把“屏幕/窗口/任意 DC”当成一张图片：
1. `Capture()` 把画面抓到内存里
2. 你直接修改 `pixels()` 里的像素（就像改图片的每个点）
3. `Present()` 再贴回去

它默认用屏幕 DC（所以不传参数也能玩），也可以传入窗口 DC，或者用 `PixelCanvas::FromWindow(hwnd)` 只对某个窗口做特效。

---

### 3）窗口画板（带边框 / 分层透明窗口）
文件：`BorderedWindowGDI.hpp`、`LayeredWindowGdi.hpp`

你可以创建一个窗口，然后让窗口：
- 在屏幕上移动、反弹
- 抖动（Shake）
- 旋转（Rotate / turnLeft / turnRight）
- 亮度/对比度/饱和度调整（部分函数）
- 加载图片并绘制到窗口上

**带边框窗口**：像普通程序窗口  
**分层窗口**：可以透明、置顶，适合做“悬浮特效”

---

### 4）分层文字特效 `LayeredTextOut`（彩虹、波浪、鱼眼、漩涡、像素化…）
文件：`LayeredTextout.hpp`

它会创建一个透明置顶的小窗口，显示炫酷文字：
- 彩虹渐变 / 两色渐变 / 纯色
- 波浪扭曲（Wave）
- 鱼眼（FishEye）
- 漩涡（Twirl）
- 像素化（Pixelate）
- 灰度（Grayscale）/ 反色（Invert）
- 对比度/亮度
- 宽扁拉伸 + 3D 旋转（更像立体标题）

你可以用 `SetText()` 改文字，用 `Show()/Hide()/Destroy()` 控制显示。

---

### 5）弹窗波浪 `WaveEffect`（消息框排队像波浪一样动）
文件：`MessageBoxWave.hpp`、`MessageBoxWindow.hpp`

它会创建一串系统消息框，让它们沿着轨迹移动，像“波浪队列”。
- `WaveEffect::CreateWaveEffect(...)`：开始
- `WaveEffect::StopWaveEffect()`：停止并清理

> 注意：它会弹很多窗口，玩完要记得停止。

---

## 我该怎么运行（小白步骤）

### 用 Visual Studio（推荐）
1. 用 VS2022 打开 `EvilockGDI.vcxproj`
2. 选择配置：`Debug` + `x64`
3. 生成并运行

项目里有一个“演示/冒烟测试”入口：`test.cpp`  
它会依次演示：画笔、像素特效、窗口移动旋转、分层文字、弹窗波浪等。

---

## 3 个最简单的例子（抄了就能跑）

### 例子 A：用 `Pen` 画图
```cpp
// 在 WM_PAINT 的 HDC 里画
Pen pen(hdc, LoadIcon(nullptr, IDI_WARNING));
pen.speed(0);
pen.clearCanvas();

pen.penup();
pen.goto_xy(300, 200);
pen.pendown();
pen.drawCircle(60);
```

### 例子 B：用 `PixelCanvas` 改像素
```cpp
evgdi::PixelCanvas canvas; // 默认：屏幕
canvas.Capture();

auto* px = canvas.pixels();
for (int y = 0; y < canvas.height(); ++y) {
  for (int x = 0; x < canvas.width(); ++x) {
    px[y * canvas.width() + x].rgb ^= (x * y); // 简单异或特效
  }
}

canvas.Present();
```

### 例子 C：分层文字特效
```cpp
LayeredTextOut t;
t.Create(720, 220);
t.SetText(L"你好，我是特效文字！");
t.SetFontSize(44);
t.SetRainbowMode(true);
t.EnableWave(true, 14.0f, 8.0f);
t.Show();
```

---

## 小朋友常见问题（FAQ）

### Q1：为什么“抬笔”了还是会有点？
有些模式是“用图标一步一步盖上去”，就算抬笔，你如果调用了“画图函数”，它还是会画。抬笔只影响“移动时是否画线”。

### Q2：为什么会卡？
因为像素处理相当于“对每个像素点做数学题”。画面越大、效果越多，就越费电脑。你可以：
- 把窗口大小调小
- 关掉不需要的滤镜（比如像素化 + 颜色处理同时开最吃性能）

### Q3：我不想影响整个桌面怎么办？
优先对“窗口”操作：
- `PixelCanvas::FromWindow(hwnd)` 只处理某个窗口
- `ScreenGDI::FromWindow(hwnd)`（旧接口）也是同样思路

---

## 安全提示（一定要看）
- 不要在别人电脑上乱跑“花屏/弹窗”类效果
- 不要长时间全屏闪烁（眼睛会不舒服）
- 如果出现很多弹窗，优先按 `Alt + F4` 关闭，或在代码里 `StopWaveEffect()`

---

## 文件索引（你会经常打开的）
- `draw.hpp`：画笔 `Pen`
- `PixelCanvas.hpp`：像素画板（默认屏幕，也可传 DC）
- `LayeredTextout.hpp`：分层文字特效
- `LayeredWindowGdi.hpp`：分层窗口画板
- `BorderedWindowGDI.hpp`：带边框窗口画板
- `MessageBoxWave.hpp` / `MessageBoxWindow.hpp`：弹窗波浪
- `test.cpp`：演示/测试入口

