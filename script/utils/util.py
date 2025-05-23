# Copyright 2023 Seoul National University
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from genericpath import isfile
import os
import argparse
import platform as plf
import shlex
import shutil
import subprocess
import multiprocessing
from .docker import run_cmd_docker

PLATFORM = {
    "linux": "linux_x86_64",
    "windows": "windows",
    "android": "android_arm64",
}


def is_windows():
    return plf.system() == "Windows"


def is_linux():
    return plf.system() == "Linux"


def is_cygwin():
    return plf.system().startswith("CYGWIN_NT")


def get_platform():
    return plf.system().lower()


def canon_path(path):
    if is_windows() or is_cygwin():
        return path.replace('\\', '/')
    return path


def run_cmd(cmd):
    args = shlex.split(cmd)
    subprocess.call(args, cwd=os.getcwd())


def copy(src, dst):
    subprocess.call(['mkdir', '-p', f'{os.path.normpath(dst)}'])
    # append filename to dst directory

    dst = canon_path(os.path.join(dst, os.path.basename(src)))

    if os.path.isdir(src):
        shutil.copytree(src, dst, ignore_dangling_symlinks=True, symlinks=True)
    else:
        try:
            shutil.copy(src, dst)
        except FileNotFoundError:
            print(dst)
            shutil.copy(src + ".exe", dst + ".exe")


def get_bazel_options(
        debug: bool,
        trace: bool, 
        platform: str,
        backend: str,
        nvidia_opencl: bool):
    opt = ""
    opt += "-c " + ("dbg" if debug else "opt") + " "
    opt += "--strip " + ("never" if debug else "always") + " "
    opt += f"--config {PLATFORM[platform]}" + ("" if backend == "none" else f" --config {backend}") + ("" if trace == False else " --config trace") + ("" if nvidia_opencl == False else " --define gpu=nvidia-opencl --copt=-DCL_TARGET_OPENCL_VERSION=300 --copt=-DCL_DELEGATE_NO_GL --copt=-DEGL_NO_X11") + " "
    return opt


def make_cmd(
        build_only: bool, 
        debug: bool,
        trace: bool, 
        platform: str, 
        backend: str, 
        target: str,
        nvidia_opencl: bool
    ):
    cmd = "bazel" + " "
    cmd += ("build" if build_only else "test") + " "
    cmd += get_bazel_options(debug, trace, platform, backend, nvidia_opencl)
    cmd += target
    return cmd


def get_dst_path(base_dir, target_platform, debug):
    build = 'debug' if debug else 'release'
    path = os.path.join(base_dir, target_platform, build)
    return canon_path(path)


ANDROID_BASE = '/data/local/tmp/'


def push_to_android(src, dst, device_connection_flag):
    run_cmd(f'adb {device_connection_flag} push {src} {ANDROID_BASE}{dst}')


def run_binary_android(basepath, path, device_connection_flag, option=''):
    run_cmd(f'adb {device_connection_flag} shell chmod 777 {ANDROID_BASE}{basepath}{path}')
    # cd & run -- to preserve the relative relation of the binary
    run_cmd(
        f'adb {device_connection_flag} shell cd {ANDROID_BASE}{basepath} && ./{path} {option}')


def get_argument_parser(desc: str):
    parser = argparse.ArgumentParser(description=desc)
    parser.add_argument('-B', '--backend', type=str,
                        default='tflite', help='Backend <tflite|none>')
    parser.add_argument('-android', action="store_true", default=False,
                        help='Test on Android (with adb)')
    parser.add_argument('-docker', action="store_true", default=False,
                        help='Compile / Pull cross-compiled binaries for android from docker (assuming that the current docker engine has devcontainer built with a /.devcontainer')
    parser.add_argument('-s', '--ssh', required=False, default=None, help="SSH host name (e.g. dev@ssh.band.org)")
    parser.add_argument('-t', '--trace', action="store_true", default=True, help='Build with trace (default = True)')
    parser.add_argument('-d', '--debug', action="store_true", default=False,
                        help='Build debug (default = release)')
    parser.add_argument('-r', '--rebuild', action="store_true", default=False,
                        help='Re-build test target, only affects to android ')
    parser.add_argument('-b', '--build', action="store_true", default=False,
                        help='Build only, only affects to local (default = false)')
    parser.add_argument('-opencl', action="store_true", default=False,
                        help='Test on Linux & GPU with OpenCL Support')
    return parser


def clean_bazel(docker):
    if docker:
        run_cmd_docker('bazel clean')
    else:
        run_cmd('bazel clean')

def get_adb_devices():
    result = subprocess.check_output(['adb', 'devices']).decode('utf-8')
    lines = result.strip().split('\n')[1:]
    device_connection_flags = []
    for line in lines:
        if line.strip():
            device_info = line.split()
            device_id = device_info[0]
            if ':' in device_id:
                # Wireless device detected
                device_connection_flags.append(f"-s {device_id}")
            else:
                # USB device detected
                device_connection_flags.append("-d")
    return device_connection_flags