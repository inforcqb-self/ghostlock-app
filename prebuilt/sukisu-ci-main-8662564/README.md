# Pinned pair: upstream `main` CI build `4.3.0-r2-13-g86625649` + its release-signed manager

The two files the build packages into the APK, pinned in the tree. Both come from the **same upstream
`main` CI run**, so they are self-consistent; the manager is signed with the real release key, not a
temporary key.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | standalone `ksud` for aarch64, built by upstream CI from `main@8662564` | 7877344 | `361a880d63702c09ae2ad52d277e17272bbe5eafce1cab6812ec5fd0f714dab5` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 20282847 | `fd3c072df508e71975976f891a4240fccd4a9937422a426358b7fa9bc6ac7964` |

## Why a CI build instead of the newest release

Because the newest release is one commit behind on the file this pin line tracks.
`v4.3.0-r2` (2026-11-05, code 41150) carries the selinux-hide line through #3936
(`kernel/feature/selinux_hide.c` = blob `2a41b805…`). Upstream then merged

```
2026-11-06  tiann/KernelSU@019a3c010  refactor(kernel): Mark original type datum to save symtab search (#3969)
2026-11-06  SukiSU e17a48a7d         the same, ported
```

which is in SukiSU `main` but **not in any release yet**, so `main`'s file is blob `385b415a…`
(41 166 B) - the identical blob to `tiann/KernelSU`'s `main` today. This pin takes `main` so the
app carries that commit; the trade is that the manager's versionCode is a CI counter (41163) rather
than a release's, which does not matter for `pm install -r` because the signer is unchanged.

## Provenance

`SukiSU-Ultra/SukiSU-Ultra` run **42467362341** (`Build Manager`, push to `main`, 2026-11-08,
head `866256499aa11d223a8ad154db53d70811f79d9b`, which was `main`'s tip at the time); artifacts
`ksud-aarch64-linux-android`, `Manager`, `aarch64-android13-5.15-lkm`.

Ancestry checked against the release line before pinning (all `behind=0`): `b445d0888` (#3877, the
selinux-hide timing side-channel fix), `15b64d095` (#3945), `e67b14e6b` (#3936, the v4.3.0 tag) and
`e17a48a7d` (#3969).

What `ksud` reports about itself:

```
$ ksud -V                       ->  ksud 4.3.0-r2-13-g86625649 (uapi: 6)
ELF64, class 2, machine 183 (AArch64)
```

`git describe` is baked in, which is the cheapest proof of the build's commit: 13 commits past tag
`v4.3.0-r2`, at `86625649`.

The two ksud behaviours the chain depends on are present here as well: `resetprop --delete` (`-d`)
and the **no-flag** `late-load` form (its `--magica/--allow-shell/--post-magica/--kmi` flags stay
optional), so the chain's `ksud resetprop …` and plain `ksud late-load` are unchanged.

## `sukisu-manager.apk`

Checked with the local SDK's `apksigner` / `aapt2` (the artifact ships as
`SukiSU_v4.3.0-r2_41163-release.apk`):

```
package      com.sukisu.ultra
versionCode  41163          versionName  v4.3.0-r2
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             (C=zh, ST=gaungdong, L=guangzhou, O=shirkneko, OU=shirkneko, CN=shirkneko)
```

Same signer as every previous pin, so on a device that already has the manager `pm install -r` is an
in-place upgrade (41150 -> 41163) -- no uninstall, no lost grants. The `versionName` still says
`v4.3.0-r2` because the tree is on that tag's line; the CI counter is what moves.

Cross-check of the kernel side, from the same run's `aarch64-android13-5.15-lkm` artifact (676 192 B,
sha256 `d7fcbd72437d14fe6159fc0b80228f13962bed8337129a0f46662e1c30f0c1e2`; it is what this `ksud`
embeds and is not shipped separately): it pins exactly **one** manager certificate, the official
`947ae944…` above, and no temporary key.

## Why the files are committed instead of fetched

Upstream's CI artifacts need a token this workflow does not have (and expire), so both are pinned and
their sha256s are asserted in `.github/workflows/build.yml`: a silent swap fails the build. The
previous pins (`prebuilt/sukisu-ultra-4.3.0-r2-41150/` -- official v4.3.0-r2 release;
`prebuilt/sukisu-ultra-4.3.0-41100/`; `prebuilt/sukisu-ci-main-080ba098/`;
`prebuilt/sukisu-ultra-4.2.1-40994/`; `prebuilt/sukisu-ultra-3877-7a1c0438/`;
`prebuilt/sukisu-ultra-4c15fca1/`; before that the KernelSU `v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
