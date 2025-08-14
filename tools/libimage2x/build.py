#!/usr/bin/env python3

import subprocess
import os
import argparse
import shutil
import platform
from pathlib import Path


def cur_dir(path=''):
    return (Path(__file__).parent / path).resolve()


def run_command(cmd, cwd=cur_dir()):
    print(f"run: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    if isinstance(cmd, str):
        subprocess.run(
            cmd, 
            shell=True, 
            cwd=cwd, 
        ).check_returncode()
    else:
        subprocess.run(
            cmd, 
            cwd=cwd, 
        ).check_returncode()


def build(rebuild=False):
    build_dir = cur_dir('build')
    if not rebuild:
        if platform.system() == 'Windows':
            rebuild = not build_dir.joinpath('CMakeCache.txt').exists()
        else:
            rebuild = not build_dir.joinpath('Makefile').exists()
            
    if rebuild:
        if build_dir.exists():
            shutil.rmtree(build_dir)
        os.makedirs(build_dir)
        
        # Use Visual Studio 2022 on Windows
        if platform.system() == 'Windows':
            run_command(['cmake', '..', '-G', 'Visual Studio 17 2022'], cwd=build_dir)
        else:
            run_command(['cmake', '..'], cwd=build_dir)
        
    if platform.system() == 'Windows':
        run_command(['cmake', '--build', '.', '--config', 'Release', '--parallel', '4'], cwd=build_dir)
        for dll_file in [
            build_dir / 'third_party/boost/libs/program_options/Release/boost_program_options-vc143-mt-x64-1_86.dll',
            build_dir / 'third_party/librealesrgan_ncnn_vulkan/Release/librealesrgan-ncnn-vulkan.dll',
            build_dir / 'third_party/librife_ncnn_vulkan/Release/librife-ncnn-vulkan.dll',
            build_dir / 'third_party/librealcugan_ncnn_vulkan/Release/librealcugan-ncnn-vulkan.dll',
            cur_dir() / '../../third_party/ncnn-shared/x64/bin/ncnn.dll',
        ]:
            shutil.copyfile(dll_file, build_dir / 'Release' / dll_file.name)
    else:
        run_command('cd build && make -j4')
    
    src_models_dir = cur_dir('../../models')
    if platform.system() == 'Windows':
        dst_models_dir = build_dir / 'Release' / 'models'
    else:
        dst_models_dir = build_dir / 'models'
    if not dst_models_dir.exists():
        shutil.copytree(src_models_dir, dst_models_dir)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--rebuild', default=False, action='store_true')
    args = parser.parse_args()
    build(args.rebuild)


if __name__ == "__main__":
    main()
