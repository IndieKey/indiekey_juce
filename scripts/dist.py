# Script location matters, cwd does not
import argparse
import json
import shutil
from datetime import datetime
from pathlib import Path

import boto3
import pygit2

script_path = Path(__file__)
script_dir = script_path.parent

# Git version
repo = pygit2.Repository(path='.')
git_version = repo.describe(pattern='v*')
git_branch = repo.head.shorthand

juce_module_name = 'indiekey_juce'


def upload_to_spaces(args, file: Path):
    session = boto3.session.Session()

    key = args.spaces_key
    secret = args.spaces_secret

    if not key:
        raise Exception('Need spaces key')

    if not secret:
        raise Exception('Need spaces secret')

    client = session.client('s3',
                            endpoint_url="https://lon1.digitaloceanspaces.com",
                            region_name="lon1",
                            aws_access_key_id=key,
                            aws_secret_access_key=secret)

    folder = 'branches/' + git_branch

    if git_branch == 'HEAD':
        # If we're in head, we're most likely building from a tag in which case we want to archive the artifacts
        folder = 'archive/' + git_version

    bucket = 'indiekey-juce'
    file_name = folder + '/' + file.name
    client.upload_file(str(file), bucket, file_name)

    print("Uploaded artefacts to {}/{}".format(bucket, file_name))


def build(args):
    path_to_build = Path(args.path_to_build)
    path_to_module = path_to_build / 'module'
    path_to_build_macos = Path(args.path_to_build_macos)

    path_to_build_windows = Path(args.path_to_build_windows)

    shutil.copytree(path_to_build_macos, path_to_module, dirs_exist_ok=True)
    shutil.copytree(path_to_build_windows / juce_module_name / 'libs', path_to_module / juce_module_name / 'libs', dirs_exist_ok=True)

    version_data = {
        "version": git_version,
        "build_number": args.build_number,
        "date": str(datetime.now())
    }

    with open(path_to_module / juce_module_name / 'version.json', 'w') as file:
        json.dump(version_data, file, indent=4)

    archive_path = path_to_build / f'indiekey_juce-{git_version}-{args.build_number}-dist'
    path_to_zip = shutil.make_archive(str(archive_path), 'zip', path_to_module)

    if args.upload:
        upload_to_spaces(args, Path(path_to_zip))


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--path-to-build",
                        help="The folder to build the project in",
                        default="build-dist")

    parser.add_argument("--path-to-build-macos",
                        help="Specifies the path to the macOS build",
                        default="build-macos")

    parser.add_argument("--path-to-build-windows",
                        help="Specifies the path to the Windows build",
                        default="build-windows")

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

