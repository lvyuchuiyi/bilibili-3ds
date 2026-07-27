# BiliBili for 3DS

在 Nintendo 3DS 上浏览和观看 BiliBili 视频的第三方客户端。

## 功能

- 浏览 BiliBili 推荐视频
- 搜索视频（支持中文关键词）
- 查看视频详情（标题、UP主、播放量）
- 硬件加速视频播放（3DS MVD H.264 解码器）
- 触摸屏 + 实体按键操作

## 硬件要求

- Nintendo 3DS / 3DS XL / 2DS / New 3DS / New 3DS XL / New 2DS XL
- 系统版本 11.0+（支持 Homebrew Launcher）
- SD 卡、Wi-Fi 连接

## 编译方法

### 方法一：双击 setup.bat（推荐）

1. 双击 setup.bat
2. 安装器自动下载 devkitPro，按提示安装（勾选 3DS Development）
3. 安装完成后自动编译
4. 编译好的文件在 outputs/ 目录

### 方法二：GitHub Actions（无需本地安装）

1. Fork 本仓库
2. 进入 Actions 页面
3. 手动触发 Build BiliBili 3DS 工作流
4. 编译完成后下载 bilibili3ds-build.zip

## 安装到 3DS

### CIA 安装（推荐）
- 将 bilibili3ds.cia 复制到 SD 卡
- 运行 FBI - SD - Install and Delete CIA
- 应用出现在主屏幕

### 3DSX 安装
- 将 bilibili3ds.3dsx 和 bilibili3ds.smdh 复制到 /3ds/bilibili3ds/
- 打开 Homebrew Launcher 运行

## 操作说明

| 按键 | 功能 |
|------|------|
| 十字键上下 | 列表滚动 |
| A | 确认 / 选择 / 播放 |
| B | 返回上一级 |
| X | 播放 / 暂停 |
| 触摸屏 | 点击选择 / 键盘输入 |

## 技术栈

- UI: citro3d（GPU 加速 2D 渲染）
- 网络: SOC service + mbedTLS（HTTPS）
- API: BiliBili 非官方 API
- 视频解码: 3DS MVD 硬件 H.264 解码器

## 注意事项

- 本项目为第三方客户端，与 BiliBili 官方无关
- 播放使用 MVD 硬件解码，240p/360p H.264 最适配
- 首次加载推荐视频需要数秒（取决于网络）

## 许可

MIT License
