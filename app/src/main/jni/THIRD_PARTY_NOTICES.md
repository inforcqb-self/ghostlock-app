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
`prebuilt/sukisu-ultra-3877-7a1c0438/` (their sha256 is asserted in the workflow, and the provenance
is in that directory's README) into the APK. They have two different provenances on purpose:

* `lib/arm64-v8a/libksud.so` — the standalone `ksud`, **built by this project's own fork**
  `inforcqb/SukiSU-Ultra`, whose branch `ci` is upstream main `223897c7` plus one ported KernelSU
  commit: `7a1c0438` "sync: port KernelSU's selinux hide fix (#3877) verbatim"
  (`kernel/feature/selinux_hide.c`, +214/−125, from `tiann/KernelSU@a85dcbcf`). Built through that
  fork's PR #1 (`ci` → `main`, the `pull_request` path with a temporary keystore), CI run
  `38119744728`, artifact `ksud-aarch64-linux-android`, sha256
  `d7fec18cd9c2bc7b6e437797e3ac8898449187b51b46698cc3dc59ffa21b21b6`, 6999648 bytes. The chain
  invokes it as `ksud resetprop …` (busybox style), by absolute path, from the app's own copy (the
  private staged directory, and `98ca011` additionally installs it to `/data/adb/ksud` before
  `late-load`). The kernel module is packed **inside** it (`pack_lkm`), which is why no separate
  `.ko` is shipped.
* `assets/device/sukisu-manager.apk` — **upstream's official** manager APK
  (`SukiSU-Ultra/SukiSU-Ultra` CI run `37934517573`, `main@4c15fca1`, file
  `SukiSU_v4.2.0_40965-release.apk`, sha256
  `d3b07d2638745e79faa765c9bfe9281269414399e0550a13cc186a88e645fbb4`), so a device that cannot reach
  GitHub still gets a manager: the chain pushes it and runs `pm install -r` with root. Its package is
  `com.sukisu.ultra`, which is what `ChainSpec.KSU_MANAGER_PACKAGE` names. It stays the official
  build because the fork's LKM pins **two** manager certificates — the official one and that run's
  temporary PR key (both verified with `apksigner`) — so the fork-built `ksud` accepts it and the
  device-side install remains a no-op upgrade.

The pin before this one was an upstream `main@4c15fca1` pair, and before that a KernelSU
`v3.3.0-69-gdf03912f` pair (their READMEs are in git history) — committed rather than fetched
for the same reason: another repository's CI artifacts need a token this workflow does not have,
and a pinned pair with asserted sha256s makes the build reproducible.

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
