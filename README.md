# BiliBili for 3DS

�?Nintendo 3DS 上浏览和观看 BiliBili 视频的第三方客户端�?
## 功能

- 浏览 BiliBili 推荐视频
- 搜索视频（支持中文关键词�?- 查看视频详情（标题、UP主、播放量�?- 硬件加速视频播放（3DS MVD H.264 解码器）
- 触摸�?+ 实体按键操作

## 硬件要求

- Nintendo 3DS / 3DS XL / 2DS / New 3DS / New 3DS XL / New 2DS XL
- 系统版本 11.0+ (支持 Homebrew Launcher)
- SD 卡、Wi-Fi 连接

## 编译方法

### 方法一：双�?setup.bat（推荐）

1. 双击 `setup.bat`
2. 安装器自动下�?devkitPro，按提示安装（勾�?"3DS Development"�?3. 安装完成后自动编�?4. 编译好的文件�?`outputs/` 目录

### 方法二：手动编译

1. 安装 [devkitPro](https://devkitpro.org/wiki/Getting_Started)（�?3DS Development�?2. 打开 "devkitPro MSYS" 终端
3. 进入项目目录，运�?`make`

## 安装�?3DS

1. �?`bilibili3ds.3dsx` �?`bilibili3ds.smdh` 复制�?SD 卡的 `/3ds/bilibili3ds/`
2. SD 卡插�?3DS，打开 Homebrew Launcher 运行

## 操作说明

| 按键 | 功能 |
|------|------|
| ↑↓ | 列表滚动 |
| A | 确认/选择/播放 |
| B | 返回上一�?|
| X | 播放/暂停 |
| 触摸�?| 点击选择 / 键盘输入 |

## 技术栈

- **UI**: citro3d (GPU 加�?2D 渲染)
- **网络**: SOC service + mbedTLS (HTTPS)
- **API**: BiliBili 非官�?API
- **视频解码**: 3DS MVD 硬件 H.264 解码�?
## 注意事项

- 本项目为第三方客户端，与 BiliBili 官方无关
- 播放使用 MVD 硬件解码�?40p/360p H.264 最适配
- 首次加载推荐视频需要数秒（取决于网络）

## 许可

MIT License

