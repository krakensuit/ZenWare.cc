# ZenWare.cc — 中文

**Left 4 Dead 2 训练用内外挂**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8`

> **仅源代码 · 仅学习与本地服务器。**
> 本仓库与 Releases 中没有预编译的 `.exe` / `.dll` —— 请从源码构建，
> 使用 `-insecure` 启动游戏，在自己的地图上玩（`map c1m1_hotel`）。
> VAC 服务器 = 封号风险，风险自负。

🇬🇧 [English](README.md) · 🇷🇺 [Русский](README_RU.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇪🇸 [Español](README_ES.md) · 🇵🇹 [Português](README_PT.md) · 🇵🇱 [Polski](README_PL.md) · 🇫🇷 [Français](README_FR.md)

## 关于

ZenWare.cc 是 Left 4 Dead 2（Source 引擎，2009）的训练沙盒：渲染、预测、
移动与网络——在自己的电脑、自己的服务器上，带 `-insecure` 启动。不绕过
VAC，不碰官方服务器，不提供成品二进制。

代码源自 [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)，
已发展为一个解决方案里的三件工具（`ZenWare.sln`，`Release | Win32`）：

| 部分 | 模式 | 功能 | 产物 |
|---|---|---|---|
| `ZenWare.DLL` | 内部（注入） | 完整功能：边角 ESP、Chams、自瞄、身法、游戏内菜单 | `bin/Release/ZenWare.dll`（x86，/MT） |
| `ZenWare.Loader` | GUI 启动器，仅本地构建 | Manual-map / LoadLibrary 注入、主题、8 种语言 | 本地 `dist/ZenWare.exe`，从不发布 |
| `ZenWare.External` | 外部（不注入） | RPM 读取 + GDI 覆盖层 + `SendInput` 连跳/压枪 | `bin/Release/ZenWare.External.exe` |

新手？先玩 **External**——它从不写游戏内存，是学习整条管线最安全的方式。
详情：[SECURITY.md](SECURITY.md) · 第三方：[NOTICE.md](NOTICE.md) ·
许可证：[LICENSE](LICENSE) · 结构：[ARCHITECTURE.md](ARCHITECTURE.md)。

### 工作原理

**内部 DLL。**注入时 `DllMain` 起一个线程（逻辑绝不在 loader 锁上跑），
`Entry::Load` 按 fail-closed 顺序初始化：日志 → 崩溃记录器 → 等
`serverbrowser.dll` → 特征码扫描（带 RVA 缓存 `ZenWare.offsets`）→ 接口
（`VEngineCvar007` 先从 `vstdlib.dll` 拿）→ netvar → `MinHook` 挂钩 →
加载配置。缺特征码或接口 = 弹窗，而不是崩溃。

每帧跑两条链：

- **Tick**（`ClientMode::CreateMove`）——预测、移动（BunnyHop →
  AutoStrafe → JumpStats → AutoShove）、战斗（Aimbot → TriggerBot →
  AutoPistol → NoSpread）。引擎原函数每 tick 只调一次。
- **帧**（`EngineVGui::Paint`，仅 `PAINT_UIPANELS`）——第三人称 /
  fullbright / 隐藏手臂 cvar、killfeed 与 hitmarker 的 tick、ESP、雷达、
  告警、菜单、准星、手雷预览、覆盖层，以及 killfeed / hitmarker /
  JumpStats 的动画绘制。

设计上不用引擎事件——killfeed 与 hitmarker 靠轮询 HP/存活变化。所有绘制
只走 `MatSystemSurface` 上的 `G::Draw`；实体只用 netvar 读取，调虚函数前
必查 `ClassID`。

**启动器。**Win32 界面，带闪屏、跟随系统深色/浅色主题、8 种语言、动画
RGB 图标。DLL、External 与图标都以内嵌资源打包——只有一个输出文件。
Manual-map 注入（重定位、导入表、区段保护、擦头部），失败回退
`LoadLibrary`。无网络，无自动更新。

**外部端。**只读：20 Hz 的 `ReadProcessMemory` 快照、60 Hz 的穿透式 GDI
覆盖层、移动只用 `SendInput`。解析器（特征 + 锚点 + 校验）每次启动重新
定位地址。

### 功能

<details open>
<summary><b>视觉</b> —— ESP · Chams · 覆盖层</summary>

- **ESP**——边角框（按队伍着色）、带边框血条 + 可选血量文字、带阴影名字、
  距离、带弹匣子弹数的武器、物品、普通感染者、全部特感含 Boomer（类名回退）、
  可选 ESP 距离上限、淡化连线
- **Chams**——5 套配色，敌/友/Tank 独立颜色，穿墙开关，幽灵与 dormant 过滤
- **世界**——NoFog、Fullbright、世界/手臂 FOV 分开调、带距离第三人称、隐藏
  手臂、自定准星、FPS / 坐标 / 血量覆盖层、存活普感计数
- **情报**——带偏航旋转的 2D 雷达、观战列表、Tank / Witch 告警、轮询式
  killfeed 动画卡片、带音效的 hitmarker、伤害数字、命中十字与当局统计
- **团队**——被控告警（Smoker/Hunter）、救人告警、Tank 血条、队伍血量面板、
  附近特感独立面板、口水告警、已扔手雷计时（土雷 / 火瓶火焰）
- **反馈**——红色受伤闪屏、指向最后攻击者的箭头、最小伤害过滤、武器 HUD
  （弹匣 + 备弹）、黄色 `RELOADING` 与红色低弹量警告
- **投掷物**——弹道预览（火瓶 / 土雷 / 胆汁 / 榴弹发射器），带弹跳模拟与
  落点标记

</details>

<details>
<summary><b>战斗</b> —— 瞄准辅助</summary>

- **自瞄**——静默、自瞄 FOV / 距离优先、爆头 / 身体中心、平滑、仅可见、按键、
  普感 + 全部特感 + Tank
- **按武器**——步枪、冲锋枪、霰弹枪、狙击、手枪分组，各自 FOV / 平滑 / 命中点
- **帮手**——TriggerBot（沿瞄准后射线开火，无 tick 延迟）、AutoShove（推开
  被控队友）、AutoPistol、NoSpread（11 把枪白名单，默认开）

</details>

<details>
<summary><b>身法</b> —— 连跳与压枪</summary>

- **连跳包**——Perfect / Legit BunnyHop、EdgeJump、EdgeBug、JumpBug、
  LongJump 辅助、FastStop、Prestrafe、AutoDuck、起跳延迟可调
- **压枪**——Legit / Rage / W-only / Directional AutoStrafe
- **JumpStats**——KZ 面板：距离、预加速、最高速、坠落、跳高、滞空、空中实时
  sync%、压枪数、当局 JB / EB 计数

</details>

<details>
<summary><b>菜单 / 系统</b></summary>

- 游戏内菜单（`INSERT`）：5 个页（Visuals / Move / View / Combat / Misc），
  每页滚动、RGB 图标、`?` 帮助（EN + RU）、色板、按键绑定、滑杆、一键隐藏
  ESP 的 panic 键
- 8 种语言实时切换（按钮或 `F7`）：EN / RU / DE / ES / PT / PL / FR / ZH
- 配置——3 个槽（`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`），
  `bool / int / float / Color`，浮点滑杆用 int 代理并回同步
- 日志——`%TEMP%/ZenWare.log` 会搬到 `<gamedir>/ZenWare.log`（按 PID 回退），
  崩溃记录为 `module+offset` 加面包屑
- 偏移缓存——`<gamedir>/ZenWare.offsets`（自动，删掉强制重扫）；
  `Tools/SigScan` 可离线校验 15 个特征码

</details>

### 构建与开玩 —— 仅本地

**1. 构建**（Visual Studio 2022 + `使用 C++ 的桌面开发`）：

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. 游戏**——Steam → L4D2 → 启动选项：

```text
-insecure -windowed -console
```

**3. 地图**——游戏内控制台：

```text
map c1m1_hotel
```

**4. 注入**——以**管理员**运行自己构建的启动器 → 点 `INJECT`。

纯外部（不注入）：运行本地
`ZenWare.External/bin/Release/ZenWare.External.exe` 或 `START-ZenWare.bat`。

> 不要把构建好的 `exe` 传到公开 Releases。源码政策：[SECURITY.md](SECURITY.md)。

### 环境要求

| 项目 | 版本 |
|---|---|
| 系统 | Windows 10/11 x64 |
| 游戏 | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `使用 C++ 的桌面开发` |
| SDK | Windows SDK `10.0.26100.0` |
| 目标 | `Release` · `Win32`（x86） |

### 按键

| 按键 | 动作 |
|:---:|---|
| `INSERT` | 打开 / 关闭菜单 |
| `F11` | 卸载 |
| `F7` | 语言 EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space`（按住） | 连跳（默认） |
| `MOUSE4`（按住） | 自瞄（默认） |

改键：`Misc → Menu key / Aimbot key`——点击 → `[press key]` → 按下按键，`ESC` = 关。
Panic 键立即隐藏所有框（`Visuals → ESP`）。

### 配置与日志

- 槽位在游戏 DLL 路径旁：`ZenWare.cfg`、`ZenWare2.cfg`、
  `ZenWare3.cfg`——纯 `key=value`，117 个键。当局数值（JB/EB 计数）从不保存。
- 日志：`<gamedir>/left4dead2/ZenWare.log`。崩溃长这样：
  `[!!!] EXCEPTION code=... at module+0x...` + 最后一条面包屑——有这一对就够，
  不需要 dump。
- 特征缓存：`<gamedir>/left4dead2/ZenWare.offsets`（RVA + 模块指纹）；游戏
  更新后删掉，或跑 `Verify-Signatures.bat`，报 `NOT FOUND` 就修 `Offsets.cpp`。

### 项目结构

```text
ZenWare.cc/
├── ZenWare.sln                  解决方案（DLL + Loader + External）
├── ZenWare.DLL/                 内部模块
│   ├── src/Entry/               初始化顺序、崩溃记录、卸载
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 L4D2 接口、实体、DrawManager、GameUtil
│   ├── src/Features/            Vars（设置）+ Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger、Pattern、Offsets 缓存、NetVars、MinHook
├── ZenWare.Loader/              GUI 启动器 —— 仅本地构建，无自动更新
├── ZenWare.External/            外部覆盖层 —— ESP、Memory、Movement、Overlay
├── Tools/SigScan/               校验你 client.dll 特征码的工具
├── .github/workflows/           CI 编译检查 —— 不上传二进制
├── dist/                        仅本地输出（git-ignored）
├── Build-SingleFile.ps1         本地构建脚本（无 Publish）
├── Verify-Signatures.bat        一键校验特征码
├── START-ZenWare.bat            本地启动外部端
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   本文件的译文
```

### 截图

说明：[docs/screenshots/HOWTO.md](docs/screenshots/HOWTO.md)。
`menu.png`（INSERT 菜单）· `esp.png`（ESP + 雷达打尸潮）· `loader.png`（启动器窗口）。

### 排错

| 现象 | 办法 |
|---|---|
| 启动报 `XorString` / `FATAL ERROR` | 游戏更新后特征失效 → 跑 `Verify-Signatures.bat`，修 `Offsets.cpp` |
| 游戏内崩溃 | 打开日志 → 找 `[!!!] EXCEPTION` → 报 `module+0x...` + 最后面包屑，不用 dump |
| 杀毒报警 | 文件夹 → 加白名单（`WriteProcessMemory` / `CreateRemoteThread` 启发式） |
| 方块代替文字 | 按 `F7`（字体回退 / 语言） |
| 外部端没框 | 看覆盖层顶部几行（解析器诊断）与特征码 |
| 菜单开着但点击进了游戏 | 先关游戏自己的菜单（本作只在自家菜单开时吞 `ESC`） |

## 法律

| 主题 | 条款 |
|---|---|
| 许可证 | MIT——[LICENSE](LICENSE)，`AS IS`，无担保 |
| 第三方 | [NOTICE.md](NOTICE.md)：MinHook、FontAwesome、Valve SDK 头、基址 |
| 商标 | Left 4 Dead 2 / Steam 归 Valve，无关联、无背书 |
| 分发 | 仅源码——[SECURITY.md](SECURITY.md)，无二进制 |
| 使用 | 仅本地 `-insecure`。不绕 VAC，封号自负 |

<div align="center">

**致谢** ·
基址：[Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Hook 引擎：MinHook（Tsuda Kageyu，BSD 风格）·
图标：[FontAwesome 6 Free](https://fontawesome.com)

</div>
