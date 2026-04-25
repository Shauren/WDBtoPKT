# WDBtoPKT

Convert a WDB file to PKT that can be later parsed with [WowPacketParser](https://github.com/TrinityCore/WowPacketParser)

## Building the tool

### Requirements

* CMake 3.24
* Visual Studio 2026
* .NET 10.0 SDK

```
mkdir build
cd build
cmake ..
```

## Usage
`./WDBtoPKTRunner [path_to_wdb.wdb] [path_to_wdb2.wdb]...`

## Supported client versions

Because this tool produces a PKT file to be parsed with WowPacketParser only a handful of client patches are supported

Current list includes all of `9.x`, `10.x`, `11.x` and `12.x`
