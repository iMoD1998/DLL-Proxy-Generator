# DLL Proxy Generator

Creates resources for DLL interception/hijacking through module exports.

### Building
Currently only builds with Visual Studio 2019

Open .sln in Visual Studio
Click Build

### Usage
```
USAGE:
  DLL Proxy Generator.exe [-?|-h|--help] [-v|--verbose] [-p|--visualstudio] [-d|--def] [-f|--forward <NEWDLLNAME>] [-n|--vsname <PROJNAME>] [-o|--out <OUTDIR>] <DLLPATH>

Display usage information.

OPTIONS, ARGUMENTS:
  -?, -h, --help
  -v, --verbose           Show infomation about exports
  -p, --visualstudio      Generate Visual Studio project
  -d, --def               Prefer def file over #pragma
  -f, --forward <NEWDLLNAME>
                          Use export forwarding to forward exports to old DLL with new name
  -n, --vsname <PROJNAME> Name for visual studio project
  -o, --out <OUTDIR>      Out directory for files
  <DLLPATH>               Path of the DLL to get exports from
```
