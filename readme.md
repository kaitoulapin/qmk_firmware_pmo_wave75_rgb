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

- Lighting → `Indicators` 控制状态提示的亮度。默认主灯区提示跟随主灯亮度，
  Logo 区提示跟随 Logo 有效亮度（包括 `Follow Backlight Brightness`）。
  `Allow Indicators Off` 默认关闭，保留最低 5% 亮度；打开后允许亮度为零。
  `Custom Indicator Brightness` 默认关闭，打开后显示统一控制两个灯区的百分比滑块，
  默认值 20%。100% 对应 RGB 输出 160，5% 对应 8；自定义值同样受最低亮度限制。
  闪烁的熄灭阶段仍为黑色，关闭自定义后恢复灯区跟随，保存后跨重启保留。
  覆盖 FN 连接、配对／重连、Win 锁、Caps、低电量、操作反馈、休眠设置及电量／充电提示。
  正常动画和工厂测试保留原行为。需同时刷入新版固件并导入新版 JSON。
  固件用链接包装识别厂商库中已核对的状态提示调用位置；构建会验证库 SHA256，
  替换库版本时必须重新核对调用位置。全局系统计时问题不在此补丁中修复。

- Logo 灯的 `Speed (%)` 支持 0～100 连续调速，0 为暂停动画，数值越大越快。
  `LG_SPI` / `LG_SPD` 每次增减 5 个百分点。速度通过 VIA 保存后可跨重启保留；
  原有 Logo 灯效、亮度、颜色及状态指示继续由厂商库处理。
- Logo 菜单中的 `Follow Backlight Speed` 和 `Follow Backlight Brightness`
  可分别启用速度和亮度跟随主灯。速度将主灯的 0～255 映射到 Logo 的 0～100%，
  亮度使用主灯的亮度设定值；不同灯效的实际周期和瞬时亮度仍由各自动画决定。
  跟随时隐藏对应的手动滑块，关闭跟随后恢复 Logo 原有手动参数。
  两个开关均默认关闭，并与 Logo 速度一起通过 VIA 保存。

- 随机按键渐灭：刷入本仓库的 `via` 固件，导入仓库中的
  `keyboards/hm/wave75_rgb/WAVE 75 RGB.JSON`，在 Lighting → Backlight → Effect
  选择 `Random Reactive`（编号 47，随机按键渐灭）。按下的键随机亮起一种颜色，
  随后渐灭；松开不会重新触发。Brightness 控制亮度，Effect Speed 越高渐灭越快
  （目标时长 1 秒至 0.3 秒，已在该灯效内部补偿下述计时偏差，用户试用确认观感可接受，精确时长尚未测量）。Logo 和状态指示灯仍由原有逻辑控制。
  厂商旧 JSON 仅列出 0～45，无法选择新增效果。
  渐灭固定每次按下生成的随机色，以 16 位精度计算平方衰减包络，最后缩放 RGB 通道并四舍五入，
  减少中间量化并缩短微弱亮度尾段。使用 QMK 帧时间，未点亮的 LED 跳过颜色计算，松开不会重新触发。
  根据 HAL 记录的 AHB 时钟和当前 SysTick 重装值换算该灯效的持续时间，不改变全局系统计时。
  RGB 刷新间隔下限设为 5 个系统 tick，按当前配置约 7.5ms；实际帧率仍取决于驱动和运行负载。

- `lib/rdr_lib/librdrcommon.a` 是厂商预编译库（无法提供源码），入库以保证 clone 后可直接编译。
  `*.a` 默认被 `.gitignore` 忽略，仓库里已为该文件加了例外规则。
  SHA256：`CEA2276AE9D5B092F81536F7BCE68BBDCC37DA0C1B3D0F1D8F665A762294F94B`
- 用 VIA 改键/改灯需要刷 `via` 版本固件，然后在 `usevia.app`（Chrome/Edge）的 Design 标签里
  导入 `keyboards/hm/wave75_rgb/WAVE 75 RGB.JSON`，再切到 Configure 配置。



## 开发验证

配置迁移、最低亮度和颜色缩放使用 QMK 原有 GoogleTest 框架：

```bash
make test:wave75_lighting
```

固件编译后，可运行维护检查，核对闭源库调用位置、工厂测试例外及 VIA 定义：

```bash
python3 scripts/verify-wave75-indicators.py \
  --objdump arm-none-eabi-objdump \
  --firmware .build/hm_wave75_rgb_via.elf
```

`scripts/check-wave75-indicator-library.py` 由键盘构建自动调用，校验固定库版本。
这两项 Python 工具使用标准库，不需要额外安装 Python 包。宿主测试不链接 ARM 厂商库。
上述检查不能替代键盘上的连接、充电、低电量和灯光观感验证。

## 已知问题：FS026 系统计时与时钟配置不匹配

记录日期：2026-10-11。状态：已核对源码及固件、厂商库的反汇编，全局计时尚未修复，未在硬件上测量系统 tick。
后续 Random Reactive 测试版本已局部换算持续时间，并把刷新间隔改为 5 tick；以下现象和表格记录补偿前的版本。

现象：Random Reactive 的代码设定最慢 1000ms、最快 300ms，用户实测最慢约 2 秒、最快约 0.5 秒。
不能把代码中的毫秒值直接当作已验证的真实时长，也不能仅靠缩短灯效参数掩盖计时误差。

已确认的静态证据：

- `keyboards/hm/wave75_rgb/chconf.h` 设置 `CH_CFG_ST_FREQUENCY = 1000`，QMK 按每个系统 tick 为 1ms 换算。
- `lib/chibios-contrib/os/hal/ports/ES32/FS026/hal_lld.h` 默认选择 HRC48（48MHz），AHB 不分频，PLL 未启用。
  当前 ELF 的 `rcu_initStruct` 数据与此一致；其中存在 72MHz PLL 参数不代表实际启用了 PLL。
- `lib/chibios-contrib/os/hal/ports/ES32/LLD/TIMv1/hal_st_lld.c` 的 `st_lld_init()`
  写死 `SysTick->LOAD = 72000 - 1`，`CTRL = 0x10007`。当前 ELF 反汇编确认写入重装值 `0x1193F`（71999）。
- `platforms/chibios/timer.c` 的 `timer_read32()` 根据配置频率换算系统 tick；当前 ELF 没有补偿上述时钟差异。
- 厂商 `librdrcommon.a` 的 `User_Systime_Init` 配置独立硬件定时器，不修改 SysTick 重装值。
  检查到的休眠／唤醒路径会关闭、恢复 SysTick 中断，唤醒后选择 HRC48，未修改其重装值。

按 48MHz 工作时钟推算，每个 tick 实际为 `72000 / 48000000 = 1.5ms`：

| 代码设定 | 按时钟配置推算的真实时间 |
| --- | --- |
| 渐灭 1000ms | 约 1.5 秒 |
| 渐灭 300ms | 约 0.45 秒 |
| RGB 刷新间隔下限 8ms | 约 12ms |

上述倍率能解释时长偏长及最快档约 0.5 秒，但不能完全解释最慢档约 2 秒，
也尚未证明它是灯光卡顿的唯一原因。实际工作时钟、tick 倍率和灯光提交延迟仍需硬件验证。

后续修复方向：根据实际系统／AHB 时钟及 `CH_CFG_ST_FREQUENCY` 计算 SysTick 重装值，
并验证冷启动、休眠唤醒后的计时一致性。修改会影响全局定时，需回归灯效、Logo、休眠和 USB／无线连接行为。
暂未修改子模块中的计时实现；原有头文件宏修复是另一项独立问题。

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
