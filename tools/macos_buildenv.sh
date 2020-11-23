#!/bin/bash
set -o pipefail

if [ "$2" != "--ghactions" ] && ! $(return 0 2>/dev/null); then
  echo "This script must be run by sourcing it:"
  echo "source $0 $@"
  exit 1
fi

COMMAND=$1
shift 1

realpath() {
    OLDPWD="${PWD}"
    cd "$1" || exit 1
    pwd
    cd "${OLDPWD}" || exit 1
}

# some hackery is required to be compatible with both bash and zsh
THIS_SCRIPT_NAME=${BASH_SOURCE[0]}
[ -z "$THIS_SCRIPT_NAME" ] && THIS_SCRIPT_NAME=$0

MIXXX_ROOT="$(realpath "$(dirname "$THIS_SCRIPT_NAME")/..")"

read -d'\n' BUILDENV_NAME BUILDENV_SHA256 < "${MIXXX_ROOT}/cmake/macos_build_environment"

[ -z "$BUILDENV_BASEPATH" ] && BUILDENV_BASEPATH="${MIXXX_ROOT}/buildenv"

case "$COMMAND" in
    name)
        if [ "$1" = "--ghactions" ]; then
            echo "::set-output name=buildenv_name::$envname"
        else
            echo "$BUILDENV_NAME"
        fi
        ;;

    setup)
        if [[ "$BUILDENV_NAME" =~ .*macosminimum([0-9]*\.[0-9]*).* ]]; then
            MACOSX_DEPLOYMENT_TARGET="${BASH_REMATCH[1]}"
        else
            echo "Build environment did not match expected pattern. Check ${MIXXX_ROOT}/cmake/macos_build_environment file." >&2
            return
        fi

        BUILDENV_PATH="${BUILDENV_BASEPATH}/${BUILDENV_NAME}"
        mkdir -p "${BUILDENV_BASEPATH}"
        if [ ! -d "${BUILDENV_PATH}" ]; then
            if [ "$1" != "--profile" ]; then
                echo "Build environment $BUILDENV_NAME not found in mixxx repository, downloading it..."
                curl "https://downloads.mixxx.org/builds/buildserver/2.3.x-unix/${BUILDENV_NAME}.tar.gz" -o "${BUILDENV_PATH}.tar.gz"
                OBSERVED_SHA256=$(shasum -a 256 "${BUILDENV_PATH}.tar.gz"|cut -f 1 -d' ')
                if [ $OBSERVED_SHA256 == $BUILDENV_SHA256 ]; then
                    echo "Download matched expected SHA256 sum $BUILDENV_SHA256"
                else
                    echo "ERROR: Download did not match expected SHA256 checksum!"
                    echo "Expected $BUILDENV_SHA256"
                    echo "Got $OBSERVED_SHA256"
                    exit 1
                fi
                echo "Extracting ${BUILDENV_NAME}.tar.gz..."
                tar xf "${BUILDENV_PATH}.tar.gz" -C "${BUILDENV_BASEPATH}"
            else
                echo "Build environment $BUILDENV_NAME not found in mixxx repository, run the command below to download it."
                echo "source ${THIS_SCRIPT_NAME} setup"
                return # exit would quit the shell being started
            fi
        elif [ "$1" != "--profile" ]; then
            echo "Build environment found: ${BUILDENV_PATH}"
        fi

        export CMAKE_PREFIX_PATH="${BUILDENV_PATH}"
        export PATH="${BUILDENV_PATH}/bin:${PATH}"
        export Qt5_DIR="$(find "${BUILDENV_PATH}" -type d -path "*/cmake/Qt5")"
        export QT_QPA_PLATFORM_PLUGIN_PATH="$(find "${BUILDENV_PATH}" -type d -path "*/plugins")"

        if [ "$1" = "--ghactions" ]; then
            echo "::set-output name=macosx_deployment_target::${MACOSX_DEPLOYMENT_TARGET}"
            echo "::set-output name=cmake_prefix_path::${CMAKE_PREFIX_PATH}"
            echo "::set-output name=path::${PATH}"
            echo "::set-output name=qt_path::${Qt5_DIR}"
            echo "::set-output name=qt_qpa_platform_plugin_path::${QT_QPA_PLATFORM_PLUGIN_PATH}"
        fi
        ;;
esac
