#!/usr/bin/env bash
#
# mayhem/build.sh — builds the sanitized fuzz harnesses (+ standalone reproducers) and, with the
# project's NORMAL flags, RmlUi's own unit test suite. Runs in the commit image as `mayhem` in /mayhem.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE STANDALONE_FUZZ_MAIN MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# 1) Sanitized RmlUi core + harnesses. -fsanitize=fuzzer-no-link goes on the library unconditionally, so the
#    fuzzed code carries coverage instrumentation even with SANITIZER_FLAGS empty. Build type "None" keeps
#    RMLUI_DEBUG on (assertions abort via the harness's system interface) and lets $DEBUG_FLAGS (-gdwarf-3)
#    be the only debug flag. Build dirs are /build*/, which upstream's .gitignore covers.
FUZZ_FLAGS="-O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S mayhem/cmake -B build-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=None \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DRMLUI_SOURCE_DIR="$SRC" -DLSAN_OFF_SOURCE="$SRC/mayhem/lsan_off.cc" -DBUILD_SHARED_LIBS=OFF -DRMLUI_PRECOMPILED_HEADERS=OFF
cmake --build build-fuzz -j"$MAYHEM_JOBS"
for t in fuzz_rml_document fuzz_style_sheet fuzz_data_expression; do
  cp "build-fuzz/$t" "build-fuzz/$t-standalone" /mayhem/
done

# 2) The unit test suite, normal flags (no sanitizer, no -gdwarf-3), separate build dir.
cmake -S . -B build-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=OFF -DRMLUI_BACKEND=GLFW_GL3 \
  -DRMLUI_PRECOMPILED_HEADERS=OFF \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build-tests -j"$MAYHEM_JOBS" --target rmlui_unit_tests
