#!/usr/bin/env bash
# ============================================================================
#  LivePadRig - Build & Packaging Script (macOS)
#  Membangun plugin VST3 + AU (Component) Release lalu merapikan hasilnya
#  ke folder release/macos/ (.app-style bundle + optional .dmg)
#
#  Pemakaian (jalankan dari repo root, di macOS):
#    bash scripts/build_release_macos.sh
#    bash scripts/build_release_macos.sh --skip-build
#    bash scripts/build_release_macos.sh --dmg
#
#  Output:
#    release/macos/LivePadRig_macOS_v<ver>/   (folder portable)
#    release/macos/LivePadRig_macOS_v<ver>.dmg (jika --dmg)
# ============================================================================
set -euo pipefail

VERSION="1.3.1"
SKIP_BUILD=0
MAKE_DMG=0
for arg in "$@"; do
  case "$arg" in
    --skip-build) SKIP_BUILD=1 ;;
    --dmg) MAKE_DMG=1 ;;
    *) echo "Argumen tidak dikenal: $arg"; exit 1 ;;
  esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "=== LivePadRig macOS Release Build v$VERSION ==="
echo "Repo: $REPO_ROOT"

# ---------------------------------------------------------------------------
# 1) Pastikan JUCE tersedia & Projucer terbangun
# ---------------------------------------------------------------------------
if [ ! -d "JUCE/modules" ]; then
  echo ">>> JUCE tidak ditemukan. Cloning..."
  git clone --depth 1 https://github.com/juce-framework/JUCE.git
fi

# Injection path module (sama seperti workflow CI) + resave agar project
# Xcode selalu sinkron dengan LivePadRig.jucer.
if [ "$SKIP_BUILD" -eq 0 ]; then
  python3 - <<'PY'
import re
with open('LivePadRig.jucer', 'r') as f:
    data = f.read()
data = re.sub(r'(<MODULEPATH[^>]*path=")[^"]*(")', r'\g<1>JUCE/modules\2', data)
data = re.sub(r'(<MODULE [^>]*path=")[^"]*(")', r'\g<1>JUCE/modules\2', data)
with open('LivePadRig.jucer', 'w') as f:
    f.write(data)
PY

  PROJUCER="JUCE/extras/Projucer/Builds/MacOSX/build/Release/Projucer.app/Contents/MacOS/Projucer"
  if [ ! -x "$PROJUCER" ]; then
    echo ">>> Membangun Projucer..."
    (cd JUCE/extras/Projucer/Builds/MacOSX && xcodebuild -project Projucer.xcodeproj -configuration Release)
  fi
  "$PROJUCER" --resave LivePadRig.jucer

  echo ">>> Building VST3 & AU (Release)..."
  (cd Builds/MacOSX && xcodebuild -project LivePadRig.xcodeproj -configuration Release)
fi

BUILD_DIR="Builds/MacOSX/build/Release"
VST3_SRC="$BUILD_DIR/LivePadRig.vst3"
AU_SRC="$BUILD_DIR/LivePadRig.component"
[ -d "$VST3_SRC" ] || { echo "ERROR: VST3 tidak ditemukan di $VST3_SRC"; exit 1; }
[ -d "$AU_SRC" ]   || { echo "ERROR: AU Component tidak ditemukan di $AU_SRC"; exit 1; }

# ---------------------------------------------------------------------------
# 2) Rakit folder release/macos/<name>/
# ---------------------------------------------------------------------------
RELEASE_ROOT="release/macos"
PKG_NAME="LivePadRig_macOS_v${VERSION}"
PKG_DIR="$RELEASE_ROOT/$PKG_NAME"

echo ">>> Menyusun paket: $PKG_DIR"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/Audio" "$PKG_DIR/config"

# 2a) Plugin binaries
cp -R "$VST3_SRC" "$PKG_DIR/"
cp -R "$AU_SRC"   "$PKG_DIR/"

# 2b) Aset audio
if [ -d "assets/audio" ]; then
  cp -R assets/audio/* "$PKG_DIR/Audio/"
else
  echo "WARNING: assets/audio tidak ditemukan"
fi

# 2c) Konfigurasi default (rootSoundDir mengarah ke /Users/Shared/LivePadRig/Audio)
sed 's#__LIVEPADRIG_AUDIO_DIR__#/Users/Shared/LivePadRig/Audio#g' \
    config/default_settings.xml > "$PKG_DIR/config/default_settings.xml"

# 2d) Manual (jika ada)
[ -f "LIVEPAD RIG USER MANUAL.pdf" ] && cp "LIVEPAD RIG USER MANUAL.pdf" "$PKG_DIR/"

# 2e) README instalasi
cat > "$PKG_DIR/README.txt" <<EOF
LIVEPADRIG v${VERSION} - macOS
==============================

ISI PAKET
- LivePadRig.vst3          : plugin VST3
- LivePadRig.component     : plugin AU (Audio Unit)
- Audio/                   : aset pad & drum kit bawaan
- config/default_settings.xml : konfigurasi default

CARA PASANG (PORTABLE)
1. Salin LivePadRig.vst3     -> ~/Library/Audio/Plug-Ins/VST3/
   Salin LivePadRig.component-> ~/Library/Audio/Plug-Ins/Components/
   (atau ke /Library/... untuk semua user)
2. Salin isi folder Audio/   -> /Users/Shared/LivePadRig/Audio/
3. Salin config/default_settings.xml ->
   ~/Library/Application Support/LivePadRig/settings.xml
   (langkah 2-3 opsional; bisa juga lewat tombol LOCATE AUDIO di plugin)
4. Buka DAW, scan ulang plugin. Untuk AU: reset validasi via
   "killall -9 AudioComponentRegistrar" bila perlu.

CATATAN STATE v1.3
- Semua fader/DSP/preset/routing MIDI tersimpan otomatis dan dipulihkan
  saat aplikasi dibuka kembali.
- Dalam DAW, seluruh state ikut tersimpan di file project (VST3/AU state).
EOF

# ---------------------------------------------------------------------------
# 3) Optional: buat DMG
# ---------------------------------------------------------------------------
if [ "$MAKE_DMG" -eq 1 ]; then
  DMG_PATH="$RELEASE_ROOT/$PKG_NAME.dmg"
  echo ">>> Membuat DMG: $DMG_PATH"
  hdiutil create -volname "LivePadRig v${VERSION}" -srcfolder "$PKG_DIR" -ov -format UDZO "$DMG_PATH"
fi

echo ""
echo "=== SELESAI. Output di: $RELEASE_ROOT ==="
ls -la "$RELEASE_ROOT"
