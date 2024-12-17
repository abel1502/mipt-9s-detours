from __future__ import annotations
import typing
import subprocess
import argparse
import pathlib
import io
import sys


parser = argparse.ArgumentParser()
parser.add_argument("dll", type=str)
parser.add_argument("--lib", type=str, default=None, required=False)


def main(args):
    dll = pathlib.Path(args.dll)
    lib = pathlib.Path(args.lib) if args.lib is not None else dll.with_suffix(".lib")
    defs = dll.with_suffix(".def")
    
    proc = subprocess.Popen(f"wsl -- gendef {dll.as_posix()}", shell=True, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL)
    proc.communicate()
    
    proc = subprocess.Popen(["cmd.exe"], stdin=subprocess.PIPE, stdout=subprocess.DEVNULL)
    proc.communicate(
        f"""
        call "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\VsDevCmd.bat" -startdir=none -arch=x64 -host_arch=x64
        lib /def:{defs} /machine:x64 /out:{lib}
        """.encode(),
    )

if __name__ == "__main__":
    main(parser.parse_args())

