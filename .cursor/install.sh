#!/usr/bin/env bash
#
# Cloud Agent bootstrap for ESP-IDF (v3.3-dev, GNU Make build system).
#
# This script is idempotent: it can be run repeatedly and against a partially
# prepared machine (e.g. a warm snapshot) without side effects. It installs the
# host toolchain prerequisites, the Xtensa ESP32 cross-compiler, the ESP-IDF git
# submodules, and a Python virtual environment with the required packages.
#
set -euo pipefail

# IDF_PATH is the repository root (the directory that contains this .cursor dir).
IDF_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Pinned Xtensa toolchain matching tools/toolchain_versions.mk
# (crosstool-ng-1.22.0-80-g6c4433a, gcc 5.2.0).
TOOLCHAIN_TARBALL="xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz"
TOOLCHAIN_URL="https://dl.espressif.com/dl/${TOOLCHAIN_TARBALL}"
TOOLCHAIN_DIR="${HOME}/esp"
TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/xtensa-esp32-elf/bin"

VENV_DIR="${HOME}/.idf-venv"

echo "==> ESP-IDF cloud install: IDF_PATH=${IDF_PATH}"

# 1. Host system packages required by the GNU Make build and the kconfig menu tool.
echo "==> Installing system packages"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update -qq
sudo apt-get install -y -qq \
	gcc git wget make \
	flex bison gperf \
	libncurses-dev \
	python3 python3-venv python3-pip python-is-python3

# 2. Xtensa ESP32 cross-compiler (pinned version). Skip if already present.
if [ -x "${TOOLCHAIN_BIN}/xtensa-esp32-elf-gcc" ]; then
	echo "==> Toolchain already present at ${TOOLCHAIN_BIN}"
else
	echo "==> Downloading Xtensa toolchain"
	mkdir -p "${TOOLCHAIN_DIR}"
	tmp_tar="$(mktemp --suffix=.tar.gz)"
	wget -q -O "${tmp_tar}" "${TOOLCHAIN_URL}"
	tar -xzf "${tmp_tar}" -C "${TOOLCHAIN_DIR}"
	rm -f "${tmp_tar}"
fi
"${TOOLCHAIN_BIN}/xtensa-esp32-elf-gcc" --version | head -1

# 3. Fetch git submodules (component libraries such as wifi, bt, mbedtls, lwip).
echo "==> Initialising git submodules"
git -C "${IDF_PATH}" submodule update --init --recursive

# 4. Python environment with the ESP-IDF requirements.
#    setuptools is pinned <81 because the build's dependency checker imports the
#    (since-removed) pkg_resources module.
echo "==> Setting up Python virtualenv"
if [ ! -x "${VENV_DIR}/bin/python" ]; then
	python3 -m venv "${VENV_DIR}"
fi
"${VENV_DIR}/bin/pip" install -q --upgrade pip
"${VENV_DIR}/bin/pip" install -q "setuptools<81" -r "${IDF_PATH}/requirements.txt"

# 5. Persist the environment for interactive and login shells.
#    A single guarded block keeps this idempotent.
echo "==> Configuring shell environment (~/.bashrc)"
MARKER="# >>> esp-idf cloud env >>>"
if ! grep -qF "${MARKER}" "${HOME}/.bashrc" 2>/dev/null; then
	cat >> "${HOME}/.bashrc" <<EOF

${MARKER}
export IDF_PATH="${IDF_PATH}"
export PATH="${TOOLCHAIN_BIN}:\${PATH}"
# Activate the ESP-IDF Python virtualenv (adds venv python + tools to PATH).
if [ -f "${VENV_DIR}/bin/activate" ]; then
	source "${VENV_DIR}/bin/activate"
fi
# Force the host (kconfig) tools to build with gcc. The default 'cc' is clang,
# which rejects the legacy implicit-int test in tools/kconfig/lxdialog.
export CC=gcc
# <<< esp-idf cloud env <<<
EOF
fi

echo "==> ESP-IDF cloud install complete"
