import os
import pathlib
import sys
import subprocess
import platform
import shutil

cur_dir = pathlib.Path(os.path.dirname(os.path.abspath(__file__)))

def main():
    args = sys.argv[1:]
    output_dir = None
    for i, arg in enumerate(args):
        if arg == '-o':
            output_dir = pathlib.Path(args[i + 1]).resolve()
            args.pop(i)
            args.pop(i)
            break

    subprocess.run(['python' if platform.system() == 'Windows' else 'python3', cur_dir / 'tools' / 'libimage2x' / 'build.py'] + args).check_returncode()
    print("build success")
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
        if platform.system() == 'Windows':
            release_dir = cur_dir / 'tools' / 'libimage2x' / 'build' / 'Release'
            # 拷贝 *.dll 和 *.exe 到 output_dir
            for file in release_dir.glob('*.dll'):
                shutil.copy(file, output_dir / file.name)
            for file in release_dir.glob('*.exe'):
                shutil.copy(file, output_dir / file.name)
            print("copy dll and exe to:", output_dir)
        else:
            release_dir =  cur_dir / 'tools' / 'libimage2x' / 'build'
            for file in release_dir.glob('*.dylib'):
                shutil.copy(file, output_dir / file.name)
            shutil.copy(release_dir / 'image2x', output_dir / 'image2x')
            print("copy dylib to:", output_dir)
    else:
        print("no output dir")
    
if __name__ == "__main__":
    main()