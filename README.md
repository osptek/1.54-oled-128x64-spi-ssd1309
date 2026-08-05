<p align="left"><img alt="OSPTEK" src="./images/logo.png" width="200" /></p>

<h1 align="center">OSPTEK 1.54″ OLED 128×64（SSD1309 · SPI）</h1>

<p align="center"><b>单色 OLED 模组 · SPI · SSD1309</b></p>

<p align="center"><a href="./README_EN.md">English</a> | 简体中文</p>

<p align="center">
  <img alt="Size: 1.54 inch" src="https://img.shields.io/badge/Size-1.54%22-3498DB?style=flat-square" />
  <img alt="Resolution: 128x64" src="https://img.shields.io/badge/Resolution-128%C3%9764-8E44AD?style=flat-square" />
  <img alt="Interface: SPI" src="https://img.shields.io/badge/Interface-SPI-27AE60?style=flat-square" />
  <img alt="Driver: SSD1309" src="https://img.shields.io/badge/Driver-SSD1309-E7352C?style=flat-square" />
</p>

<p align="center"><img alt="OSPTEK 1.54 寸 OLED 128×64 模组（SSD1309）宣传图" src="./images/product.png" width="640" /></p>

## 目录

- [分支介绍](#分支介绍)
- [产品简介](#产品简介)
- [规格参数](#规格参数)
- [示例工程](#示例工程)
- [仓库结构](#仓库结构)
- [相关资料](#相关资料)
- [购买链接](#购买链接)
- [技术支持](#技术支持)

---

## 分支介绍

本仓库按料号分分支（**默认分支**为屏幕 [`OED154-12864W002-C24`](https://github.com/osptek/1.54-oled-128x64-spi-ssd1309/tree/OED154-12864W002-C24)）：

| 分支 | 说明 |
| ---- | ---- |
| [`OED154-12864W002-C24`](https://github.com/osptek/1.54-oled-128x64-spi-ssd1309/tree/OED154-12864W002-C24) | 1.54″ 128×64 OLED **屏幕**规格与资料 |
| **[`ODM154-12864W001-P7`](https://github.com/osptek/1.54-oled-128x64-spi-ssd1309/tree/ODM154-12864W001-P7)** | **本分支** · 配套 **模组**（含 PCB）规格与资料 |

## 产品简介

OSPTEK **1.54 寸 128×64 OLED** 是一款 **SPI** 单色显示模组，驱动芯片为 **SSD1309**。适合状态栏、菜单提示、调试信息与简单动画等显示场景。

规格标识（仓库名）：`1.54-oled-128x64-spi-ssd1309`

> 规格书中驱动 IC 可能标注为 **SPD0301**：与 **SSD1309** 为同一芯片族的不同料号，**软件完全兼容**；本仓库统一按 **SSD1309** 命名。

当前模组版本：**ODM154-12864W001-P7**。模组细节以 [`docs/ODM154-12864W001-P7.pdf`](./docs/ODM154-12864W001-P7.pdf) 为准；屏幕规格以 [`docs/OED154-12864W002-C24.pdf`](./docs/OED154-12864W002-C24.pdf) 为准（屏幕分支：[`OED154-12864W002-C24`](https://github.com/osptek/1.54-oled-128x64-spi-ssd1309/tree/OED154-12864W002-C24)）。

## 规格参数

| 项目 | 规格 |
| ---- | ---- |
| 尺寸 | 1.54 英寸 |
| 类型 | OLED（单色） |
| 分辨率 | 128×64 |
| 接口 | SPI（4-wire） |
| 驱动 IC | SSD1309 |

> 完整外形尺寸、引脚定义、供电与电气特性以产品规格书 / 驱动手册为准。

## 示例工程

| 说明 | 路径 |
| ---- | ---- |
| ESP32-S3 · SSD1309 SPI bringup（动画演示） | [`examples/esp32s3-1.54-oled-128x64-spi-ssd1309-bringup/`](./examples/esp32s3-1.54-oled-128x64-spi-ssd1309-bringup/) |

## 仓库结构

```text
1.54-oled-128x64-spi-ssd1309/
├── README.md
├── README_EN.md
├── MODULE_VERSION.md
├── LICENSE
├── images/          # README 用图
├── docs/            # 规格书、驱动手册、初始化等
└── examples/        # 示例工程
```

## 相关资料

### 本产品资料

| 资料 | 链接 |
| ---- | ---- |
| 模组规格书（ODM154-12864W001-P7） | [`docs/ODM154-12864W001-P7.pdf`](./docs/ODM154-12864W001-P7.pdf) |
| 屏幕规格书（OED154-12864W002-C24） | [`docs/OED154-12864W002-C24.pdf`](./docs/OED154-12864W002-C24.pdf) |
| 驱动 IC 数据手册（SSD1309 / SPD0301） | [`docs/SPD_0301_0_1_d964ef5b8f.pdf`](./docs/SPD_0301_0_1_d964ef5b8f.pdf) |
| 初始化代码 | [`docs/1.54寸 2864初始化(spd0301).C`](./docs/1.54%E5%AF%B8%202864%E5%88%9D%E5%A7%8B%E5%8C%96%28spd0301%29.C) |

### 示例工程

- [ESP32-S3 SSD1309 SPI bringup](./examples/esp32s3-1.54-oled-128x64-spi-ssd1309-bringup/)

## 购买链接

<p align="center">
  <a href="https://shop110742373.taobao.com/"><img alt="淘宝官方店铺" src="https://img.shields.io/badge/淘宝-官方店铺-FF6A00?style=for-the-badge" /></a>
  &nbsp;&nbsp;
  <a href="https://www.aliexpress.com/store/1105701619"><img alt="速卖通官方店铺" src="https://img.shields.io/badge/速卖通-官方店铺-FF6A00?style=for-the-badge" /></a>
</p>

**国内（淘宝）**

- 店铺：[鱼鹰光电工厂店](https://shop110742373.taobao.com/)

**海外（AliExpress）**

- 店铺：[OSPTEK Official Store](https://www.aliexpress.com/store/1105701619)

## 技术支持

- 技术支持 / 产品咨询：<luyu@osptek.com>
- QQ 技术交流群：**985881096**
- 公司官网：<https://osptek.com/>
- 有任何问题，都可以在本仓库 Issues 中提问

---

<p align="center"><sub>© 2026 OSPTEK 鱼鹰光电 · 本仓库资料采用 CC BY 4.0 许可</sub></p>
