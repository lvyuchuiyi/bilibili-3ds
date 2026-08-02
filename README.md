# BiliBili for 3DS

Nintendo 3DS 上的 Bilibili 视频客户端。

## 功能

- ✅ 热门视频浏览
- ✅ 视频搜索
- ✅ 视频详情（标题、UP主、时长、播放量）
- ✅ 中文标题显示（内置 Droid Sans Fallback 字体）
- ✅ 低分辨率视频播放（360P MP4 + MVD 硬件解码，真机可用）
- ❌ 弹幕、评论、投币、直播（不在范围内）

## 安装

### 方法一：FBI 扫码安装

1. 打开 [Releases](https://github.com/lvyuchuiyi/bilibili-3ds/releases)
2. 下载最新 `bilibili3ds.cia`
3. 用 FBI 的 Remote Install 扫描二维码

### 方法二：Homebrew Launcher（3dsx）

1. 下载 `bilibili3ds.3dsx`
2. 复制到 SD 卡的 `/3ds/bilibili3ds/`
3. 打开 Homebrew Launcher 运行

## 操作说明

| 按键 | 功能 |
|------|------|
| ↑↓ | 列表滚动 |
| A | 确认/选择/播放 |
| B | 返回 |
| X | 暂停/继续 |

## 技术栈

- **UI**: citro2d + citro3d（GPU 加速 2D 渲染）
- **网络**: 3DS 原生 http:C 服务（自动处理 HTTPS/chunked/重定向）
- **API**: Bilibili Web API + WBI 签名（参数排序 + MD5）
- **字体**: Droid Sans Fallback（SIL OFL 许可，内置 romfs）
- **视频解码**: MVD 硬件 H.264 解码器
- **MP4 解析**: 内置最小解封装器（AVCC → Annex B）

## 构建

```bash
make -j$(nproc)
```

需要 devkitPro + devkitARM。

## 注意

- 视频播放依赖 3DS MVD 硬件，模拟器（Azahar/Citra）不支持
- 网络功能在 Azahar 模拟器中可以正常工作
