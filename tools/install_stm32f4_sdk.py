#!/usr/bin/env python3
"""Install the pinned STM32CubeF4 CMSIS/HAL outside the framework repository."""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

SDK_VERSION = "v1.28.3"
SDK_COMMIT = "94cae6e83f00e276a11957e7833c01ac3d0bd7af"
SDK_REPOSITORY = "https://github.com/STMicroelectronics/STM32CubeF4.git"
SDK_COMPONENTS = ("Drivers/CMSIS/Device/ST/STM32F4xx", "Drivers/STM32F4xx_HAL_Driver")


def git(directory, *arguments):
    return subprocess.run(["git", "-C", str(directory), *arguments], check=True,
                          text=True, stdout=subprocess.PIPE).stdout.strip()


def verify(directory):
    if git(directory, "rev-parse", "HEAD") != SDK_COMMIT:
        raise SystemExit(f"{directory}: expected STM32CubeF4 {SDK_VERSION} ({SDK_COMMIT})")
    for component in SDK_COMPONENTS:
        expected = git(directory, "rev-parse", f"HEAD:{component}")
        if git(directory / component, "rev-parse", "HEAD") != expected:
            raise SystemExit(f"{directory / component}: expected revision {expected}")
    if git(directory, "status", "--porcelain", "--untracked-files=no"):
        raise SystemExit(f"{directory}: SDK has local modifications; use a clean installation")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path, help="External STM32CubeF4 installation directory")
    destination = parser.parse_args().destination.expanduser().resolve()
    repository = Path(__file__).resolve().parents[1]
    if destination == repository or repository in destination.parents:
        parser.error("the SDK must be installed outside the framework repository")
    if destination.exists():
        verify(destination)
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix=f".{destination.name}-", dir=destination.parent) as temporary:
            checkout = Path(temporary) / "sdk"
            checkout.mkdir()
            git(checkout, "init", "--quiet")
            git(checkout, "remote", "add", "origin", SDK_REPOSITORY)
            git(checkout, "sparse-checkout", "set", "Drivers/CMSIS/Include", *SDK_COMPONENTS)
            git(checkout, "fetch", "--filter=blob:none", "--depth=1", "origin", SDK_COMMIT)
            git(checkout, "checkout", "--detach", "FETCH_HEAD")
            git(checkout, "submodule", "update", "--init", "--depth=1", *SDK_COMPONENTS)
            verify(checkout)
            checkout.rename(destination)
    print(f"STM32CubeF4 {SDK_VERSION}: {destination}")
    print(f"export STM32F4_SDK_DIR={shlex.quote(str(destination))}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"SDK installation failed: git exited with status {error.returncode}") from None
