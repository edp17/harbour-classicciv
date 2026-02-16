Name:       harbour-classicciv
Version:    0.1.0
Release:    1
Summary:    DOS runner for Civilization (requires original game files)
License:    GPL-2.0-or-later
Group:      Applications/Games
URL:        https://example.invalid
Source0:    %{name}-%{version}.tar.bz2

BuildRequires: cmake
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: sailfishsilica-qt5-devel

# DOSBox Staging build deps
BuildRequires: meson
BuildRequires: ninja
BuildRequires: pkgconfig(sdl2)
BuildRequires: pkgconfig(opus)
BuildRequires: pkgconfig(ogg)

# For building opusfile (autotools)
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: make
BuildRequires: pkgconfig
BuildRequires: pkgconfig(openssl)
BuildRequires: git
BuildRequires: patchelf

Requires: sailfishsilica-qt5
#Requires: fluidsynth

# We require the package, not the SONAME() virtual provide
%define __requires_exclude ^(libfluidsynth\\.so\\(\\)\\(64bit\\)|libiir\\.so\\.1\\(\\)\\(64bit\\)|libslirp\\.so\\.0\\(\\)\\(64bit\\)|libmt32emu\\.so\\(\\)\\(64bit\\)|libsdl2_net\\.so\\(\\)\\(64bit\\))$

%description
A Sailfish OS launcher that runs Civilization (DOS) using a bundled DOSBox build.
The user must provide original Civ DOS files (CIV.EXE and data) in the app data folder.

%prep
%setup -q -n %{name}

mkdir -p 3rdparty
rm -rf 3rdparty/dosbox-staging 3rdparty/opusfile

echo "edp17"
ls -la %{_sourcedir}

tar -xzf 3rdparty/tarballs/dosbox-staging-0.82.1.tar.gz
DOSBOX_TOP="$(ls -d dosbox-staging* | head -n 1)"
mv "$DOSBOX_TOP" 3rdparty/dosbox-staging
test -f 3rdparty/dosbox-staging/meson.build

tar -xzf 3rdparty/tarballs/opusfile-0.12.tar.gz
mv opusfile-0.12 3rdparty/opusfile
test -f 3rdparty/opusfile/configure

%build
pushd 3rdparty/opusfile
DEPS_PREFIX="%{_builddir}/%{name}-%{version}/_deps/usr"
mkdir -p "$DEPS_PREFIX"

./configure --prefix="$DEPS_PREFIX" --host=%{_target_platform}
make %{?_smp_mflags}
make install
popd

# Build DOSBox Staging, pointing pkg-config to the private prefix
pushd 3rdparty/dosbox-staging
rm -rf build
test -f meson.build

export PKG_CONFIG_PATH="$DEPS_PREFIX/lib/pkgconfig:$DEPS_PREFIX/share/pkgconfig:$PKG_CONFIG_PATH"

meson setup build . --buildtype=release --prefix=/usr -Duse_opengl=false -Duse_alsa=false
ninja -C build
popd

# Build your Qt launcher
%cmake .
%cmake_build

%install
%cmake_install
# Create SONAME symlinks for bundled libs (inside buildroot)
LIBDIR=%{buildroot}%{_libdir}/%{name}
install -d "$LIBDIR"

# iir
ln -sf libiir.so.1.9.3 "$LIBDIR/libiir.so.1"
ln -sf libiir.so.1     "$LIBDIR/libiir.so"

# Clean up accidental symlinks created in /usr/
rm -f %{buildroot}/usr/libiir.so %{buildroot}/usr/libiir.so.1
rm -f %{buildroot}%{_prefix}/libiir.so %{buildroot}%{_prefix}/libiir.so.1

# slirp
ln -sf libslirp.so.0.4.0 "$LIBDIR/libslirp.so.0"
ln -sf libslirp.so.0     "$LIBDIR/libslirp.so"

# Bundle opusfile runtime library into our private lib dir
install -d %{buildroot}%{_libdir}/%{name}

# Locate the built opusfile shared library in our private deps prefix
OPUSFILE_SRC="$(find %{_builddir}/%{name}-%{version}/_deps/usr -type f -name 'libopusfile.so.*' | head -n 1)"
echo "OPUSFILE_SRC=$OPUSFILE_SRC"
test -n "$OPUSFILE_SRC"
test -f "$OPUSFILE_SRC"

# Copy the real file (e.g. libopusfile.so.0.12.0)
install -m 0644 "$OPUSFILE_SRC" %{buildroot}%{_libdir}/%{name}/

# Ensure SONAME symlink exists (what rpm dependency wants)
BASENAME="$(basename "$OPUSFILE_SRC")"
ln -sf "$BASENAME" %{buildroot}%{_libdir}/%{name}/libopusfile.so.0

# Install dosbox binary into app share
install -d %{buildroot}%{_datadir}/%{name}/bin
install -m 0755 3rdparty/dosbox-staging/build/dosbox %{buildroot}%{_datadir}/%{name}/bin/dosbox

cat > %{buildroot}%{_datadir}/%{name}/bin/dosbox-wrapper.sh <<'EOF'
#!/bin/sh
export LD_LIBRARY_PATH="/usr/lib64/harbour-classicciv:${LD_LIBRARY_PATH}"
exec "/usr/share/harbour-classicciv/bin/dosbox" "$@"
EOF
chmod 0755 %{buildroot}%{_datadir}/%{name}/bin/dosbox-wrapper.sh

# Ensure dosbox finds our bundled .so's in /usr/lib64/harbour-classicciv
patchelf --set-rpath %{_libdir}/%{name} %{buildroot}%{_datadir}/%{name}/bin/dosbox
# Verify
readelf -d %{buildroot}%{_datadir}/%{name}/bin/dosbox | grep -E 'RPATH|RUNPATH' || true

# (Optional but recommended) Install license texts
# install -d %{buildroot}%{_datadir}/%{name}/licenses
# install -m 0644 3rdparty/dosbox-staging/COPYING %{buildroot}%{_datadir}/%{name}/licenses/COPYING.DOSBOX

%files
%defattr(-,root,root,-)
%{_bindir}/harbour-classicciv
%{_datadir}/harbour-classicciv/
%{_datadir}/applications/harbour-classicciv.desktop
%{_datadir}/icons/hicolor/256x256/apps/harbour-classicciv.png

%{_libdir}/harbour-classicciv/libiir.so.1.9.3
%{_libdir}/harbour-classicciv/libiir.so.1
%{_libdir}/harbour-classicciv/libiir.so

%{_libdir}/harbour-classicciv/libslirp.so.0.4.0
%{_libdir}/harbour-classicciv/libslirp.so.0
%{_libdir}/harbour-classicciv/libslirp.so

%{_libdir}/harbour-classicciv/libfluidsynth.so
%{_libdir}/harbour-classicciv/libmt32emu.so
%{_libdir}/harbour-classicciv/libsdl2_net.so

%{_libdir}/%{name}/libopusfile.so.*

