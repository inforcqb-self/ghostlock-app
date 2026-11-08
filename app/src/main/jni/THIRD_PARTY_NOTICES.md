# Third-party notices — `app/src/main/jni`

Scope: everything the `libmagica2.so` JNI library compiles. Origins and licences
as they stand in the tree *today*; the open item is spelled out at the bottom.

| Path | Origin | Licence header in the file |
|---|---|---|
| `magica.cpp` | ported from the Magica fork (`.scratch/magica-upstream`, branch `v21-adbroot-fix`; upstream Magica v2.1) | none — Magica upstream is **Unlicense** (public domain) |
| `logging.h` | Magica upstream, `TAG` retargeted | none — Unlicense |
| `android_filesystem_config.h` | AOSP (`system/core/include/private/android_filesystem_config.h`) | **Apache-2.0**, kept intact |
| `system_properties/**` | vendored AOSP `libcutils`/`libbase` properties fork (this is the bundled resetprop: `__system_property_find/update/add/delete/set`) | **BSD-3-Clause / Apache-2.0**, kept intact (every `.cpp`/`.h` carries its own AOSP header) |
| `lsplt/syscall.hpp` | Google / AOSP `syscall` wrapper | **BSD-3-Clause**, kept intact |
| `lsplt/**` (rest: `lsplt.cc`, `elf_util.cc/.hpp`, `logging.hpp`, `include/lsplt.hpp`) | **LSPlt**, an LSPosed project, vendored via Magica | **no licence header at all** |
| `Android.mk`, `Application.mk` | Magica/bionic-convention build files, adapted | n/a (build description, not shipped in the APK) |

The vendored files carry the licence headers they arrived with; **do not strip
those headers** when touching the files (in particular everything under
`system_properties/**` and `android_filesystem_config.h`).

Keep-this-in-mind facts about the vendored copies:

* `system_properties/**` is a *fork* of AOSP (it patches around the platform's
  restrictions, see `include/api/hacks.h`), so it is not a verbatim AOSP copy even
  though the headers are AOSP's.
* `lsplt/**` is used for exactly one hook: `capset` in
  `/system/lib64/libandroid_runtime.so` (see `JNI_OnLoad` in `magica.cpp`). If the
  licensing is not settled, that hook is the only thing that has to be rewritten —
  a self-written PLT/GOT patcher for one symbol is a small, contained piece of work
  (read the ELF, find the `.rela.plt` entry for `capset`, mprotect the page
  writable, store the replacement address, flush the instruction cache).

## Also shipped in the APK: SukiSU-Ultra's `ksud` and its manager app

Neither is compiled here. The build step `Stage the pinned ksud + manager` copies two files out of
`prebuilt/sukisu-ci-main-8662564/` (their sha256 is asserted in the workflow, and the provenance is in
that directory's README) into the APK. Both come from the **same upstream `main` CI run**
(`SukiSU-Ultra/SukiSU-Ultra` run `42467362341`, `Build Manager`, head `8662564`, 2026-11-08):

* `lib/arm64-v8a/libksud.so` — the standalone `ksud` built by that run (artifact
  `ksud-aarch64-linux-android`), sha256
  `361a880d63702c09ae2ad52d277e17272bbe5eafce1cab6812ec5fd0f714dab5`, 7877344 bytes; it reports
  itself as `4.3.0-r2-13-g86625649 (uapi: 6)`. The chain invokes it as `ksud resetprop …` (busybox
  style), by absolute path, from the app's own copy (the private staged directory, and `98ca011`
  additionally installs it to `/data/adb/ksud` before `late-load`). The kernel module is packed
  **inside** it (`pack_lkm`), which is why no separate `.ko` is shipped. It is a CI build rather than
  a release because the newest release (`v4.3.0-r2`, 2026-11-05) carries the selinux-hide line only
  through #3936, while upstream's #3969 (synced into SukiSU `main` as `e17a48a7d` on 2026-11-06) is
  not in any release yet; `main`'s `kernel/feature/selinux_hide.c` is blob `385b415a…`, byte-identical
  to `tiann/KernelSU`'s `main`. The commits this pin line tracks are all in the build — #3877 (the
  timing side-channel fix, SukiSU `b445d0888` == upstream `a85dcbcf`), #3945 (SukiSU `15b64d095` ==
  upstream `528bafc56`), #3936 (`e67b14e6b`, the v4.3.0 tag) and #3969 (`e17a48a7d`).
* `assets/device/sukisu-manager.apk` — that run's `Manager` artifact
  (`SukiSU_v4.3.0-r2_41163-release.apk`: `package com.sukisu.ultra`, `versionCode 41163`,
  `versionName v4.3.0-r2`, signer
  SHA-256 `947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef`), sha256
  `fd3c072df508e71975976f891a4240fccd4a9937422a426358b7fa9bc6ac7964`, 20282847 bytes — so a device
  that cannot reach GitHub still gets a manager: the chain pushes it and runs `pm install -r` with
  root. Its package is `com.sukisu.ultra`, which is what `ChainSpec.KSU_MANAGER_PACKAGE` names. The
  pair is self-consistent: the `aarch64-android13-5.15-lkm` artifact of the same run (the copy
  embedded in this `ksud`, 676 192 bytes, sha256 `d7fcbd72…`) pins exactly this one manager
  certificate and no temporary key; every previous pin carried the same signer, so on a device that
  already has the manager the install is an in-place upgrade (41150 -> 41163, no uninstall, no lost
  grants). Taking `main` instead of the release only changes the manager's versionCode, which is a CI
  counter rather than a release's.

The pins before this one were the official v4.3.0-r2 (41150) release pair, the official v4.3.0 (41100)
release pair, the upstream `main` CI build `4.2.1-r2-29-g080ba098` (the #3877 + #3945 pair, before a
release had it), the official v4.2.1 (40994) release pair, our fork's `ci` `ksud` (`ci@7a1c0438`, the
KernelSU #3877 port) paired with the
official v4.2.0 manager, an upstream `main@4c15fca1` pair, and before that a KernelSU
`v3.3.0-69-gdf03912f` pair (their READMEs are in git history) — committed rather than fetched for the
same reason: the tree must be reproducible without a token, and a pinned pair with asserted sha256s
makes a silent swap fail the build.

**Licence: GPL-3.0** (KernelSU family) for both. GPL-3.0 is copyleft, so redistributing an APK that
carries these binaries obliges the distributor to the GPL's terms (source offer and licence text).
It is recorded here so that decision is made on purpose — the same reason the LSPlt item below is
still open.

## TODO(licence) — must be settled before redistribution

**LSPlt (`app/src/main/jni/lsplt/**`, excluding `syscall.hpp`) is vendored with no
licence header and no settled licence for this tree.** Upstream LSPlt is an LSPosed
project; its licence terms have to be confirmed and recorded here (and a notice
added to `lsplt/**` and to the APK's own notices) **or** the library has to be
replaced by a self-written PLT/GOT patcher as described above. Until then this code
is prototype-grade and must not be redistributed in a release build.
