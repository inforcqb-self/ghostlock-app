# Pinned pair: upstream `main` CI build `4.2.1-r2-29-g080ba098` + its release-signed manager

The two files the build packages into the APK, pinned in the tree. Both come from the **same upstream
CI run**, so they are self-consistent; the manager is signed with the real release key, not a
temporary PR key.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | standalone `ksud` for aarch64, built by upstream CI from `main@080ba098` | 7857184 | `0a6529dd5e59bd1b2944e9715e3ac10208c60fe1ae3721f7745dad4206ccd4b5` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 18842065 | `c13f4884b47e6648fb116ed49647eaa544ee54d6c30467b933164bdccf25aa5e` |

Provenance: `SukiSU-Ultra/SukiSU-Ultra` **run 40585558597** (`Build Manager`, push to `main`,
2026-10-27, head `080ba098902c1ea8f09748536f9d920329e2f7ce` -- which was `main`'s tip at the time);
artifacts `ksud-aarch64-linux-android`, `Manager`, `aarch64-android13-5.15-lkm`.

## Why a CI build and not a release

Because `#3945` is not in any release yet. The selinux-hide timeline upstream and here:

```
2026-10-10  tiann/KernelSU@a85dcbcf   fix(kernel): fix time side-channel detection in selinux hide (#3877)
2026-10-13  SukiSU b445d0888          the same fix, ported verbatim (same blob 459f99f9…, +214/−125)
2026-10-15  release v4.2.1 (40994)    contains #3877
2026-10-22  release v4.2.1-r1 (41049) contains #3877
2026-10-24  release v4.2.1-r2 (41068) contains #3877          <- published 10:52
2026-10-24  tiann/KernelSU@528bafc56   kernel: remove useless code (#3945)      follow-up, 12:55
2026-10-24  SukiSU 15b64d095          the same follow-up, ported
```

So `v4.2.1-r2` misses `#3945` by two hours, and `main` is the only place with both. After the two
ports, `kernel/feature/selinux_hide.c` on SukiSU's `main` is blob `8f23888b…` (40 161 B) --
**the identical blob to `tiann/KernelSU`'s `main`** -- so SukiSU's tree and upstream's agree on this
file byte for byte, at this commit.

## `ksud`

```
$ ksud -V                       ->  ksud 4.2.1-r2-29-g080ba098 (uapi: 6)
ELF64, class 2, machine 183 (AArch64)
```

`git describe` is baked in, which is the cheapest proof of which commit it was built from: `29`
commits after tag `v4.2.1-r2`, at `080ba098`. `resetprop --delete` is present (unchanged since
v4.2.1, see `userspace/ksud/src/resetprop.rs`), and `late-load` keeps the plain no-flag form the
chain uses.

## `sukisu-manager.apk`

Checked with the local SDK's `apksigner` / `aapt2` (the artifact ships as
`SukiSU_v4.2.1-r2_41097-release.apk`):

```
package      com.sukisu.ultra
versionCode  41097          versionName  v4.2.1-r2
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             (C=zh, ST=gaungdong, L=guangzhou, O=shirkneko, OU=shirkneko, CN=shirkneko)
```

Same signer as every pin before it, so `pm install -r` stays an in-place upgrade. The `versionName`
still says `v4.2.1-r2` while the code is 41097 rather than the release's 41068: the CI build bumps
the code per run, the release tag does not -- both were built from this same `v4.2.1-r2` line.

Cross-check of the kernel side, from the same run's `aarch64-android13-5.15-lkm` artifact (665 960 B,
sha256 `b11eb59a6e48c743c44bcf74792f5bfbf292735997324c137845e114d7d18f98`; it is what this `ksud`
embeds, and it is *not* shipped separately): it pins exactly **one** manager certificate, the
official `947ae944…` above, and no temporary key.

## Why the files are committed instead of fetched

Upstream's CI artifacts need a token this workflow does not have (and expire), so both are pinned and
their sha256s are asserted in `.github/workflows/build.yml`: a silent swap fails the build. The
previous pins (`prebuilt/sukisu-ultra-4.2.1-40994/` -- official v4.2.1 release pair;
`prebuilt/sukisu-ultra-3877-7a1c0438/` -- fork `ci` `ksud` + official v4.2.0 manager;
`prebuilt/sukisu-ultra-4c15fca1/` -- upstream `main@4c15fca1` pair; before that the KernelSU
`v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
