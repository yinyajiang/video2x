#!/usr/bin/env python3

import subprocess
import os
import argparse


def run_command(cmd, cwd=os.path.dirname(os.path.abspath(__file__))):
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


def init_submodules():
    run_command(['git', 'submodule', 'status'])
    run_command(['git', 'submodule', 'sync', '--recursive'])
    run_command(['git', 'submodule', 'update', '--init', '--recursive'])


def build(rebuild=False):
    rebuild = rebuild or not os.path.exists('build/Makefile')
    if rebuild:
        run_command(['rm', '-rf', 'build'])
        run_command('mkdir build && cd build && cmake ..')
        
    run_command('cd build && make -j4')
    run_command(['cp', '-r', 'models', 'build'])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--rebuild', default=False, action='store_true')
    args = parser.parse_args()
    init_submodules()
    build(args.rebuild)


if __name__ == "__main__":
    main()
