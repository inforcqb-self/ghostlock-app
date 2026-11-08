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
`prebuilt/sukisu-ultra-4.3.0-r2-41150/` (their sha256 is asserted in the workflow, and the provenance
is in that directory's README) into the APK. Both are assets of the **same official stable release**,
SukiSU-Ultra `v4.3.0-r2` (tag `645ef986`, published 2026-11-05):

* `lib/arm64-v8a/libksud.so` — the release asset `ksud-aarch64-linux-android`, sha256
  `dec34535789f28e3ecda2788618a2e57c70f77b17b5ce129ff50026b9ba42f35`, 7867384 bytes; it reports
  itself as `4.3.0-r2 (uapi: 6)`. The chain invokes it as `ksud resetprop …` (busybox style), by
  absolute path, from the app's own copy (the private staged directory, and `98ca011` additionally
  installs it to `/data/adb/ksud` before `late-load`). The kernel module is packed **inside** it
  (`pack_lkm`), which is why no separate `.ko` is shipped. Releases have published the `ksud`
  themselves since `v4.2.1-r1`, so no CI-run build is involved; both behaviours the chain relies on
  were re-checked in this tag's tree (`resetprop --delete` exists, and the `late-load` flags stay
  optional so the plain no-flag form still loads the embedded LKM). The selinux-hide line this pin
  chased is in the release through #3936 — #3877 (the timing side-channel fix, SukiSU `b445d0888` ==
  upstream `a85dcbcf`), #3945 (SukiSU `15b64d095` == upstream `528bafc56`) — with
  `kernel/feature/selinux_hide.c` at blob `2a41b805…`. Upstream's later #3969 (a symtab-search
  refactor, SukiSU `e17a48a7d`, blob `385b415a…`) is only in SukiSU `main`, because it landed one day
  after this release: that is the deliberate trade for an official stable pin. The earlier fork build
  (this project's `inforcqb/SukiSU-Ultra` PR #1, porting #3877 by hand), the `main` CI build that
  replaced it, and the v4.3.0 release before this one are all superseded.
* `assets/device/sukisu-manager.apk` — the release's asset
  `SukiSU_v4.3.0-r2_41150-release.apk` (the plain build, not the `-spoofed` variant; `package
  com.sukisu.ultra`, `versionCode 41150`, `versionName v4.3.0-r2`, signer
  SHA-256 `947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef`), sha256
  `6fe64e7bcf9fd768c58f46afea2f27ee9fb85d2783472a7e81334df21e700636`, 20131351 bytes — so a device
  that cannot reach GitHub still gets a manager: the chain pushes it and runs `pm install -r` with
  root. Its package is `com.sukisu.ultra`, which is what `ChainSpec.KSU_MANAGER_PACKAGE` names. The
  pair is self-consistent: the `aarch64-android13-5.15-lkm` asset of the same release (the copy
  embedded in this `ksud`, 671 360 bytes, sha256 `f0d3f0a3…`) pins exactly this one manager
  certificate and no temporary key; every previous pin carried the same signer, so on a device that
  already has the manager the install is an in-place upgrade (no uninstall, no lost grants).

The pins before this one were the official v4.3.0 (41100) release pair, the upstream `main` CI build
`4.2.1-r2-29-g080ba098` (the #3877 + #3945 pair, before a release had it), the official v4.2.1
(40994) release pair, our fork's `ci` `ksud` (`ci@7a1c0438`, the KernelSU #3877 port) paired with the
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
