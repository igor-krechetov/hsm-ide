#!/usr/bin/env bash

build_dir="$1"
install_dir="$2"
version="$3"
appdir_root="$build_dir/AppDir"

rm -Rfv $appdir_root
DESTDIR=$appdir_root cmake --install build

linuxdeploy-x86_64.AppImage \
    --appdir $appdir_root \
    --executable $appdir_root/usr/bin/hsm_ide \
    --desktop-file $appdir_root/usr/share/applications/hsm_ide.desktop \
    --plugin qt

# Keep only xcb platform plugin
find $appdir_root/usr/plugins/platforms -type f ! -name 'libqxcb.so' -delete

# Remove all input context plugins if not needed
rm -rfv $appdir_root/usr/plugins/platforminputcontexts/*

# Remove all xcbglintegrations if not needed (for OpenGL, keep if you use it)
rm -rfv $appdir_root/usr/plugins/xcbglintegrations/*

# Remove all platformthemes if not needed
# rm -rfv $appdir_root/usr/plugins/platformthemes/*

# Remove all imageformats except what you use (e.g., keep only jpeg and png)
find $appdir_root/usr/plugins/imageformats -type f ! -name 'libqjpeg.so' ! -name 'libqpng.so' -delete

# Remove unused translations
rm -rf $appdir_root/usr/translations/*

# Remove unused Qt modules
rm -f $appdir_root/usr/lib/libQt6Qml*.so.6
rm -f $appdir_root/usr/lib/libQt6Quick.so.6
rm -f $appdir_root/usr/lib/libQt6Pdf.so.6
rm -f $appdir_root/usr/lib/libQt6VirtualKeyboard.so.6

# package application
appimagetool-x86_64.AppImage $appdir_root $install_dir/hsmide-$version-linux-x64.AppImage