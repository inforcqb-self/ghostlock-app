# Pinned pair: our SukiSU-Ultra fork's `ci` (KernelSU #3877) + upstream's official manager

The two files the build packages into the APK, pinned in the tree -- now with **two different
provenances on purpose**, because the point of this pin is one KernelSU fix and nothing else.

| file | what it is | bytes | sha256 |
|---|---|---|---|
| `ksud` | the standalone `ksud` for aarch64, **built by our fork** (`inforcqb/SukiSU-Ultra`) | 6999648 | `d7fec18cd9c2bc7b6e437797e3ac8898449187b51b46698cc3dc59ffa21b21b6` |
| `sukisu-manager.apk` | the manager app the chain installs with `pm install -r`, **upstream's official build** (`com.sukisu.ultra`) | 16718549 | `d3b07d2638745e79faa765c9bfe9281269414399e0550a13cc186a88e645fbb4` |

## `ksud` — why it comes from our fork

Our fork's branch `ci` is upstream SukiSU `main` (`223897c7`, 2026-10-10) **plus one ported
KernelSU commit**:

```
7a1c0438  sync: port KernelSU's selinux hide fix (#3877) verbatim
kernel/feature/selinux_hide.c   +214 / −125      (blob 459f99f9…)
Source: tiann/KernelSU@a85dcbcfc3b09ae410f31aae124d04f3777fdf1c
```

It was ported verbatim because our copy of that file was byte-identical to upstream's pre-fix
version (blob `033cab8d…` at `6a2f6324`) -- there was no local SukiSU work in it to preserve.

The fork builds it through the `pull_request` path (fork PR #1, `ci` → `main`), which generates a
temporary keystore, builds the LKM against that key, and signs the manager with it -- so the
artifact pair is self-consistent. Provenance: `inforcqb/SukiSU-Ultra` CI run **38119744728**
(`ci`, `pull_request`, 2026-10-11), artifact `ksud-aarch64-linux-android`.

That run's LKM pins **two** manager certificates, and both were checked with `apksigner` against
the actual APKs:

```
947ae944…  = upstream's official SukiSU manager   ← the one shipped here
78e5c47c…  = that run's temporary PR key          ← not shipped
```

So the `ksud` from the fork **accepts the official manager**, which is why the manager here stays
upstream's official build: no re-signing, no uninstall, and the device keeps the manager it already
has. (If upstream ever drops the first pin, this pair stops being valid -- the two files must be
checked together.)

## `sukisu-manager.apk` — unchanged, upstream's

Provenance: `SukiSU-Ultra/SukiSU-Ultra` CI run **37934517573**, branch `main`, head `4c15fca1`,
2026-10-09; artifact `Manager`, file name `SukiSU_v4.2.0_40965-release.apk`. The manager is *not*
what this pin is about -- the property and `late-load` work is done by `ksud` -- and keeping the
official one keeps the device-side install a no-op upgrade.

## Why the files are committed instead of fetched

Another repository's CI artifacts need a token this workflow does not have, so both are pinned and
their sha256s are asserted in `.github/workflows/build.yml`: a silent swap fails the build. The
previous pin (`prebuilt/sukisu-ultra-4c15fca1/`, upstream `main@4c15fca1`) is in git history.

## Licence

GPL-3.0 (SukiSU-Ultra / KernelSU family), for both files. See
`app/src/main/jni/THIRD_PARTY_NOTICES.md`.
