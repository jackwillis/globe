// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "optional_atlas_file.hpp"
#include <Files.h>
#include <Processes.h>

namespace {

bool read_data_fork(void* context, std::span<std::uint8_t> bytes) {
    const auto ref = *static_cast<const short*>(context);
    const auto expected = static_cast<long>(bytes.size());
    long actual = expected;
    return FSRead(ref, &actual, bytes.data()) == noErr && actual == expected;
}

} // namespace

globe::StartupAtlas load_optional_atlas() {
    ProcessSerialNumber process{};
    FSSpec app{};
    ProcessInfoRec info{};
    info.processInfoLength = sizeof(info);
    info.processAppSpec = &app;
    if (GetCurrentProcess(&process) != noErr || GetProcessInformation(&process, &info) != noErr)
        return {{}, {}, "Application folder unavailable"};

    unsigned char data_name[] = {4, 'D', 'a', 't', 'a'};
    CInfoPBRec catalog{};
    catalog.dirInfo.ioNamePtr = data_name;
    catalog.dirInfo.ioVRefNum = app.vRefNum;
    catalog.dirInfo.ioDrDirID = app.parID;
    catalog.dirInfo.ioFDirIndex = 0; // named child lookup
    if (PBGetCatInfoSync(&catalog) != noErr || (catalog.dirInfo.ioFlAttrib & 0x10) == 0)
        return {{}, {}, "Data folder unavailable"};

    const unsigned char atlas_name[] = {10, 'a', 't', 'l', 'a', 's', '.', 'g', 'a', 't', 'l'};
    FSSpec spec{};
    if (FSMakeFSSpec(app.vRefNum, catalog.dirInfo.ioDrDirID, atlas_name, &spec) != noErr)
        return {{}, {}, "Data/atlas.gatl unavailable"};
    short ref = 0;
    if (FSpOpenDF(&spec, fsRdPerm, &ref) != noErr)
        return {{}, {}, "Cannot open atlas"};
    long size = 0;
    auto result = GetEOF(ref, &size) == noErr
                      ? globe::read_startup_atlas(size, &ref, read_data_fork)
                      : globe::StartupAtlas{{}, {}, "Cannot determine atlas size"};
    if (FSClose(ref) != noErr && result.scene)
        return {{}, {}, "Cannot close atlas"};
    return result;
}
