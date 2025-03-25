#!/usr/bin/env python3 -u
import argparse
import glob
import os
import platform
import re
import shutil
import string
import subprocess
from pathlib import Path

import pygit2

# Script location matters, cwd does not
script_path = Path(__file__)
script_dir = script_path.parent

# Git version
repo = pygit2.Repository(path='.')
git_version = repo.describe(pattern='v*')
git_branch = repo.head.shorthand

juce_module_name = 'indiekey_juce'


def verify_architecture(path: Path, expected_architectures: [string]):
    """
    Verifies given file to contain all expected architectures. Throws an exception is expectation is not met.
    :param path: The path of the file to check.
    :param expected_architectures: The expected architectures.
    """

    for arch in expected_architectures:
        try:
            subprocess.run(['lipo', path, '-verify_arch', arch], check=True)
        except subprocess.CalledProcessError as e:
            raise Exception('File "' + str(path) + '" does not contain code for architecture ' + arch)

    print('Verified "' + str(path) + '" to contain code for arch(s): [' + ', '.join(expected_architectures) + ']')


def lipo(input_base_path_x86_64: Path, input_base_path_arm64: Path, output_base_path: Path, filename: Path):
    """
    Lipos binaries from two different places together.
    :param input_base_path_x86_64: The path where to find the x86_64 binary.
    :param input_base_path_arm64: The path where to find the arm64 binary.
    :param output_base_path: The path where to put the universal binary.
    :param filename: The filename of the binaries to lipo, relative to the base paths.
    :return:
    """

    x86_64_file_path = input_base_path_x86_64 / filename
    arm64_file_path = input_base_path_arm64 / filename
    output_file_path = output_base_path / filename

    subprocess.run(['lipo', '-create', x86_64_file_path, arm64_file_path, '-output', output_file_path], check=True)

    verify_architecture(output_file_path, ['x86_64', 'arm64'])


def lipo_glob(input_base_path_x86_64: Path, input_base_path_arm64: Path, output_base_path: Path, glob_pattern):
    """
    Lipos all glob matched binaries together. This requires equal matches for both base directories, otherwise this
    function will raise an exception.
    :param input_base_path_x86_64: The path where to find the x86_64 binary.
    :param input_base_path_arm64: The path where to find the arm64 binary.
    :param output_base_path: The path where to put the universal binary.
    :param glob_pattern: The pattern to match against.
    """

    matches_x86_64 = glob.glob(glob_pattern, root_dir=input_base_path_x86_64)
    matches_arm64 = glob.glob(glob_pattern, root_dir=input_base_path_arm64)

    if matches_x86_64 != matches_arm64:
        raise Exception('Matched files not equal for both directories')

    # Since we've just proven that both lists are equal at this point we only have to iterate one.

    for path_to_binary in matches_x86_64:
        lipo(input_base_path_x86_64, input_base_path_arm64, output_base_path, path_to_binary)


def build(args):
    path_to_module = Path(args.path_to_build) / juce_module_name
    path_to_module.mkdir(parents=True, exist_ok=True)

    path_to_libs = path_to_module / 'libs'  # Note: this is JUCE module specific
    path_to_libs.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env['VCPKG_OVERLAY_TRIPLETS'] = str(script_dir / 'triplets')

    if platform.system() == 'Darwin':

        subprocess.run([Path('submodules') / 'vcpkg' / 'bootstrap-vcpkg.sh'], check=True, cwd=script_dir)
        vcpkg = Path('submodules') / 'vcpkg' / 'vcpkg'

        vcpkg_installed_arm64 = script_dir / 'vcpkg_installed_arm64'
        vcpkg_installed_x86_64 = script_dir / 'vcpkg_installed_x86_64'

        subprocess.run([vcpkg, 'install', '--triplet=macos-arm64', f'--x-install-root={vcpkg_installed_arm64}'],
                       check=True, cwd=script_dir, env=env)
        subprocess.run([vcpkg, 'install', '--triplet=macos-x86-64', f'--x-install-root={vcpkg_installed_x86_64}'],
                       check=True, cwd=script_dir, env=env)

        path_to_libs_macosx = path_to_libs / 'MacOSX'
        path_to_libs_macosx.mkdir(parents=True, exist_ok=True)

        lipo_glob(Path(f'{vcpkg_installed_x86_64}/macos-x86-64/lib'),
                  Path(f'{vcpkg_installed_arm64}/macos-arm64/lib'),
                  path_to_libs_macosx, '*.a')

        shutil.copytree(f'{vcpkg_installed_arm64}/macos-arm64/include', path_to_module / 'include', dirs_exist_ok=True)

    elif platform.system() == 'Windows':
        subprocess.run([script_dir / 'submodules' / 'vcpkg' / 'bootstrap-vcpkg.bat'], check=True, cwd=script_dir)
        vcpkg = script_dir / 'submodules' / 'vcpkg' / 'vcpkg.exe'

        # Due to crappy windows paths longer than 260 are not supported. When using the default buildtrees root, the path
        # becomes too long, so instead we place the buildtrees in the root of the drive.
        vcpkg_buildtrees_root = 'C:\\indiekey_juce_vcpkg_buildtrees'
        vcpkg_installed = script_dir / 'vcpkg_installed'

        # MT
        subprocess.run([vcpkg, 'install', '--triplet=windows-x64-mt', f'--x-buildtrees-root={vcpkg_buildtrees_root}',
                        f'--x-install-root={vcpkg_installed}'],
                       check=True, cwd=script_dir, env=env)

        path_to_libs_vs2022_x64_mt = path_to_libs / 'VisualStudio2022' / 'x64' / 'MT'
        path_to_libs_vs2022_x64_mt.mkdir(parents=True, exist_ok=True)

        path_to_libs_vs2022_x64_mtd = path_to_libs / 'VisualStudio2022' / 'x64' / 'MTd'
        path_to_libs_vs2022_x64_mtd.mkdir(parents=True, exist_ok=True)

        shutil.copytree(f'{vcpkg_installed}/windows-x64-mt/lib', path_to_libs_vs2022_x64_mt, dirs_exist_ok=True)
        shutil.copytree(f'{vcpkg_installed}/windows-x64-mt/debug/lib', path_to_libs_vs2022_x64_mtd, dirs_exist_ok=True)

        # MD
        subprocess.run([vcpkg, 'install', '--triplet=windows-x64-md', f'--x-buildtrees-root={vcpkg_buildtrees_root}',
                        f'--x-install-root={vcpkg_installed}'],
                       check=True, cwd=script_dir, env=env)

        path_to_libs_vs2022_x64_md = path_to_libs / 'VisualStudio2022' / 'x64' / 'MD'
        path_to_libs_vs2022_x64_md.mkdir(parents=True, exist_ok=True)

        path_to_libs_vs2022_x64_mdd = path_to_libs / 'VisualStudio2022' / 'x64' / 'MDd'
        path_to_libs_vs2022_x64_mdd.mkdir(parents=True, exist_ok=True)

        shutil.copytree(f'{vcpkg_installed}/windows-x64-md/lib', path_to_libs_vs2022_x64_md, dirs_exist_ok=True)
        shutil.copytree(f'{vcpkg_installed}/windows-x64-md/debug/lib', path_to_libs_vs2022_x64_mdd, dirs_exist_ok=True)
        shutil.copytree(f'{vcpkg_installed}/windows-x64-md/include', path_to_module / 'include', dirs_exist_ok=True)

        # Headers
        shutil.copytree(f'{vcpkg_installed}/windows-x64-md/include', path_to_module / 'include', dirs_exist_ok=True)

    shutil.copytree(script_dir / 'include', path_to_module / 'include', dirs_exist_ok=True)
    shutil.copytree(script_dir / 'src', path_to_module / 'src', dirs_exist_ok=True)
    shutil.copy2(script_dir / 'CMakeLists.txt', path_to_module)
    shutil.copy2(script_dir / 'indiekey_juce.h', path_to_module)
    shutil.copy2(script_dir / 'indiekey_juce.cpp', path_to_module)


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--path-to-build",
                        help="The folder to build the project in",
                        default="build")

    parser.add_argument("--path-to-downloads",
                        help="The folder to store temporary downloads",
                        default="downloads")

    parser.add_argument("--build-number",
                        help="Specifies the build number",
                        default="0")

    parser.add_argument("--upload",
                        help="Upload the archive to spaces",
                        action='store_true')

    parser.add_argument("--spaces-key",
                        help="Specify the key for uploading to spaces")

    parser.add_argument("--spaces-secret",
                        help="Specify the secret for uploading to spaces")

    build(parser.parse_args())


if __name__ == '__main__':
    print("Invoke {} as script. Script dir: {}".format(script_path, script_dir))
    main()
