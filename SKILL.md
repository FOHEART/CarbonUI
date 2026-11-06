---
name: cli-anything-carbonui
description: 控制 CarbonUI 样式库的命令行接口——离线渲染控件截图、导出 QSS、报告版本与渲染环境
---

# CarbonUI CLI Skill（carbonui_cli）

CarbonUI 是纯 Qt Widgets 样式库（暗色主题 + 可折叠分组框）。本 CLI 把它的核心能力
暴露成可自动化调用的命令：**渲染控件截图**（给 Agent/人看 UI 长什么样）、**导出
QSS**（拿到真实样式文本）、**报告渲染环境**（版本/Qt/QPA/字体族）。

## 调用方式

### 方式一：直调 carbonui_cli

```text
carbonui_cli [--json] <command> [options]
```

无参数启动进入 REPL；`exit`/`quit`/EOF 退出，`json` 翻转输出模式。

### 方式二：Python harness（推荐 Agent 使用）

```bash
pip install -e agent-harness          # 或直接用仓库内 agent-harness/
carbonui-agent info
carbonui-agent snapshot --widget button --shape rect --out shot.png
```

harness 的二进制定位顺序：环境变量 `CARBONUI_CLI_BIN` > 部署目录（与
`agent-harness/` 同级）> 仓库构建产物 `out/build/tdd-tests/cli/` > PATH。

## 命令表

| 命令 | 说明 | 关键参数 | 支持 --json |
| --- | --- | --- | --- |
| `info` | 库版本、Qt 版本、QPA、默认字体族、候选字体列表 | 无 | 是 |
| `snapshot` | 渲染单个控件到 PNG | `--widget`（必填，见下）、`--shape`、`--state`、`--text`、`--width/--height`（px）、`--out` | 是 |
| `gallery` | 渲染整套控件总览到 PNG | `--out` | 是 |
| `qss` | 导出样式函数写入控件的 QSS 文本 | `--widget` 或 `--all`、`--shape`（仅 button/radio） | 是 |
| `help` / `version` | 帮助 / 同 info | 无 | 是 |

`--widget` 取值：`button | radio | checkbox | lineedit | tab | combo | tree | statusbar | groupbox`

`--shape` 取值：`round | left | right | rect`（分段控件的最左/中间/最右格）

`--state` 取值：`normal | disabled | checked | collapsed`；与控件对应关系：
`disabled`（button/radio/lineedit/combo/checkbox）、`checked`（radio/checkbox）、
`collapsed`（groupbox 折叠态，缺省展开）。

`--width/--height` 覆盖控件尺寸（像素）；groupbox 高度由折叠控制器决定，忽略覆写。

## 输出信封（所有命令统一）

```json
{"ok": true, "code": 0, "message": "…", "data": {}}
```

- 自动化一律加 `--json`，只看 `ok`/`code`，不要解析人类可读文本；
- `data` 永远存在；失败时为空对象；
- **进程退出码只分 0（成功）/ 1（失败）**，细分错误看信封 `code`。

### data 字段 schema

| 命令 | data 字段 |
| --- | --- |
| `info` | `version`（如 "0.2.1"）、`versionTag`（"v0.2.1"）、`qtVersion`、`platform`/`qpa`、`defaultFontFamily`、`fontFamilies`（不超过 5 个候选）、`fontFamilyCount` |
| `snapshot` | `widget`、`shape`（空串=该类型无外形概念；button/radio 缺省回填 "round"）、`state`、`out`（按输入原样）、`width`/`height`（宿主像素，含 8px 底板边距）、`bytes` |
| `gallery` | `out`、`width`、`height`、`bytes` |
| `qss` | `widget`、`qss`（多行文本）；`--all` 时为 `{ "<widget>": "<qss>", … }` |

## 错误码表（信封 code；均实测核对）

| code | 含义 | 实测 message 样例 |
| --- | --- | --- |
| 0 | 成功 | `ok` |
| 1 | 参数/用法错误（改命令重试，不要原样重跑） | `未知命令 bogus（可用：info snapshot gallery qss help version）` |
| 1 | 同上 | `未知的 --widget 取值 nope（允许 button\|radio\|…\|groupbox）` |
| 1 | 同上 | `缺少 --widget（或用 --all 导出全部）`（qss 未给参数时） |
| 2 | 输出文件写不进去 | `无法写入 X:\no\such\dir\a.png（目录不存在或无权限）` |
| 3 | 渲染失败（QPA 环境异常） | `渲染失败：button 抓到的图像为空` |

## 示例（输出均实测）

### 1. 查询渲染环境

```text
$ carbonui_cli --json info
{"code":0,"data":{"defaultFontFamily":"HarmonyOS Sans SC","fontFamilies":["Arial","Arial Black","Bahnschrift","Bahnschrift Condensed","Bahnschrift Light"],"fontFamilyCount":177,"platform":"windows","qpa":"windows","qtVersion":"5.15.2","version":"0.2.1","versionTag":"v0.2.1"},"message":"CarbonUI 0.2.1 on Qt 5.15.2 (windows)","ok":true}
```

（`defaultFontFamily` 是内置字体注入后的结果；`fontFamilies` 是系统字体库前 5 个候选。）

### 2. 渲染一个禁用态按钮

```text
$ carbonui_cli snapshot --widget button --state disabled --text Cancel --out shot.png
ok (code=0): ok
{ "bytes": 1559, "height": 52, "out": "shot.png", "shape": "round", "state": "disabled", "widget": "button", "width": 136 }
```

### 3. 导出全部控件 QSS

```text
$ carbonui_cli --json qss --all
{"code":0,"data":{"button":"QPushButton { background-color: #009ee8; … }","checkbox":"QCheckBox { … }"},"message":"导出 9 个控件的 QSS","ok":true}
```

### 4. 渲染整套控件总览

```text
$ carbonui_cli gallery --out preview.png
ok (code=0): ok
{ "bytes": 7916, "height": 260, "out": "preview.png", "width": 380 }
```

## 部署目录布局

```text
<install>/
├── carbonui_cli(.exe)     # scripts/deploy.bat 汇总产物
├── agent-harness/         # Python harness（pip install -e agent-harness）
├── SKILL.md               # 本文件（唯一来源，harness 内不放副本）
├── include/               # 库头文件
└── lib/                   # 库产物
```

## 注意事项

- **单位**：`--width/--height` 与信封里的 `width/height/bytes` 全部是像素/字节，无其他单位。
- **渲染环境**：QPA 缺省 Win32 用 `windows`、其余平台用 `offscreen`（offscreen 无字体
  数据库，整图画不出文字——实测踩过）；仓库内构建时自动加载内置字体
  HarmonyOS Sans SC，保证跨机渲染一致。可用 `QT_QPA_PLATFORM` 环境变量覆盖。
- **控件不弹窗**：抓图走 `QWidget::grab()`，全程无窗口，无头 CI 可跑。
- **QSS 的真实来源**：`qss` 命令是"真的建控件并调用样式函数后读回 styleSheet()"，
  输出与库行为一致，不会出现文档与代码漂移。
- **目录必须存在**：`--out` 不会自动建目录，父目录不存在返回 code 2。
