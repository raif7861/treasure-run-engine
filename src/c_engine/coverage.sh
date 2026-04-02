#!/bin/bash
# coverage.sh — weighted coverage summary (robust version)

set -eE
set -o pipefail

BUILD_DIR="coverage_build"
BIN_DIR="coverage_bin"
SRC_DIR="src"
TEST_DIR="tests"
SUMMARY_FILE="coverage_summary.txt"

CC=gcc
CHECK_PREFIX=$(brew --prefix check 2>/dev/null || echo /usr)
CFLAGS="-Wall -Wextra -std=c11 -g -fprofile-arcs -ftest-coverage -D_POSIX_C_SOURCE=200809L -Iinclude -I${CHECK_PREFIX}/include"
LDFLAGS="-L${CHECK_PREFIX}/lib -lcheck -lsubunit -lpthread -lm -L../dist -lpuzzlegen"

command -v bc >/dev/null 2>&1 || { echo "[ERROR] 'bc' not found. Run: apt-get install -y bc"; exit 1; }

echo "[coverage] Cleaning..."
rm -rf "$BUILD_DIR" "$BIN_DIR" "$SUMMARY_FILE"
mkdir -p "$BUILD_DIR" "$BIN_DIR"

echo "[coverage] Compiling..."
for f in "$SRC_DIR"/*.c; do
  echo "  $f"
  $CC $CFLAGS -c "$f" -o "$BUILD_DIR/$(basename "${f%.c}.o")"
done

for f in "$TEST_DIR"/*.c; do
  echo "  $f"
  $CC $CFLAGS -c "$f" -o "$BUILD_DIR/$(basename "${f%.c}.o")"
done

echo "[coverage] Linking test executables..."
# Try building separate test executables (for multiple mains)
src_objs=$(ls "$BUILD_DIR"/*.o | grep -v "test_")
exe_count=0

for test_obj in "$BUILD_DIR"/test_*.o; do
  test_name=$(basename "${test_obj%.o}")
  echo "  Trying to build $test_name"
  if $CC -fprofile-arcs -ftest-coverage -o "$BIN_DIR/${test_name}_cov" "$test_obj" $src_objs $LDFLAGS 2>/dev/null; then
    echo "    ✓ Built ${test_name}_cov"
    exe_count=$((exe_count + 1))
  else
    echo "    ✗ Failed (no main?)"
    :  # no-op to prevent empty else block
  fi
done

# If no individual executables were built, try linking all tests together (single main case)
if [ $exe_count -eq 0 ]; then
  echo "[coverage] No individual test executables built, trying combined linking..."
  test_objs=$(ls "$BUILD_DIR"/test_*.o)
  if $CC -fprofile-arcs -ftest-coverage -o "$BIN_DIR/all_tests_cov" $test_objs $src_objs $LDFLAGS; then
    echo "  ✓ Built all_tests_cov (single main)"
    exe_count=1
  else
    echo "[ERROR] Could not build any test executables"
    exit 1
  fi
fi

echo "[coverage] Running tests..."
export LD_LIBRARY_PATH="../dist:$LD_LIBRARY_PATH"
export CK_VERBOSITY=silent
for test_exec in "$BIN_DIR"/*_cov; do
  test_name=$(basename "$test_exec")
  echo "  Running $test_name"
  "$test_exec" >/dev/null || echo "[WARN] $test_name had failures (continuing for coverage)"
done

echo "[coverage] Running gcov..."
total_lines=0; weighted_line_hits=0
total_branches=0; weighted_branch_hits=0

for f in "$SRC_DIR"/*.c; do
  filename=$(basename "$f")
  
  # Skip graph.c - it's a reusable ADT, not part of the assignment
  if [[ "$filename" == "graph.c" ]]; then
    echo "[gcov] Skipping $filename (excluded from coverage)"
    continue
  fi
  
  echo "[gcov] Processing $filename"
  if ! output=$(gcov -b -c -o "$BUILD_DIR" "$f" 2>/dev/null); then
    echo "[WARN] gcov failed on $f"
    continue
  fi
  echo "$output" | grep -E "Lines executed|Branches executed" >> "$SUMMARY_FILE"

  while read -r line; do
    if [[ $line =~ ^Lines\ executed:([0-9.]+)%\ of\ ([0-9]+) ]]; then
      percent="${BASH_REMATCH[1]}"; lines="${BASH_REMATCH[2]}"
      total_lines=$((total_lines + lines))
      weighted_line_hits=$(echo "$weighted_line_hits + $percent * $lines / 100" | bc -l)
    fi
    if [[ $line =~ ^Branches\ executed:([0-9.]+)%\ of\ ([0-9]+) ]]; then
      percent="${BASH_REMATCH[1]}"; branches="${BASH_REMATCH[2]}"
      total_branches=$((total_branches + branches))
      weighted_branch_hits=$(echo "$weighted_branch_hits + $percent * $branches / 100" | bc -l)
    fi
  done <<< "$output"
done

if (( total_lines > 0 )); then
  avg_line=$(echo "scale=2; 100 * $weighted_line_hits / $total_lines" | bc -l)
else
  avg_line=0
fi

if (( total_branches > 0 )); then
  avg_branch=$(echo "scale=2; 100 * $weighted_branch_hits / $total_branches" | bc -l)
else
  avg_branch=0
fi

{
  echo "AVERAGE LINE COVERAGE: $avg_line%"
  echo "AVERAGE BRANCH COVERAGE: $avg_branch%"
} >> "$SUMMARY_FILE"

echo "[coverage] Weighted summary: lines=${avg_line}% branches=${avg_branch}%"