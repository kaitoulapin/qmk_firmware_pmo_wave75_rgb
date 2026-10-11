# PMO_WAVE75_RGB 官方固件移植到最新版仓库

本仓库把 PMO WAVE 75 RGB（`hm/wave75_rgb`）的官方固件移植到最新版 QMK。
厂商未提供源码的预编译库 `lib/rdr_lib/librdrcommon.a` 已随仓库提供，无需另外下载。

## 编译步骤

1. 拉取子模块（首次 clone 后执行一次）：

   ```bash
   git submodule update --init --recursive
   ```

2. **修复子模块里的宏定义**（上游未修，不改会编译失败）：

   ```bash
   # Git Bash / QMK MSYS
   ./scripts/fix-chibios-contrib.sh
   ```

   ```powershell
   # Windows PowerShell
   powershell -ExecutionPolicy Bypass -File .\scripts\fix-chibios-contrib.ps1
   ```

   原因：`lib/chibios-contrib/os/common/ext/CMSIS/ES32/FS026/system_fs026.h` 里
   `#ifndef __SYSTEM_FS026_H__` 对应的却是 `#define __SYSTEM_ES32F0283_H__`，
   宏名不一致，重复包含时会报错：

   <img width="2145" height="375" alt="image" src="https://github.com/user-attachments/assets/b1937c70-34d1-40c0-a3ed-90dfba25eee6" />

3. 编译 `via` 版本固件：

   ```bash
   make hm/wave75_rgb:via -j2
   # 或
   qmk compile -kb hm/wave75_rgb -km via
   ```

   产物在 `.build/hm_wave75_rgb_via.bin`（同时会复制一份到仓库根目录）。

## 说明

- Logo 灯的 `Speed (%)` 支持 0～100 连续调速，0 为暂停动画，数值越大越快。
  `LG_SPI` / `LG_SPD` 每次增减 5 个百分点。速度通过 VIA 保存后可跨重启保留；
  原有 Logo 灯效、亮度、颜色及状态指示继续由厂商库处理。

- 随机按键渐灭：刷入本仓库的 `via` 固件，导入仓库中的
  `keyboards/hm/wave75_rgb/WAVE 75 RGB.JSON`，在 Lighting → Backlight → Effect
  选择 `Random Reactive`（编号 47，随机按键渐灭）。按下的键随机亮起一种颜色，
  随后渐灭；松开不会重新触发。Brightness 控制亮度，Effect Speed 越高渐灭越快
  （约 3 秒至 0.3 秒）。Logo 和状态指示灯仍由原有逻辑控制。
  厂商旧 JSON 仅列出 0～45，无法选择新增效果。

- `lib/rdr_lib/librdrcommon.a` 是厂商预编译库（无法提供源码），入库以保证 clone 后可直接编译。
  `*.a` 默认被 `.gitignore` 忽略，仓库里已为该文件加了例外规则。
  SHA256：`CEA2276AE9D5B092F81536F7BCE68BBDCC37DA0C1B3D0F1D8F665A762294F94B`
- 用 VIA 改键/改灯需要刷 `via` 版本固件，然后在 `usevia.app`（Chrome/Edge）的 Design 标签里
  导入 `keyboards/hm/wave75_rgb/WAVE 75 RGB.JSON`，再切到 Configure 配置。



# Quantum Mechanical Keyboard Firmware

[![Current Version](https://img.shields.io/github/tag/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/tags)
[![Discord](https://img.shields.io/discord/440868230475677696.svg)](https://discord.gg/qmk)
[![Docs Status](https://img.shields.io/badge/docs-ready-orange.svg)](https://docs.qmk.fm)
[![GitHub contributors](https://img.shields.io/github/contributors/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/pulse/monthly)
[![GitHub forks](https://img.shields.io/github/forks/qmk/qmk_firmware.svg?style=social&label=Fork)](https://github.com/qmk/qmk_firmware/)

This is a keyboard firmware based on the [tmk\_keyboard firmware](https://github.com/tmk/tmk_keyboard) with some useful features for Atmel AVR and ARM controllers, and more specifically, the [OLKB product line](https://olkb.com), the [ErgoDox EZ](https://ergodox-ez.com) keyboard, and the Clueboard product line.

## Documentation

* [See the official documentation on docs.qmk.fm](https://docs.qmk.fm)

The docs are powered by [VitePress](https://vitepress.dev/). They are also viewable offline; see [Previewing the Documentation](https://docs.qmk.fm/#/contributing?id=previewing-the-documentation) for more details.

You can request changes by making a fork and opening a [pull request](https://github.com/qmk/qmk_firmware/pulls).

## Supported Keyboards

* [Planck](/keyboards/planck/)
* [Preonic](/keyboards/preonic/)
* [ErgoDox EZ](/keyboards/ergodox_ez/)
* [Clueboard](/keyboards/clueboard/)
* [Cluepad](/keyboards/clueboard/17/)
* [Atreus](/keyboards/atreus/)

The project also includes community support for [lots of other keyboards](/keyboards/).

## Maintainers

QMK is developed and maintained by Jack Humbert of OLKB with contributions from the community, and of course, [Hasu](https://github.com/tmk). The OLKB product firmwares are maintained by [Jack Humbert](https://github.com/jackhumbert), the Ergodox EZ by [ZSA Technology Labs](https://github.com/zsa), the Clueboard by [Zach White](https://github.com/skullydazed), and the Atreus by [Phil Hagelberg](https://github.com/technomancy).

## Official Website

[qmk.fm](https://qmk.fm) is the official website of QMK, where you can find links to this page, the documentation, and the keyboards supported by QMK.
