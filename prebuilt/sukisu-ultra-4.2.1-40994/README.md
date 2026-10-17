# Pinned pair: upstream's official SukiSU-Ultra **v4.2.1** (40994) `ksud` + manager

The two files the build packages into the APK, pinned in the tree. Both now come from the **same
official stable release**, so the pair is self-consistent by construction -- no fork build, and no
temporary PR key anywhere in it.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | the standalone `ksud` for aarch64, from the `v4.2.1` tag's CI build | 7162112 | `3dcbf20265743b094111c7f41022206c8434affb8f6904322428cc94460ce02e` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r` (`com.sukisu.ultra`) | 17173205 | `c7f69280cf468e8415c585ff31f963d0147386831217a1e89b822afddc5f40ed` |

## `ksud`

Provenance: `SukiSU-Ultra/SukiSU-Ultra` CI run **38704356559** (`Release`, tag `v4.2.1`, head
`fd2a8515`, 2026-10-15); artifact `ksud-aarch64-linux-android` (4 470 902 B compressed,
7 162 112 B unpacked). Every build job in that run was green -- this `ksud`, all four LKM families
and the manager; only the final `release` job failed, which is why the APK asset exists separately
from the job that normally attaches it.

What it reports about itself:

```
strings ksud | grep 'uapi:'   ->   "4.2.1 (uapi: 5)"     (VERSION_NAME)
ELF64, class 2, machine 183 (AArch64)
```

Run on the device (PJA110, pushed to /data/local/tmp and removed again):

```
$ ksud -V        ->  ksud 4.2.1 (uapi: 5)
$ ksud late-load --help
  --magica [<MAGICA>]            Use adb root to execute late-load for jailbreaking by Magica
  --allow-shell                  Pass allow_shell=1 when loading kernelsu.ko
  --post-magica                  Restore adb properties after magica late-load
  --kmi <KMI>                    Specify kernel KMI version instead of auto-detection
  --package-name <PACKAGE_NAME>  manager package name [default: com.sukisu.ultra]
  --spoof-release / --spoof-version …
```

So the plain `ksud late-load` the chain runs is still the no-flag form (embedded LKM + stage
scripts), and its default manager package is the bundled one.

One detail worth knowing when reading device-side logs: `build.rs` bakes
`VERSION_CODE = 40000 - 2815 + git rev-list --count`, and that count differs slightly between the
two jobs of the same release run -- this `ksud` carries **40992**, the manager APK carries
**40994**. That is upstream's own shipped combination (both are artifacts of run 38704356559), not
a mismatch introduced by this pin.

The KernelSU #3877 selinux-hide fix that the previous pin needed a fork PR for is now **upstream
stable**: `kernel/feature/selinux_hide.c` at `v4.2.1` is blob `459f99f9…`, the post-fix content the
fork's `ci` branch had ported (it used to be `033cab8d…`, the pre-fix blob at `6a2f6324`). So the
fork is no longer in the picture; `inforcqb/SukiSU-Ultra`'s `ci` / PR #1 stays as a record, not as a
source.

`resetprop` accepts `--delete` (`-d`) at this tag -- `userspace/ksud/src/resetprop.rs` has the flag
and `--delete <name>` removes the property. That is what the chain's closing step uses
(`ksud resetprop --delete service.adb.tcp.port`).

`late-load` gains an optional `--restore-adb-props` flag at this tag (and `magica` support inside
`ksud` itself); called with no arguments it behaves as before -- load the embedded LKM, run the
late-load stage scripts -- which is the form the chain uses.

## `sukisu-manager.apk`

Provenance: the same tag's published asset, `SukiSU_v4.2.1_40994-release.apk` (release `v4.2.1`,
published 2026-10-15). Checked with the local SDK's `apksigner` / `aapt2`:

```
package      com.sukisu.ultra
versionCode  40994          versionName  v4.2.1
signer       SHA-256 947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef
             (C=zh, ST=gaungdong, L=guangzhou, O=shirkneko, OU=shirkneko, CN=shirkneko)
```

That is the same signer the previous pin (v4.2.0, 40965) carried, so `pm install -r` on the device
stays an in-place upgrade: 40965 -> 40994, no uninstall, no lost grants.

The `aarch64-android13-5.15-lkm` module from the same run (438 560 B, sha256 `4ae58774…`, i.e. the
copy embedded in this `ksud`) pins exactly **one** manager certificate -- the official one above --
and carries no temporary PR key. (The fork build it replaces pinned two, one of which was a
throwaway.)

## Why the files are committed instead of fetched

Another repository's CI artifacts need a token this workflow does not have, so both are pinned and
their sha256s are asserted in `.github/workflows/build.yml`: a silent swap fails the build. The
previous pins (`prebuilt/sukisu-ultra-3877-7a1c0438/` -- fork `ci` `ksud` + official v4.2.0 manager;
`prebuilt/sukisu-ultra-4c15fca1/` -- upstream `main@4c15fca1` pair; before that the KernelSU
`v3.3.0-69` build) are in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
