# Mac OS 9 CD image: installation and test evidence (#171)

The Linux release packager stages `PoodleEarth-<version>-<revision>-hfs.iso`
alongside the StuffIt archive and corresponding source. The version comes from
CMake and the revision from Git. The image is a non-bootable ISO-9660/classic HFS
hybrid with 2048-byte optical sectors and an Apple Partition Map. The Macintosh
view contains Poodle Earth (both forks, `APPL` / `PdlE`), ReadMe, two license
parts and DataCredit. It contains no system software.

cdrkit's `genisoimage -hfs -part --macbin` decodes MacBinary into the HFS
catalog and shares payload extents with the ISO view. The ISO view is accessible
with standard ISO readers, while Finder needs the HFS view to copy the
application with its resource fork. The build suppresses cdrkit's empty Desktop
files to keep the image compact. [cdrkit options](https://manpages.debian.org/testing/genisoimage/genisoimage.1.en.html)

ClassicMac's Tools script uses a classic HFS partition in an Apple-partitioned
raw disk image. Its `.iso` name and integration with ClassicMac do not establish
compatibility with other virtual optical drives. A raw APM/HFS alternative would
be more clearly labeled `.img`. The hybrid keeps the `-hfs.iso` suffix and
gives ISO readers a conventional filesystem. No ClassicMac source was copied.
[ClassicMac Tools script](https://github.com/amcchord/ClassicMac/blob/main/scripts/build-guest-cd.sh)

## Attach and copy

In a PowerPC emulator with a working Mac OS 9 guest, attach the image to the
virtual **CD-ROM** drive. For QEMU, add `-cdrom /path/to/PoodleEarth-…-hfs.iso`
to the existing guest command if its CD drive is available. In a GUI, select the
image in the virtual CD drive's media picker. In Finder, open the Poodle Earth
volume, drag **Poodle Earth** to a folder on the writable Macintosh hard disk,
and open the copied application. Use the Finder copy so both forks travel
together. See the [user guide](user-guide.md) for the StuffIt alternative and controls.

## Jack's acceptance of the tested image

On November 19, 2026, Jack reported results for the local PR #172 test image:

- File: `PoodleEarth-PR172-2074034-local-hfs.iso`, 1,210,368 bytes.
- Source: `20740340c939bb5eda5185f5f288650db50850ee`.
- SHA-256: `1f4326ce52904f1f152f1d35ecca958974b7ed5cd5ce13bf1931913efb538021`.
- **Physical Power Mac G4, bare Mac OS 9.2.2:** Jack burned the image on
  Fedora with xorriso 1.5.8.pl02 and reported that the app, files and Get Info
  worked. Memory settings were read-only on CD until the app was copied to the
  hard drive. The native report did not separately specify the launch location.
- **ClassicMac 4.7.0, emulated Power Mac G4, Mac OS 9.2.1:** Jack attached
  the image as a CD, launched the app from the mounted CD, copied it to the guest
  desktop and successfully launched the copy.

These are owner-reported tests, not agent-observed runs. The
[native record](https://github.com/jackwillis/globe/issues/171#issuecomment-7001544331)
and [emulator acceptance and waiver](https://github.com/jackwillis/globe/issues/171#issuecomment-7002974798)
supersede the earlier pending-test statements. Jack approved carrying both tests
forward without retest **only for subsequent text-only changes**. The waiver
excludes changes to the executable, Finder metadata or packaging behavior.
Neither this evidence nor the waiver establishes QEMU/SheepShaver acceptance or
compatibility with every guest OS.

The [v0.1.1 release](https://github.com/jackwillis/globe/releases/tag/v0.1.1) publishes the hybrid ISO alongside StuffIt,
matching source, the release manifest and SHA-256 checksums. Use the published
checksums to verify the release image.

## Build and verify on Linux

From a clean checkout and with the release workflow's Retro68 and Rust images:

```sh
sudo apt-get install podman unar genisoimage faketime hfsutils
tools/build-os9.sh --floppy
GLOBE_RELEASE_STRICT=1 tools/package-release.sh build-os9-floppy
(cd build-os9-floppy/release && sha256sum -c SHA256SUMS)
python3 -m unittest discover -s tests -p test_package_cd.py -v
epoch=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct)}
python3 tools/package_cd.py build-os9-floppy /tmp/poodle-earth-rebuild.iso "$epoch"
cmp build-os9-floppy/release/PoodleEarth-*-hfs.iso /tmp/poodle-earth-rebuild.iso
```

The packager checks the ISO descriptor, partition map, and classic HFS signature,
then uses hfsutils to read back the directory and MacBinary forks independently
of cdrkit. To repeat the structural verification on an existing file, add
`--verify-only` to the Python command and pass that image's path. To inspect the
ISO view separately, run `isoinfo -d -i IMAGE` and `isoinfo -f -i IMAGE`.

The staging dates and cdrkit clock use `SOURCE_DATE_EPOCH` (default: commit
time) in UTC. The packager clears the unused driver descriptor bytes left
uninitialized by cdrkit. CI compares two builds from the same input binary, text and epoch.
Expect identical output within the same Linux toolchain. Cross-version cdrkit
reproducibility and fresh Retro68 binary reproducibility are separate checks.
`RELEASE.json` records revision, epoch, byte counts and SHA-256, and
`SHA256SUMS` includes the CD image.
