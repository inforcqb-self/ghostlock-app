# jniLibs/ — 内置的 adb 命令行

`arm64-v8a/libadbcli.so` **不是**共享库，而是 **Android platform-tools 的 `adb` 可执行文件**
（重命名成 `lib*.so` 才会被 AGP 打进 APK 并解压到 App 的 `nativeLibraryDir` —— 那里是唯一
允许 App 执行自己文件的位置；`/data/data` 下的文件被 SELinux 禁止执行）。

* 来源：`lzhiyong/android-sdk-tools` release **34.0.3** 的 `android-sdk-tools-static-aarch64.zip`
  里的 `platform-tools/adb`（静态链接 aarch64，`e_machine=0xB7`，无 `PT_INTERP`），
  许可证 **Apache-2.0**（platform-tools 的 `NOTICE`）。把 `NOTICE` 的归属信息保留在这里。
* **为什么用 34.0.3 而不是更新的 35.0.2**：35.0.2 的这份静态构建在 Android 上跑
  **adb server 会崩** —— 真机实测（PJA110）无论 `start-server` 还是
  `-L tcp:5037 fork-server server --reply-fd` 都在加载密钥后 `Aborted`
  （`fdsan: failed to exchange ownership of file descriptor`，`adb.2000.log` 里可见）；
  34.0.3 的 server 正常启动，且 `pair` / `connect` / `shell` / **退出码透传** 全部实测通过。
  换版本前请先在设备上验证 `start-server` 能起来。
* `app/build.gradle.kts` 里对 `**/libadbcli.so` 设了 `keepDebugSymbols`，避免 AGP 用 NDK 的
  `llvm-strip` 动这个可执行文件。

## `arm64-v8a/libksud.so` —— `ksud`（构建时从 `prebuilt/` 的**钉住版本**复制，**不committed**）

同样是**可执行文件而不是共享库**（ELF 64-bit LSB pie executable, ARM aarch64），
由 App 在跑链之前推送到设备的 `/data/local/tmp/gl-w1/ksud`（0755），链里**用绝对路径**调用。
`app/build.gradle.kts` 里对 `**/libksud.so` 设了 `keepDebugSymbols`（别让 llvm-strip 动它）。

* **来源（钉死，不再跟"最新 release"走）**：CI 的 `Stage the pinned ksud + manager` 步骤把
  `prebuilt/sukisu-ultra-4.2.1-40994/ksud` 复制到这里，并**校验 sha256**
  （`3dcbf20265743b094111c7f41022206c8434affb8f6904322428cc94460ce02e`，7162112 B）。
  这份 `ksud` 是上游 **`SukiSU-Ultra/SukiSU-Ultra` 官方稳定版 `v4.2.1`（versionCode 40994）**
  那个 tag 自己的 CI 产物：run `38704356559`（`Release`，head `fd2a8515`，2026-10-15），
  产物 `ksud-aarch64-linux-android`；二进制里自报 `4.2.1 (uapi: 5)`。**内核模块就打包在它里面**
  （`pack_lkm`），所以不需要单独下载 `.ko`。
  早先那个"自己 fork 一份只为移植 KernelSU #3877 的 selinux-hide 修复"的构建已经不需要了 ——
  该修复已在 `v4.2.1` 里（该 tag 的 `kernel/feature/selinux_hide.c` blob = `459f99f9…`，即修复后的内容）。
  换版本的方法写在 `prebuilt/sukisu-ultra-4.2.1-40994/README.md`（更早的 fork `ci`
  和上游 `main@4c15fca1` 两版见 git 历史）。
* 该文件**不提交进仓库**（`.gitignore` 已忽略，提交的是 `prebuilt/` 里那两份原件）；本地手工构建：

  ```sh
  mkdir -p app/src/main/jniLibs/arm64-v8a
  cp prebuilt/sukisu-ultra-4.2.1-40994/ksud app/src/main/jniLibs/arm64-v8a/libksud.so
  ```
* 为什么打进 APK：链的第 8b 步要把 `ro.secure` / `ro.debuggable` 写回去，而**裸 `resetprop`
  不在 `adb shell` 的 PATH 里**（真机实测：设备上只有管理端安装目录里那个符号链接
  `/data/adb/ksu/bin/resetprop -> /data/adb/ksud`）；`ksud resetprop` 是绝对路径可寻址、
  且与版本无关的写法。实测：`…/ksud resetprop ro.secure` → `1`，
  `su -c '… -Z ro.secure'` → `u:object_r:userdebug_or_eng_prop:s0`。
* `prepareKsud()` 的选择顺序：**先本 APK 自带**这一份（链与"链装的那些"都是按它做的），只有 APK 里
  根本没有 `libksud.so` 时才退回已安装管理端里的那份——那份属于设备上原来那套 KSU，正是这条链不该依赖的东西。
* `ksud late-load`（步骤 10）用的也是**这一份**：步骤 9「安装自带 ksud」会先把它覆盖到
  `/data/adb/ksud`（`mkdir -p /data/adb && cp -f … && chmod 755`，并 `test -x` 读回确认），因为
  `late-load` 与 part 1 装上的管理端都认这个路径，而**从没装过 KernelSU 的设备上它根本不存在**、
  装着别的 KSU 分支的设备上它又是另一套 ksud。装不上且路径上也没有可执行文件时，链会在这一步停下并说明。
  链里的 `resetprop`（步骤 8b）本来就用自带副本，不走这个路径。
* 许可证：**GPL-3.0**（KernelSU 家族），见 `app/src/main/jni/THIRD_PARTY_NOTICES.md`。

## `assets/device/sukisu-manager.apk` —— 管理端（上游官方 `v4.2.1`）

CI 的同一步把 `prebuilt/sukisu-ultra-4.2.1-40994/sukisu-manager.apk` 复制到
`app/src/main/assets/device/sukisu-manager.apk`（也不提交进仓库，sha256
`c7f69280cf468e8415c585ff31f963d0147386831217a1e89b822afddc5f40ed`，17173205 B）；
文件名保持 `sukisu-manager.apk` 是为了不动链里的 `ChainSpec.KSU_MANAGER_APK`。
它就是上游 `v4.2.1` 那个 release 的正式发布包 `SukiSU_v4.2.1_40994-release.apk`
（`package com.sukisu.ultra`、`versionCode 40994`、`versionName v4.2.1`），签名证书
SHA-256 `947ae944…`（`apksigner` 实测），正是该 tag 的 LKM 里钉的那一张 —— 两者同源，
不再有"fork 构建里混了个临时 PR 密钥"这种事。与上一版（`v4.2.0` / 40965）**同一个签名者**，
所以设备侧 `pm install -r` 仍是原地升级：40965 → 40994，不用卸载、不丢授权。
链的第 7b 步把它推到 `/data/local/tmp/gl-w1/sukisu-manager.apk` 并用 root `pm install -r` 装上，
这样设备侧不需要访问 GitHub 就能拿到管理端（`su` 授权、模块管理）。
装上的包名因此是 **`com.sukisu.ultra`**，与 `ChainSpec.KSU_MANAGER_PACKAGE` 一致
（更早一版钉的是 KernelSU 的管理端，包名是 `me.weishu.kernelsu`）。


