# Pinned pair: upstream's official SukiSU-Ultra **v4.3.0-r2** (41150)

The two files the build packages into the APK, pinned in the tree. Both are assets of the **same
official stable release**, so the pair is self-consistent by construction -- no CI-run build, no
temporary key, and the release ships the `ksud` itself.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | the standalone `ksud` for aarch64, published as a release asset | 7867384 | `dec34535789f28e3ecda2788618a2e57c70f77b17b5ce129ff50026b9ba42f35` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 20131351 | `6fe64e7bcf9fd768c58f46afea2f27ee9fb85d2783472a7e81334df21e700636` |

## Provenance

Release **`v4.3.0-r2`**, published 2026-11-05, tag commit `645ef9860b9b7f900bb5d1b27e224f2c311d39c0`;
assets `ksud-aarch64-linux-android`, `SukiSU_v4.3.0-r2_41150-release.apk` (the plain build, not the
release's `-spoofed` variant), and `lkm-aarch64-android13-5.15_kernelsu.ko` (671 360 B, sha256
`f0d3f0a3dcf9b27c7468f001e431e610d507b3ddd78c412ab41fa0189ea7ba1a`; it is the module this `ksud`
embeds and is not shipped separately).

What `ksud` reports about itself:

```
$ ksud -V                       ->  ksud 4.3.0-r2 (uapi: 6)
ELF64, class 2, machine 183 (AArch64)
```

The two ksud behaviours the chain depends on are still there at this tag (re-checked in the tag's
tree, not assumed):

* `resetprop --delete` (`-d`) -- `userspace/ksud/src/resetprop.rs` keeps the flag; this is the
  closing step that removes `service.adb.tcp.port`;
* the **no-flag** `late-load` form -- `userspace/ksud/src/cli.rs` keeps `--magica`, `--allow-shell`,
  `--post-magica`, `--kmi`, `--package-name` all optional, so `ksud late-load` still loads the
  embedded LKM and runs the stage scripts.

## Kernel side (selinux hide)

`kernel/feature/selinux_hide.c` at this tag is blob `2a41b805…` (40 537 B). The lineage it carries:

```
2026-10-10  tiann/KernelSU@a85dcbcf  fix time side-channel detection in selinux hide (#3877)
2026-10-13  SukiSU b445d0888         the same fix, ported verbatim (blob 459f99f9…)
2026-10-24  tiann/KernelSU@528bafc56 kernel: remove useless code (#3945)
2026-10-24  SukiSU 15b64d095         the same follow-up, ported (blob 8f23888b…)
2026-10-29  tiann/KernelSU@db6263d3b fix(kernel): align SELinux hide policy parsing (#3936)
2026-10-29  SukiSU e67b14e6b         the same, ported -- this is the v4.3.0 tag commit
2026-11-05  release v4.3.0-r2        blob 2a41b805…  (r2 == v4.3.0 here)
```

**One thing this release does *not* have yet:** upstream's `019a3c010` "refactor(kernel): Mark
original type datum to save symtab search (#3969)" landed 2026-11-06 and SukiSU ported it the same
day as `e17a48a7d` -- one day *after* r2 was published. SukiSU's `main` therefore has
`selinux_hide.c` = blob `385b415a…` (41 166 B, identical to upstream `main` today), while r2 has
`2a41b805…`. #3969 is a symtab-search optimisation (a refactor, not a published security fix), so
pinning the release trades that one commit for the "official stable" property. If it ever matters,
the `main` CI build is the alternative -- that is how the pin before v4.3.0 was chosen.

## `sukisu-manager.apk`

```
package      com.sukisu.ultra
versionCode  41150          versionName  v4.3.0-r2
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             (C=zh, ST=gaungdong, L=guangzhou, O=shirkneko, OU=shirkneko, CN=shirkneko)
```

Same signer as every previous pin (v4.2.0 / v4.2.1 / v4.2.1-r2 / the CI build 41097 / v4.3.0), so on
a device that already has the manager `pm install -r` is an in-place upgrade -- no uninstall, no
lost grants. The release's LKM pins exactly this one manager certificate and no temporary key.

## Why the files are committed instead of fetched

Upstream's assets are downloaded by hand and asserted by sha256 in `.github/workflows/build.yml`: a
silent swap fails the build. The previous pins (`prebuilt/sukisu-ultra-4.3.0-41100/`,
`prebuilt/sukisu-ci-main-080ba098/`, `prebuilt/sukisu-ultra-4.2.1-40994/`,
`prebuilt/sukisu-ultra-3877-7a1c0438/`, `prebuilt/sukisu-ultra-4c15fca1/`, before that the
KernelSU `v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
