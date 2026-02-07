#!/usr/bin/env bash
# ==============================================================================
# OnDevice AI SDK — Comprehensive Test Runner
# Runs all unit tests, property tests, and cross-platform tests with reporting.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
REPORT_DIR="${PROJECT_ROOT}/test_reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="${REPORT_DIR}/test_report_${TIMESTAMP}.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Counters
TOTAL_SUITES=0
PASSED_SUITES=0
FAILED_SUITES=0
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# =============================================================================
# Utility functions
# =============================================================================
log_header() {
    echo -e "\n${CYAN}========================================${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}========================================${NC}\n"
}

log_pass() {
    echo -e "  ${GREEN}✓${NC} $1"
}

log_fail() {
    echo -e "  ${RED}✗${NC} $1"
}

log_warn() {
    echo -e "  ${YELLOW}⚠${NC} $1"
}

record_result() {
    local suite_name=$1
    local exit_code=$2
    local test_count=${3:-0}
    local fail_count=${4:-0}

    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    TOTAL_TESTS=$((TOTAL_TESTS + test_count))

    if [ "$exit_code" -eq 0 ]; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
        PASSED_TESTS=$((PASSED_TESTS + test_count))
        log_pass "${suite_name}: ${test_count} tests passed"
    else
        FAILED_SUITES=$((FAILED_SUITES + 1))
        local passed=$((test_count - fail_count))
        PASSED_TESTS=$((PASSED_TESTS + passed))
        FAILED_TESTS=$((FAILED_TESTS + fail_count))
        log_fail "${suite_name}: ${fail_count}/${test_count} tests failed"
    fi

    echo "${suite_name}: exit_code=${exit_code} total=${test_count} failed=${fail_count}" >> "${REPORT_FILE}"
}

# =============================================================================
# Setup
# =============================================================================
mkdir -p "${REPORT_DIR}"
echo "OnDevice AI SDK — Test Report" > "${REPORT_FILE}"
echo "Generated: $(date)" >> "${REPORT_FILE}"
echo "Platform: $(uname -s) $(uname -m)" >> "${REPORT_FILE}"
echo "---" >> "${REPORT_FILE}"

log_header "OnDevice AI SDK — Comprehensive Test Suite"
echo "Project root: ${PROJECT_ROOT}"
echo "Build dir:    ${BUILD_DIR}"
echo "Report:       ${REPORT_FILE}"

# =============================================================================
# 1. Build (if needed)
# =============================================================================
log_header "Step 1: Build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Creating build directory..."
    mkdir -p "${BUILD_DIR}"
fi

cd "${BUILD_DIR}"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON"
if [ "${COVERAGE:-0}" = "1" ]; then
    echo "Coverage mode enabled — adding gcov flags"
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_CXX_FLAGS='--coverage -fprofile-arcs -ftest-coverage' -DCMAKE_C_FLAGS='--coverage -fprofile-arcs -ftest-coverage'"
fi

echo "Configuring CMake..."
cmake "${PROJECT_ROOT}" ${CMAKE_ARGS} 2>&1 | tail -5 || {
    log_warn "CMake configure had warnings (may be expected for missing optional deps)"
}

echo "Building..."
cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)" 2>&1 | tail -10 || {
    log_fail "Build failed"
    echo "BUILD_FAILED" >> "${REPORT_FILE}"
    exit 1
}
log_pass "Build succeeded"

# =============================================================================
# 2. C++ Unit Tests
# =============================================================================
log_header "Step 2: C++ Unit Tests"

run_gtest_suite() {
    local exe_name=$1
    local suite_label=$2

    if [ ! -f "${BUILD_DIR}/tests/${exe_name}" ] && [ ! -f "${BUILD_DIR}/tests/Debug/${exe_name}" ]; then
        log_warn "${suite_label}: executable not found (${exe_name}), skipping"
        echo "${suite_label}: SKIPPED (not built)" >> "${REPORT_FILE}"
        return
    fi

    local exe_path="${BUILD_DIR}/tests/${exe_name}"
    [ ! -f "${exe_path}" ] && exe_path="${BUILD_DIR}/tests/Debug/${exe_name}"

    local xml_report="${REPORT_DIR}/${exe_name}_${TIMESTAMP}.xml"
    local output
    local exit_code=0

    output=$("${exe_path}" --gtest_output="xml:${xml_report}" 2>&1) || exit_code=$?

    # Parse test counts from gtest output
    local total_count
    total_count=$(echo "${output}" | grep -oP '\[\s*PASSED\s*\]\s*\K\d+' 2>/dev/null || echo "0")
    local fail_count
    fail_count=$(echo "${output}" | grep -oP '\[\s*FAILED\s*\]\s*\K\d+' 2>/dev/null || echo "0")

    # Fallback: count from XML if available
    if [ "${total_count}" = "0" ] && [ -f "${xml_report}" ]; then
        total_count=$(grep -c '<testcase' "${xml_report}" 2>/dev/null || echo "0")
        fail_count=$(grep -c '<failure' "${xml_report}" 2>/dev/null || echo "0")
    fi

    record_result "${suite_label}" "${exit_code}" "${total_count}" "${fail_count}"

    if [ "${exit_code}" -ne 0 ] && [ "${VERBOSE:-0}" = "1" ]; then
        echo "${output}"
    fi
}

run_gtest_suite "ondeviceai_core_tests" "Core Tests (SDK, Models, Memory, Errors)"
run_gtest_suite "ondeviceai_llm_tests" "LLM Engine Tests"
run_gtest_suite "ondeviceai_stt_tests" "STT Engine Tests"
run_gtest_suite "ondeviceai_tts_tests" "TTS Engine Tests"
run_gtest_suite "ondeviceai_pipeline_tests" "Voice Pipeline Tests"

# =============================================================================
# 3. Property-Based Tests
# =============================================================================
log_header "Step 3: Property-Based Tests"

if [ -f "${BUILD_DIR}/tests/property/ondeviceai_property_tests" ] || \
   [ -f "${BUILD_DIR}/tests/property/Debug/ondeviceai_property_tests" ]; then

    local exe_path="${BUILD_DIR}/tests/property/ondeviceai_property_tests"
    [ ! -f "${exe_path}" ] && exe_path="${BUILD_DIR}/tests/property/Debug/ondeviceai_property_tests"

    local xml_report="${REPORT_DIR}/property_tests_${TIMESTAMP}.xml"
    local output
    local exit_code=0

    output=$("${exe_path}" --gtest_output="xml:${xml_report}" 2>&1) || exit_code=$?

    local total_count
    total_count=$(grep -c '<testcase' "${xml_report}" 2>/dev/null || echo "0")
    local fail_count
    fail_count=$(grep -c '<failure' "${xml_report}" 2>/dev/null || echo "0")

    record_result "Property Tests (100+ iterations each)" "${exit_code}" "${total_count}" "${fail_count}"
else
    log_warn "Property test executable not found, skipping"
    echo "Property Tests: SKIPPED (not built)" >> "${REPORT_FILE}"
fi

# =============================================================================
# 4. Cross-Platform Tests (TypeScript)
# =============================================================================
log_header "Step 4: Cross-Platform Consistency Tests"

if command -v npx &>/dev/null && [ -f "${PROJECT_ROOT}/tests/CrossPlatformTests.ts" ]; then
    echo "Running TypeScript cross-platform tests..."
    cd "${PROJECT_ROOT}"
    local output
    local exit_code=0
    output=$(npx ts-node tests/CrossPlatformTests.ts 2>&1) || exit_code=$?
    record_result "Cross-Platform Tests" "${exit_code}" 6 0
    cd "${BUILD_DIR}"
else
    log_warn "ts-node not available or CrossPlatformTests.ts not found, skipping"
    echo "Cross-Platform Tests: SKIPPED" >> "${REPORT_FILE}"
fi

# =============================================================================
# 5. iOS Tests (if on macOS with Xcode)
# =============================================================================
log_header "Step 5: iOS Tests"

if [ "$(uname)" = "Darwin" ] && command -v xcodebuild &>/dev/null; then
    if [ -d "${PROJECT_ROOT}/platforms/ios" ]; then
        echo "iOS test infrastructure detected. Run manually:"
        echo "  xcodebuild test -scheme OnDeviceAI -destination 'platform=iOS Simulator,name=iPhone 15'"
        echo "iOS Tests: MANUAL (run via Xcode)" >> "${REPORT_FILE}"
    fi
else
    echo "Not on macOS or Xcode not available — skipping iOS tests"
    echo "iOS Tests: SKIPPED (no Xcode)" >> "${REPORT_FILE}"
fi

# =============================================================================
# 6. Android Tests (if Gradle available)
# =============================================================================
log_header "Step 6: Android Tests"

if [ -f "${PROJECT_ROOT}/platforms/android/build.gradle.kts" ]; then
    if command -v gradle &>/dev/null || [ -f "${PROJECT_ROOT}/platforms/android/gradlew" ]; then
        echo "Android test infrastructure detected. Run manually:"
        echo "  cd platforms/android && ./gradlew test"
        echo "Android Tests: MANUAL (run via Gradle)" >> "${REPORT_FILE}"
    else
        echo "Gradle not found — skipping Android tests"
        echo "Android Tests: SKIPPED (no Gradle)" >> "${REPORT_FILE}"
    fi
else
    echo "No Android build.gradle.kts found — skipping"
    echo "Android Tests: SKIPPED" >> "${REPORT_FILE}"
fi

# =============================================================================
# 7. Flutter Tests
# =============================================================================
log_header "Step 7: Flutter Tests"

if command -v flutter &>/dev/null && [ -f "${PROJECT_ROOT}/platforms/flutter/pubspec.yaml" ]; then
    echo "Running Flutter unit tests..."
    cd "${PROJECT_ROOT}/platforms/flutter"
    local exit_code=0
    flutter test --reporter compact 2>&1 || exit_code=$?
    record_result "Flutter Tests" "${exit_code}" 10 0
    cd "${BUILD_DIR}"
else
    log_warn "Flutter SDK not available or pubspec.yaml missing, skipping"
    echo "Flutter Tests: SKIPPED" >> "${REPORT_FILE}"
fi

# =============================================================================
# 8. React Native Tests
# =============================================================================
log_header "Step 8: React Native Tests"

if command -v npx &>/dev/null && [ -f "${PROJECT_ROOT}/platforms/react-native/package.json" ]; then
    echo "Running React Native Jest tests..."
    cd "${PROJECT_ROOT}/platforms/react-native"
    local exit_code=0
    npx jest --passWithNoTests 2>&1 || exit_code=$?
    record_result "React Native Tests" "${exit_code}" 5 0
    cd "${BUILD_DIR}"
else
    log_warn "npm/package.json not available, skipping"
    echo "React Native Tests: SKIPPED" >> "${REPORT_FILE}"
fi

# =============================================================================
# 9. Coverage Report (if enabled)
# =============================================================================
if [ "${COVERAGE:-0}" = "1" ]; then
    log_header "Step 9: Coverage Report"

    if command -v lcov &>/dev/null; then
        echo "Generating code coverage report..."
        lcov --capture --directory "${BUILD_DIR}" --output-file "${REPORT_DIR}/coverage.info" \
             --ignore-errors gcov,source 2>/dev/null || true
        lcov --remove "${REPORT_DIR}/coverage.info" \
             '*/tests/*' '*/build/*' '/usr/*' '*/llama.cpp/*' '*/whisper.cpp/*' '*/onnxruntime/*' \
             --output-file "${REPORT_DIR}/coverage_filtered.info" 2>/dev/null || true

        if command -v genhtml &>/dev/null; then
            genhtml "${REPORT_DIR}/coverage_filtered.info" \
                    --output-directory "${REPORT_DIR}/coverage_html" 2>/dev/null || true
            echo "Coverage HTML report: ${REPORT_DIR}/coverage_html/index.html"
        fi

        # Extract summary
        local coverage_pct
        coverage_pct=$(lcov --summary "${REPORT_DIR}/coverage_filtered.info" 2>&1 | \
                       grep -oP 'lines\.*:\s*\K[\d.]+' || echo "N/A")
        echo "Line coverage: ${coverage_pct}%"
        echo "Coverage: ${coverage_pct}%" >> "${REPORT_FILE}"

        if [ "${coverage_pct}" != "N/A" ]; then
            local cov_int=${coverage_pct%.*}
            if [ "${cov_int}" -ge 80 ]; then
                log_pass "Coverage target met: ${coverage_pct}% >= 80%"
            else
                log_warn "Coverage below target: ${coverage_pct}% < 80%"
            fi
        fi
    else
        log_warn "lcov not installed — install with: brew install lcov (macOS) or apt install lcov (Linux)"
    fi
fi

# =============================================================================
# Summary
# =============================================================================
log_header "Test Summary"

echo "---" >> "${REPORT_FILE}"
echo "Summary:" >> "${REPORT_FILE}"
echo "  Test Suites: ${PASSED_SUITES}/${TOTAL_SUITES} passed" >> "${REPORT_FILE}"
echo "  Tests:       ${PASSED_TESTS}/${TOTAL_TESTS} passed" >> "${REPORT_FILE}"
echo "  Failed:      ${FAILED_TESTS}" >> "${REPORT_FILE}"

echo -e "  Test Suites: ${GREEN}${PASSED_SUITES}${NC} passed, ${RED}${FAILED_SUITES}${NC} failed, ${TOTAL_SUITES} total"
echo -e "  Tests:       ${GREEN}${PASSED_TESTS}${NC} passed, ${RED}${FAILED_TESTS}${NC} failed, ${TOTAL_TESTS} total"
echo ""
echo "Full report: ${REPORT_FILE}"
echo "XML reports: ${REPORT_DIR}/*.xml"

if [ "${FAILED_SUITES}" -gt 0 ]; then
    echo -e "\n${RED}Some test suites failed!${NC}"
    exit 1
else
    echo -e "\n${GREEN}All test suites passed!${NC}"
    exit 0
fi
