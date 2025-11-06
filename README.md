# Lightweight Mouse Control

一个轻量级的鼠标录制和回放工具，支持录制鼠标轨迹并精确回放
A lightweight mouse recording and playback tool that supports recording mouse movements and accurately replaying them


## 编译说明

### 编译步骤

1. 进入项目目录
2. 生成构建文件：
   ```
   qmake.exe Lightweight-Mouse-Control.pro
   ```
3. 编译项目：
   ```
   mingw32-make.exe -j20
   ```

## 使用说明

### 基本操作

1. **开始录制**: 点击"Start Recording"按钮或按 Ctrl+B
2. **停止录制**: 再次点击按钮或按 Ctrl+B，路径将自动保存
3. **回放路径**:
   - 在左侧列表中选择一个已保存的路径
   - 调整回放速度（可选）
   - 点击"Play Selected Path"按钮
4. **停止回放**: 点击"Stop"按钮
5. **小窗模式**: 体验精简的录制和重播功能


**注意**: 使用全局热键功能可能需要管理员权限。某些杀毒软件可能会将此类程序标记为可疑，这是正常现象。
