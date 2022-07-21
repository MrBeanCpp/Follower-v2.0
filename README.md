# Follower v2.0

## 介绍

`Windows` 桌面助手，鼠标`中键`唤醒（or `Ctrl+Shift+E`）
可自定义命令，快捷开启URL（本地目录\程序、网址、cmd命令等）

额外功能：提醒事项、滚轮调节音量、键盘锁定、自动切换EN输入法、检测全屏等

![Move](https://s1.ax1x.com/2022/05/23/X9uXdK.png)  ![Still](https://s1.ax1x.com/2022/05/23/X9KiLt.png)  ![Input](https://s1.ax1x.com/2022/05/23/X9KeJg.png)

## 详细介绍

### 命令分类

`#`前缀：代表系统内置命令，如`#Edit cmd`可开启自定义命令编辑框

`@`前缀：代表`JS`语句，可用来进行简单数学运算，如`@Math.PI`

`!`前缀：调用百度翻译`API`进行`非EN->EN` & `EN->ZH`转换

无前缀：则主要用于匹配自定义命令，同时会识别绝对路径，如`C:\Windows`会打开对应文件夹，若均不匹配则尝试为`CMD`命令，如`shutdown -s -t 0`

![Inputing](https://s1.ax1x.com/2022/05/23/X9K8oT.png)

### 主要功能（自定义命令）

输入`#Edit cmd`打开自定义命令编辑窗口（`Ctrl+F`搜索），支持拖拽文件

![Editor](https://s1.ax1x.com/2022/05/23/X9KITf.md.png)

分为`Code`、`Extra`、`FileName`、`Parameter`四列

`Code`：代表用于匹配的自定义命令

`Extra`：在命令后显示用于提示（但不参与匹配），如`Code == VS`，`Extra == (Visutal Studio)`，最后显示为`VS(Visutal Studio)`

`FileName`：命令对应的`URL`，可以是绝对路径或网址

`Parameter`：给`URL`传递的启动参数

#### 例子

[]中代表每一列的内容，[`NULL`]代表空

- [notepad] [`NULL`] [C:\WINDOWS\system32\notepad.exe] [`NULL`]
- [bilibili] [`NULL`] [http://www.bilibili.com] [`NULL`]
- [VS] [(Visual Studio 2019)] [D:\Visial Studio 2019\Common7\IDE\devenv.exe] [`NULL`]
- [shutdown] [`NULL`] [C:\WINDOWS\system32\cmd.exe] [/c shutdown -s -t 0]    //`CMD`参数加上"/c"(close)，不然命令不会执行// "/k"(keep)也行
- [Startup] [(启动项文件夹)] [C:\Users\18134\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup] [`NULL`]

## 次要功能

- `#Edit note`：提醒事项
- `#Edit InputMethod`：用以将特定窗口设置为`EN`输入法，防止干扰编码和游戏，输入可执行文件名或窗口标题中包含的字符即可，如`pycharm64.exe` or `Visual Studio`
- `#Lock`：用以锁定键盘（按中键唤起并单击左键进入`INPUT`模式即可解除），用以防止看书时压倒键盘误触
- 笔记本断电上电提醒
- 24:00准时提醒睡觉
- 全屏时鼠标中键失效（防止游戏时寄掉）
- 唤起后，直接使用滚轮调节音量（不要点击左键进入`INPUT`）
- 续↑：用`Ctrl+滚轮`调整屏幕亮度（响应较慢）
- 可设置启动项（托盘）
- 可自动检测更新（托盘）
- 复制文本`1000ms`内唤起`Follower`，则自动进入翻译

### 操作手册

唤醒：鼠标中键 | 键盘（ `Ctrl+Shift+E`）

左键（|空格）单击开启`INPUT`模式，右键（| `ESC`| `Ctrl+Backspace`）可退出 & 最小化

`INPUT`状态下可使用`↑↓`键切换命令条目，使用`Ctrl+↑↓`来切换命令记录，回车执行命令

## Log

详情参见Log.txt

## 注

Https下载 需要openssl的支持（自行下载dll）
