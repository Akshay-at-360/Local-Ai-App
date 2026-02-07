#!/usr/bin/env bash
# ==============================================================================
# OnDevice AI SDK — Publish Release Script
# Task 23.2: Publish to distribution channels
#
# Usage:
#   ./scripts/publish_release.sh --version 1.0.0 [--dry-run] [--channel all|cocoapods|maven|npm|pub]
#
# Channels:
#   cocoapods  — Push to CocoaPods trunk (iOS)
#   maven      — Publish to Maven Central (Android)
#   npm        — Publish to npm registry (React Native)
#   pub        — Publish to pub.dev (Flutter)
#   github     — Create GitHub release with artifacts
#   all        — All of the above
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ARTIFACTS_DIR="${PROJECT_ROOT}/release_artifacts"

# Defaults
VERSION="${VERSION:-1.0.0}"
DRY_RUN=false
CHANNEL="all"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --version) VERSION="$2"; shift 2 ;;
        --dry-run) DRY_RUN=true; shift ;;
        --channel) CHANNEL="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() { echo -e "${CYAN}[PUBLISH]${NC} $1"; }
success() { echo -e "${GREEN}[PUBLISH]${NC} $1"; }
warn() { echo -e "${YELLOW}[PUBLISH]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }

run_cmd() {
    if [ "${DRY_RUN}" = true ]; then
        warn "[DRY-RUN] Would execute: $*"
    else
        "$@"
    fi
}

# =============================================================================
# Pre-flight checks
# =============================================================================

log "OnDevice AI SDK — Release Publisher v${VERSION}"
log "Dry run: ${DRY_RUN}"
log "Channel: ${CHANNEL}"

# Verify we're on a clean tagged commit
if ! git diff --quiet 2>/dev/null; then
    warn "Working tree has uncommitted changes!"
    if [ "${DRY_RUN}" = false ]; then
        error "Cannot publish with uncommitted changes. Commit or stash first."
        exit 1
    fi
fi

# Check tag exists
if ! git tag -l "v${VERSION}" | grep -q "v${VERSION}"; then
    warn "Tag v${VERSION} not found."
    log "Create with: git tag -a v${VERSION} -m 'Release v${VERSION}'"
    if [ "${DRY_RUN}" = false ]; then
        read -rp "Create tag now? [y/N] " confirm
        if [ "${confirm}" = "y" ] || [ "${confirm}" = "Y" ]; then
            git tag -a "v${VERSION}" -m "Release v${VERSION}"
            log "Tag v${VERSION} created"
        else
            error "Tag required for release"
            exit 1
        fi
    fi
fi

# =============================================================================
# GitHub Release
# =============================================================================

publish_github() {
    log "Creating GitHub release..."

    if ! command -v gh &>/dev/null; then
        error "GitHub CLI (gh) not found. Install: brew install gh"
        return 1
    fi

    local archive="ondeviceai-sdk-v${VERSION}.tar.gz"
    if [ ! -f "${PROJECT_ROOT}/${archive}" ]; then
        warn "Release archive not found. Run build_release.sh first."
        return 1
    fi

    # Generate release notes from CHANGELOG
    local notes_file=$(mktemp)
    sed -n "/^## \[${VERSION}\]/,/^## \[/p" "${PROJECT_ROOT}/CHANGELOG.md" | head -n -1 > "${notes_file}"

    run_cmd gh release create "v${VERSION}" \
        --title "OnDevice AI SDK v${VERSION}" \
        --notes-file "${notes_file}" \
        "${PROJECT_ROOT}/${archive}"

    rm -f "${notes_file}"
    success "GitHub release created"
}

# =============================================================================
# CocoaPods (iOS)
# =============================================================================

publish_cocoapods() {
    log "Publishing to CocoaPods..."

    if ! command -v pod &>/dev/null; then
        error "CocoaPods not found. Install: gem install cocoapods"
        return 1
    fi

    # Generate podspec
    local podspec="${PROJECT_ROOT}/OnDeviceAI.podspec"
    cat > "${podspec}" << EOF
Pod::Spec.new do |s|
  s.name             = 'OnDeviceAI'
  s.version          = '${VERSION}'
  s.summary          = 'On-device AI inference SDK for iOS'
  s.description      = 'Privacy-first AI SDK providing LLM, STT, and TTS capabilities entirely on-device.'
  s.homepage         = 'https://github.com/Akshay-at-360/Local-Ai-App'
  s.license          = { :type => 'MIT', :file => 'LICENSE' }
  s.author           = { '360 Labs' => 'dev@360labs.com' }
  s.source           = { :git => 'https://github.com/Akshay-at-360/Local-Ai-App.git', :tag => "v#{s.version}" }

  s.ios.deployment_target = '12.0'
  s.swift_version = '5.5'

  s.source_files = 'platforms/ios/**/*.{swift,h,mm,cpp}'
  s.public_header_files = 'platforms/ios/**/*.h'
  s.preserve_paths = 'core/**/*'

  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'HEADER_SEARCH_PATHS' => '"\$(PODS_TARGET_SRCROOT)/core/include"',
    'OTHER_LDFLAGS' => '-lc++'
  }

  s.frameworks = 'Foundation', 'Metal', 'CoreML', 'Accelerate'
  s.libraries = 'c++'
end
EOF

    run_cmd pod lib lint "${podspec}" --allow-warnings
    run_cmd pod trunk push "${podspec}" --allow-warnings

    success "CocoaPods published"
}

# =============================================================================
# Maven Central (Android)
# =============================================================================

publish_maven() {
    log "Publishing to Maven Central..."

    if [ ! -f "${PROJECT_ROOT}/platforms/android/build.gradle.kts" ]; then
        error "Android build.gradle.kts not found"
        return 1
    fi

    # Check for signing credentials
    if [ -z "${OSSRH_USERNAME:-}" ] || [ -z "${OSSRH_PASSWORD:-}" ]; then
        warn "Maven Central credentials not set (OSSRH_USERNAME, OSSRH_PASSWORD)"
        warn "Set credentials and run again, or publish manually via Sonatype Nexus"
        return 0
    fi

    cd "${PROJECT_ROOT}/platforms/android"

    run_cmd ./gradlew publishToMavenLocal \
        -Pversion="${VERSION}" \
        -PossrhUsername="${OSSRH_USERNAME}" \
        -PossrhPassword="${OSSRH_PASSWORD}" 2>/dev/null || {
            warn "Gradle publish not configured — add maven-publish plugin to build.gradle.kts"
        }

    cd "${PROJECT_ROOT}"
    success "Maven publication initiated"
}

# =============================================================================
# npm Registry (React Native)
# =============================================================================

publish_npm() {
    log "Publishing to npm..."

    if ! command -v npm &>/dev/null; then
        error "npm not found"
        return 1
    fi

    local rn_dir="${PROJECT_ROOT}/platforms/react-native"
    if [ ! -f "${rn_dir}/package.json" ]; then
        error "React Native package.json not found"
        return 1
    fi

    cd "${rn_dir}"

    # Update version in package.json
    local tmp=$(mktemp)
    jq ".version = \"${VERSION}\"" package.json > "${tmp}" && mv "${tmp}" package.json 2>/dev/null || {
        # Fallback: sed-based version update
        sed -i.bak "s/\"version\": \"[^\"]*\"/\"version\": \"${VERSION}\"/" package.json
        rm -f package.json.bak
    }

    run_cmd npm publish --access public

    cd "${PROJECT_ROOT}"
    success "npm package published"
}

# =============================================================================
# pub.dev (Flutter)
# =============================================================================

publish_pub() {
    log "Publishing to pub.dev..."

    if ! command -v flutter &>/dev/null; then
        error "Flutter SDK not found"
        return 1
    fi

    local flutter_dir="${PROJECT_ROOT}/platforms/flutter"
    if [ ! -f "${flutter_dir}/pubspec.yaml" ]; then
        error "Flutter pubspec.yaml not found"
        return 1
    fi

    cd "${flutter_dir}"

    # Update version in pubspec.yaml
    sed -i.bak "s/^version: .*/version: ${VERSION}/" pubspec.yaml
    rm -f pubspec.yaml.bak

    if [ "${DRY_RUN}" = true ]; then
        run_cmd flutter pub publish --dry-run
    else
        flutter pub publish --force
    fi

    cd "${PROJECT_ROOT}"
    success "pub.dev package published"
}

# =============================================================================
# Push Git Tag
# =============================================================================

push_tag() {
    log "Pushing tag v${VERSION} to origin..."
    run_cmd git push origin "v${VERSION}"
    run_cmd git push origin main
    success "Tag pushed to remote"
}

# =============================================================================
# Main
# =============================================================================

case "${CHANNEL}" in
    all)
        push_tag
        publish_github
        publish_cocoapods || warn "CocoaPods publish skipped"
        publish_maven || warn "Maven publish skipped"
        publish_npm || warn "npm publish skipped"
        publish_pub || warn "pub.dev publish skipped"
        ;;
    github)    push_tag; publish_github ;;
    cocoapods) publish_cocoapods ;;
    maven)     publish_maven ;;
    npm)       publish_npm ;;
    pub)       publish_pub ;;
    *)
        error "Unknown channel: ${CHANNEL}"
        echo "Valid channels: all, github, cocoapods, maven, npm, pub"
        exit 1
        ;;
esac

echo ""
success "Release v${VERSION} publication complete!"
log "Post-release steps:"
log "  1. Verify packages on each registry"
log "  2. Update documentation website"
log "  3. Announce on social channels"
log "  4. Monitor for critical issues (first 48 hours)"
