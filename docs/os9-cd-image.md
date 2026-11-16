# Mac OS 9 CD image candidate (#171)

The Linux release packager stages `PoodleEarth-<version>-<revision>-hfs.iso`
alongside the StuffIt archive and corresponding source. The version comes from
CMake and the revision from Git. The image is a non-bootable ISO-9660/classic HFS
hybrid with 2048-byte optical sectors and an Apple Partition Map. The Macintosh
view contains Poodle Earth (both forks, `APPL` / `PdlE`), ReadMe, two license
parts and DataCredit. It contains no system software.

cdrkit's `genisoimage -hfs -part --macbin` decodes MacBinary into the HFS
catalog and shares payload extents with the ISO view. The ISO view is accessible
The build suppresses cdrkit’s empty Desktop files to keep the image compact.
with standard ISO readers, while Finder needs the HFS view to copy the
application with its resource fork. [cdrkit options](https://manpages.debian.org/testing/genisoimage/genisoimage.1.en.html)

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
together. Running directly from the disc is outside the build verification (#49).

QEMU, ClassicMac, SheepShaver and native Mac OS 9 mount/copy/launch are pending.
Record emulator version, machine model, guest OS and whether Finder shows the
HFS volume before recording acceptance on #171. Coordinate the published
download text with #129 and the newcomer instructions with #118 after acceptance.

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
time) in UTC. CI compares two builds from the same input binary, text and epoch.
Expect identical output within the same Linux toolchain. Cross-version cdrkit
reproducibility and fresh Retro68 binary reproducibility are separate checks.
`RELEASE.json` records revision, epoch, byte counts and SHA-256, and
`SHA256SUMS` includes the CD image.
