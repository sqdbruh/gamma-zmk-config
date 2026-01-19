import os
import sys


def find_marker(path: str, marker: bytes) -> bool:
    if not os.path.exists(path):
        return False
    with open(path, "rb") as f:
        data = f.read()
    return marker in data


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: verify_marker.py <build_dir> <marker>", file=sys.stderr)
        return 2

    build_dir = sys.argv[1]
    marker = sys.argv[2].encode("utf-8")

    uf2_path = os.path.join(build_dir, "zephyr", "zephyr.uf2")
    elf_path = os.path.join(build_dir, "zephyr", "zephyr.elf")

    if find_marker(uf2_path, marker):
        print(f"marker found in {uf2_path}")
        return 0

    if find_marker(elf_path, marker):
        print(f"marker found in {elf_path} (UF2 missing marker)")
        return 1

    print("marker not found in UF2 or ELF", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
