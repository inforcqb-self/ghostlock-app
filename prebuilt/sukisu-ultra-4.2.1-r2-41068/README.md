# Pinned pair: upstream's official SukiSU-Ultra **v4.2.1-r2** (41068) `ksud` + manager

The two files the build packages into the APK, pinned in the tree. Both are assets of the **same
official release**, so the pair is self-consistent by construction -- no fork build, no CI artifact
from another repository, and no temporary PR key anywhere in it.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | the standalone `ksud` for aarch64, published by the `v4.2.1-r2` release | 7668680 | `abc3822f6b4b8521d57f9159aa5b7a56b51b4013eb13f9e38297339cbd79ddec` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 18489765 | `f60ba9fd7c3d8fe3966d7641ab755176f80100bcb8e9fa28d48856ecf931f464` |

Both sha256 values are what the release itself publishes as asset digests, and the workflow asserts
them again at build time (`KSUD_SHA256` / `MANAGER_SHA256`), so a silent swap fails the build.

## `ksud`

Provenance: the release asset `ksud-aarch64-linux-android` of `SukiSU-Ultra/SukiSU-Ultra`
release **`v4.2.1-r2`** (published 2026-10-24). Unlike the previous pin, no CI run had to be quoted:
this release attaches the tool itself.

What it reports about itself (read out of the binary; it cannot be executed on the build host --
it is a bionic-linked aarch64 PIE that needs `liblog.so`):

```
strings ksud | grep 'uapi:'   ->   "4.2.1-r2 (uapi: 6)"
readelf -h                    ->   ELF64, type DYN (PIE), machine 183 (AArch64)
```

The CLI surface the chain depends on is present in this build: `resetprop` (applet-style, i.e.
`ksud resetprop …`), `resetprop --delete`, and `late-load` (called with no arguments: load the
packed module and run the late-load stage scripts). The previous pin was `4.2.1 (uapi: 5)`; the uapi
bump is upstream's own, and both files come from this one release, so the chain's two payloads
speak the same version.

## `sukisu-manager.apk`

Provenance: the same release's asset `SukiSU_v4.2.1-r2_41068-release.apk` -- the plain release
build, **not** the release's `…-release-spoofed.apk` variant (same size, different sha256; the
spoofed build is for version-spoofing installs and is not what the chain installs).

Checked here (no Android SDK on this host, so: package and version from the release's own asset
name plus the APK's resource table, signature from the APK Signing Block):

```
package      com.sukisu.ultra              (resources.arsc)
versionCode  41068                         (asset name; previous pins matched the same way)
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             CN=shirkneko, OU=shirkneko, O=shirkneko, L=guangzhou, ST=gaungdong, C=zh
```

The signer is **identical to the previous pin** (v4.2.1 / 40994), verified by parsing both APKs'
v2 signing blocks, so `pm install -r` on a device that already has the manager stays an in-place
upgrade: 40994 -> 41068, no uninstall, no lost grants.

The release also publishes the `lkm-*` kernel modules and `ksud-*` for other architectures. They are
separate assets and are not part of this pin: the chain ships only the aarch64 `ksud` (which carries
its own module) and the manager APK.

## Why the files are committed instead of fetched

Another repository's release assets are reachable, but a pin that is fetched at build time is not a
pin: the build would change under us whenever upstream re-uploads an asset, and the device-side story
is that the app must carry both files itself (国内网络到 GitHub 不稳). Committing them and asserting
their sha256 keeps the build reproducible. The previous pins
(`prebuilt/sukisu-ultra-4.2.1-40994/` -- official v4.2.1 pair;
`prebuilt/sukisu-ultra-3877-7a1c0438/` -- fork `ci` `ksud` + official v4.2.0 manager;
`prebuilt/sukisu-ultra-4c15fca1/` -- upstream `main@4c15fca1` pair; before that the KernelSU
`v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
