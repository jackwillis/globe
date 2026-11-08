// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
//! Builds a StuffIt (.sit) archive on a host without Mac forks: applications come from MacBinary
//! files (both forks, type and creator), documents from plain text (LF becomes CR, type TEXT).
//! Every entry gets the same date, so the same inputs give the same archive.
//!
//!   sit-pack -o out.sit [-m 0|13] [--date UNIX_SECONDS] --mac NAME=FILE.bin --text NAME=FILE ...
use std::{env, fs, process};
use stuffit::{unix_time_to_mac, SitArchive, SitEntry};

fn fail(msg: &str) -> ! {
    eprintln!("sit-pack: {msg}");
    process::exit(1)
}

fn be32(b: &[u8]) -> usize {
    u32::from_be_bytes([b[0], b[1], b[2], b[3]]) as usize
}

fn from_macbinary(name: &str, bytes: &[u8], date: u32) -> SitEntry {
    if bytes.len() < 128 || bytes[0] != 0 || bytes[74] != 0 || bytes[82] != 0 {
        fail(&format!("{name}: not a MacBinary file"));
    }
    let (dlen, rlen) = (be32(&bytes[83..87]), be32(&bytes[87..91]));
    let rstart = 128 + dlen.div_ceil(128) * 128;
    if bytes.len() < rstart + rlen {
        fail(&format!("{name}: MacBinary forks are truncated"));
    }
    SitEntry {
        name: name.to_string(),
        data_fork: bytes[128..128 + dlen].to_vec(),
        resource_fork: bytes[rstart..rstart + rlen].to_vec(),
        file_type: bytes[65..69].try_into().unwrap(),
        creator: bytes[69..73].try_into().unwrap(),
        finder_flags: u16::from_be_bytes([bytes[73], bytes[101]]) & 0xfeff, // not "initialized"
        creation_date: date,
        modification_date: date,
        ..Default::default()
    }
}

// stuffit-rs 0.4.0 writes a constant in the archive header's CRC slot and checksums only the first
// 32 bytes of each entry header; readers that verify them (StuffIt Expander 6.0 calls the result
// "broken") expect the ARC CRC-16 over the whole header with its own CRC field zeroed. Checked
// against a real StuffIt 5 archive: archive header = bytes 0..114 (CRC at 98), entry header =
// header_size bytes from the entry start (CRC at +32). Flat archives only, which is all we write.
fn arc_crc16(data: &[u8]) -> u16 {
    let mut crc = 0u16;
    for &b in data {
        crc ^= u16::from(b);
        for _ in 0..8 {
            crc = if crc & 1 != 0 { (crc >> 1) ^ 0xA001 } else { crc >> 1 };
        }
    }
    crc
}

fn set_crc(data: &mut [u8], start: usize, len: usize, at: usize) {
    data[at..at + 2].copy_from_slice(&[0, 0]);
    let crc = arc_crc16(&data[start..start + len]);
    data[at..at + 2].copy_from_slice(&crc.to_be_bytes());
}

fn fix_header_crcs(data: &mut [u8]) {
    let mut offset = be32(&data[88..92]);
    while offset != 0 {
        if data[offset..offset + 4] != [0xa5; 4] {
            fail("unexpected entry header while fixing CRCs");
        }
        let size = usize::from(u16::from_be_bytes([data[offset + 6], data[offset + 7]]));
        // Files also carry a CRC (at +2 of the block after the name) over that whole block: 36
        // bytes, plus 14 when there is a resource fork. stuffit-rs leaves it zero.
        if data[offset + 9] & 0x40 == 0 {
            let block = offset + size;
            let len = if data[block + 1] & 1 != 0 { 50 } else { 36 };
            set_crc(data, block, len, block + 2);
        }
        set_crc(data, offset, size, offset + 32);
        offset = be32(&data[offset + 22..offset + 26]);
    }
    set_crc(data, 0, 114, 98); // last: it does not cover the entries
}

fn main() {
    let args: Vec<String> = env::args().skip(1).collect();
    let (mut out, mut method) = (None, 13u8);
    let mut date_unix = 0i64;
    let mut entries: Vec<(bool, String, String)> = Vec::new();
    let mut i = 0;
    while i < args.len() {
        let value = args.get(i + 1).cloned().unwrap_or_else(|| fail("missing value"));
        match args[i].as_str() {
            "-o" => out = Some(value),
            "-m" => method = value.parse().unwrap_or_else(|_| fail("bad method")),
            "--date" => date_unix = value.parse().unwrap_or_else(|_| fail("bad date")),
            flag @ ("--mac" | "--text") => {
                let (name, path) = value.split_once('=').unwrap_or_else(|| fail("want NAME=FILE"));
                entries.push((flag == "--mac", name.to_string(), path.to_string()));
            }
            other => fail(&format!("unknown argument {other}")),
        }
        i += 2;
    }
    if method != 0 && method != 13 {
        fail("method must be 0 or 13");
    }
    let out = out.unwrap_or_else(|| fail("-o is required"));
    let date = unix_time_to_mac(date_unix);
    let mut archive = SitArchive::new();
    for (mac, name, path) in &entries {
        let bytes = fs::read(path).unwrap_or_else(|e| fail(&format!("{path}: {e}")));
        archive.add_entry(if *mac {
            from_macbinary(name, &bytes, date)
        } else {
            SitEntry {
                name: name.clone(),
                data_fork: bytes.iter().map(|&b| if b == b'\n' { b'\r' } else { b }).collect(),
                file_type: *b"TEXT",
                creator: *b"ttxt",
                creation_date: date,
                modification_date: date,
                ..Default::default()
            }
        });
    }
    let mut data = archive.serialize_with_method(method).unwrap_or_else(|e| fail(&format!("{e:?}")));
    fix_header_crcs(&mut data);
    fs::write(&out, data).unwrap_or_else(|e| fail(&format!("{out}: {e}")));
}
