﻿# 1.插件介绍
一个整合了修改帧数和魔手功能的插件，提供一个浮动在游戏画面中的界面，免去操作外部程序。支持快捷键启停魔手。

暂无程序文件提供下载，待测试完成后就会提供。

# 2.插件使用方法
插件只会在OpenGL模式启动游戏内生效。</br>
将opengl32.dll游戏目录。</br>
**强烈建议删除游戏目录下的d3d8.dll & d3d9.dll & ddraw.dll**</br>
打开游戏后，画面中会出现插件小窗口。你可以展开，收起界面，拖动到角落，插件会记忆小窗口的位置。暂不支持完全隐藏。

# 修改帧数
修改FPS框中的数值来调整帧数。默认值为60，取值范围是[33,9999]。</br>

## 修改帧数FAQ

### 1.为什么游戏不能达到我设定的帧数？
插件不能凭空变出性能来。

### 2.为什么帧数不能超过显示器刷新率？
请关闭垂直同步。

### 3.为什么多开之后帧数变低了/提升帧数之后干啥都卡？
**游戏在同一个循环内处理渲染和逻辑，意味着两者用同一个线程处理**</br>
所以帧数变高，显卡和CPU占用会一起变高。需要自行确定是CPU还是显卡瓶颈。如果是CPU瓶颈，尝试手动将游戏进程分散到其他CPU核心。</br>
Windows7的CPU调度貌似不如Windows10智能，更加需要人工干预。

### 4.为什么我的帧数波动很大？
电脑性能不能一直满足所设定的帧数，建议降低设置。

# 魔手功能
将原来的单个全局间隔拆分为每个按键的耗时参数，更好适配不同后摇的技能。</br>
**这一魔手需要仔细调节参数，各个时间参数不能小于0.2秒**

## 使用方法

## 设置方法

## 多套配置