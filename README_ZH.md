# ZenWare.cc — 中文

**Left 4 Dead 2 训练用内外挂**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

> **仅源代码 · 仅学习与本地服务器。**
> 本仓库与 Releases 中没有预编译的 `.exe` / `.dll` —— 请从源码构建，
> 使用 `-insecure` 启动游戏，在自己的地图上玩（`map c1m1_hotel`）。
> VAC 服务器 = 封号风险，风险自负。

## 关于

基于 [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)。三部分，一个解决方案（`ZenWare.sln`，`Release | Win32`）：

| 部分 | 输出 |
|---|---|
| `ZenWare.DLL` — 内部模块 | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — 图形加载器 | 本地 `dist/ZenWare.exe`，从不发布，有版本页面按钮 |
| `ZenWare.External` — 只读外部覆盖层 | `bin/Release/ZenWare.External.exe` |

## 功能

<details open>
<summary><b>视觉</b> — ESP · 透视上色 · 覆盖层</summary>

- **ESP** —— 方框、血条、名字、距离、武器/物品、小怪、全部特感（含 Boomer，对国外构建用类名回退）
- **透视上色** —— 5 种配色，队友 / 敌人 / 全部特感，透视穿墙
- **世界** —— 去雾，世界 + 武器 FOV，第三人称，自定义准星，FPS 覆盖层
- **情报** —— 2D 雷达，旁观者列表，Tank / Witch 警报，击杀播报，命中标记（音效 + 伤害数字）
- **团队** —— 被控警告，救援提醒，Tank 血条，队伍血量面板，附近特感列表
- **反馈** —— 命中十字，单局统计（命中/射击/命中率），受伤闪屏，最小伤害过滤，已投掷手雷计时
- **投掷物** —— 弹道预览（燃烧瓶 / 土雷 / 胆汁 / 榴弹枪）带落点标记

</details>

<details>
<summary><b>战斗</b> — 自瞄辅助</summary>

- **自瞄** —— 静默，FOV / 距离优先，头 / 身体，平滑，仅可见目标，小怪 + 全部特感
- **按武器** —— 按组独立 FOV / 平滑 / hitbox（步枪、冲锋枪、霰弹枪、狙击枪、手枪）
- **辅助** —— 扳机、自动推、自动手枪、无散布（可开关）

</details>

<details>
<summary><b>移动</b> — 连跳与摆动</summary>

- **连跳套件** —— 完美 / 拟人连跳，EdgeJump，EdgeBug，JumpBug，LongJump，FastStop，Prestrafe，AutoDuck，JumpStats，SpeedHUD
- **摆动** —— 拟人 / rage / w-only / 定向自动摆动

</details>

<details>
<summary><b>菜单 / 系统</b></summary>

- 菜单 `INSERT` · 卸载 `F11` · 语言 EN/RU/DE/ES/PT/PL/FR/ZH（按钮 + `F7`）· 动态 RGB 图标 · `?` 帮助图标 · 标签页 · 滚轮
- 配置 —— 3 个槽位（`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`，`bool / int / float / Color`）
- 日志 —— `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log`（按 pid 回退）
- 偏移缓存 —— `<gamedir>/ZenWare.offsets`（自动，删除可强制重扫）

</details>

### 构建与运行 —— 仅本地

**1. 构建**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. 准备游戏** —— Steam → L4D2 → 启动选项：

```text
-insecure -windowed -console
```

**3. 加载本地图** —— 游戏控制台：

```text
map c1m1_hotel
```

**4. 注入** —— 以管理员身份运行本地构建的加载器 → `INJECT`。

仅 External（不注入）：运行本地
`ZenWare.External/bin/Release/ZenWare.External.exe` 或 `START-ZenWare.bat`。

> 不要把构建好的 `exe` 上传到公开 GitHub Releases。源代码政策：[SECURITY.md](SECURITY.md)。

### 环境要求

| 项目 | 版本 |
|---|---|
| 系统 | Windows 10/11 x64 |
| 游戏 | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| 目标 | `Release` · `Win32` (x86) |

### 按键

| 按键 | 动作 |
|:---:|---|
| `INSERT` | 打开 / 关闭菜单 |
| `F11` | 卸载 |
| `F7` | 语言 EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space`（按住） | 连跳（默认） |
| `MOUSE4`（按住） | 自瞄（默认） |

改键：`Misc → Menu key / Aimbot key` —— 点击 → `[press key]` → 按下按键，`ESC` = 关。

## 项目结构

```text
ZenWare.cc/
├── ZenWare.sln                  解决方案（DLL + Loader + External）
├── ZenWare.DLL/                 内部模块（+ MinHook、SDK、Features、Hooks）
├── ZenWare.Loader/              图形加载器 —— 仅本地构建，无自动更新
├── ZenWare.External/            外部覆盖层 —— ESP、Memory、Movement、Overlay
├── Tools/SigScan/               本地 client.dll 签名验证器
├── .github/workflows/           CI 编译检查 —— 不上传二进制
├── dist/                        仅本地输出（git 忽略）
├── Build-SingleFile.ps1         本地构建脚本（无 Publish）
├── Verify-Signatures.bat        一键特征码检查，对比你的游戏文件
├── START-ZenWare.bat            本地 external 启动器
├── README.md · README_DE.md · README_ES.md · README_PT.md · README_PL.md · README_FR.md · README_ZH.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

## 常见问题

| 现象 | 解决方法 |
|---|---|
| 启动时 `XorString` 弹窗 | 游戏更新后特征码失效 → 双击 `Verify-Signatures.bat`，更新 `Offsets.cpp` |
| `Paint` 后崩溃 | 打开日志 → 找到 `[!!!] EXCEPTION` → 上报 `模块+0x...`，不要 dumps |
| 杀软报警 | 文件夹 → 加白名单（对 `WriteProcessMemory` / `CreateRemoteThread` 的启发式） |
| 方块字代替字母 | 按 `F7`（字体回退 / 语言） |
| External 无方框 | 看覆盖层顶部几行（resolver 诊断）和特征码 |
