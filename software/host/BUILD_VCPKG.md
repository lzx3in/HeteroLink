# vcpkg 构建说明

## 前提条件

1. 确保 `VCPKG_ROOT` 环境变量已设置：
   ```powershell
   $env:VCPKG_ROOT
   # 应该输出：E:\Code\OpenSource\vcpkg
   ```

2. 确保 vcpkg 已更新到最新版本：
   ```powershell
   cd $env:VCPKG_ROOT
   git pull
   ```

## 构建步骤

### 1. 清理旧构建（如需要）

```powershell
cd E:\Code\HeteroLink\software\host
Remove-Item -Recurse -Force build\windows-msvc-2026 -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force vcpkg_installed -ErrorAction SilentlyContinue
```

### 2. 安装 vcpkg 依赖

```powershell
cd E:\Code\HeteroLink\software\host

# 如果代理可用，设置代理
$env:HTTP_PROXY = "http://127.0.0.1:10808"
$env:HTTPS_PROXY = "http://127.0.0.1:10808"

# 安装依赖（包括 Qt 6.10.2）
& "$env:VCPKG_ROOT\vcpkg" install
```

**注意：** Qt 6.10.2 需要从源码编译，预计需要 30-60 分钟，取决于网络和设备性能。

### 3. 配置 CMake

```powershell
# 确保代理设置仍然有效
$env:HTTP_PROXY = "http://127.0.0.1:10808"
$env:HTTPS_PROXY = "http://127.0.0.1:10808"

# 使用预设配置
cmake --preset windows-msvc-2026
```

### 4. 构建

```powershell
cmake --build --preset windows-msvc-2026-build
```

## 已配置的 vcpkg 依赖

- `qtbase` 6.10.2 (GUI, Widgets)
- `qtserialport` 6.10.2
- `qtmqtt` 6.10.2
- `qtcharts` 6.10.2
- `fmt`
- `spdlog`

## 故障排除

### Qt 构建失败

如果 Qt 6.10.2 构建失败，尝试：

1. 清理 vcpkg 构建缓存：
   ```powershell
   Remove-Item -Recurse -Force "$env:VCPKG_ROOT\buildtrees\qtbase" -ErrorAction SilentlyContinue
   Remove-Item -Recurse -Force "$env:VCPKG_ROOT\packages\qtbase_x64-windows" -ErrorAction SilentlyContinue
   ```

2. 重新运行 `vcpkg install`

### 网络问题

如果下载失败，确保代理设置正确：
```powershell
$env:HTTP_PROXY = "http://127.0.0.1:10808"
$env:HTTPS_PROXY = "http://127.0.0.1:10808"
```

或者使用 `--no-downloads` 如果已有缓存。
