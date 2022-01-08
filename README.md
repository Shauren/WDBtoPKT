# WDBtoPKT

Convert a WDB file to PKT that can be later parsed with [WowPacketParser](https://github.com/TrinityCore/WowPacketParser)

## Building the tool

### Requirements

* CMake 3.8+
* C++17 capable compiler

```
mkdir build
cd build
cmake ..
```

## Usage
`./WDBtoPKT [path_to_wdb.wdb] [path_to_wdb2.wdb]...`

## Supported client versions

Because this tool produces a PKT file to be parsed with WowPacketParser only a handful of client patches are supported

Current list includes all of `9.0.1`, `9.0.2`, `9.0.5`, `9.1.0`, `9.1.5`
