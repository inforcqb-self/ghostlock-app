# Pinned pair: upstream's official SukiSU-Ultra **v4.3.0** (41100)

The two files the build packages into the APK, pinned in the tree. Both are assets of the **same
official stable release**, so the pair is self-consistent by construction -- no CI-run build, no
temporary key, and the release ships the `ksud` itself (it started doing that with v4.2.1-r1).

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | the standalone `ksud` for aarch64, published as a release asset | 7858624 | `e9fcad8189397d5c96965819ae1f184557c7a51701fed183a1e0190242a89501` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 19219289 | `a928eb57ad34b105233f92dcd55e54dd883482e417970276329a7d0f0979e212` |

## Provenance

Release **`v4.3.0`**, published 2026-10-29, tag commit `e67b14e6bbbbd3e67f24df658a01d3e73a64159c`;
assets `ksud-aarch64-linux-android`, `SukiSU_v4.3.0_41100-release.apk` (the plain build, not the
release's `-spoofed` variant), and `lkm-aarch64-android13-5.15_kernelsu.ko`.

What `ksud` reports about itself:

```
$ ksud -V                       ->  ksud 4.3.0 (uapi: 6)
ELF64, class 2, machine 183 (AArch64)
```

The two ksud behaviours the chain depends on are still there at this tag:

* `resetprop --delete` (`-d`) -- `userspace/ksud/src/resetprop.rs` keeps the flag; this is the
  closing step that removes `service.adb.tcp.port`;
* the **no-flag** `late-load` form -- `userspace/ksud/src/cli.rs` keeps `--magica`, `--allow-shell`,
  `--post-magica`, `--kmi`, `--package-name` all optional, so `ksud late-load` still loads the
  embedded LKM and runs the stage scripts.

## Kernel side (selinux hide)

The two KernelSU commits this pin line was originally chasing are both in this release and then
some: `kernel/feature/selinux_hide.c` at `v4.3.0` is blob `2a41b805…` (40 537 B), **the identical
blob to `tiann/KernelSU`'s `main`**. For the record, the lineage is

```
2026-10-10  tiann/KernelSU@a85dcbcf  fix time side-channel detection in selinux hide (#3877)
2026-10-13  SukiSU b445d0888         the same fix, ported verbatim (blob 459f99f9…)
2026-10-24  tiann/KernelSU@528bafc56 kernel: remove useless code (#3945)
2026-10-24  SukiSU 15b64d095         the same follow-up, ported (blob 8f23888b…)
2026-10-29  release v4.3.0           blob 2a41b805… == upstream main
```

so the fork build this repo once carried (`inforcqb/SukiSU-Ultra` PR #1) and the `main` CI build it
carried next (`4.2.1-r2-29-g080ba098`) are both superseded: a release now has everything they had.

Cross-check of the same release's `aarch64-android13-5.15-lkm` asset (666 288 B, sha256
`c82513223b08404c51399595e38fe4e423d5bc5eb4e89f2c2765e139b548cff2`; it is the module this `ksud`
embeds and is not shipped separately): it pins exactly **one** manager certificate, the official
`947ae944…` below, and no temporary key.

## `sukisu-manager.apk`

```
package      com.sukisu.ultra
versionCode  41100          versionName  v4.3.0
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             (C=zh, ST=gaungdong, L=guangzhou, O=shirkneko, OU=shirkneko, CN=shirkneko)
```

Same signer as every previous pin (v4.2.0 / v4.2.1 / v4.2.1-r2 / the CI build 41097), so on a device
that already has the manager `pm install -r` is an in-place upgrade -- no uninstall, no lost grants.

## Why the files are committed instead of fetched

Upstream's assets are downloaded by hand and asserted by sha256 in `.github/workflows/build.yml`: a
silent swap fails the build. The previous pins (`prebuilt/sukisu-ci-main-080ba098/` -- upstream
`main` CI build with #3877 + #3945; `prebuilt/sukisu-ultra-4.2.1-40994/` -- official v4.2.1 release;
`prebuilt/sukisu-ultra-3877-7a1c0438/` -- fork `ci` `ksud` + official v4.2.0 manager;
`prebuilt/sukisu-ultra-4c15fca1/` -- upstream `main@4c15fca1` pair; before that the KernelSU
`v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
