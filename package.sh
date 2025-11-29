# NOTE: just a draft to remember how to package Qt app on Ubuntu

export LD_LIBRARY_PATH=~/Qt/6.3.0/gcc_64/lib

sudo apt install patchelf chrpath
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

~/tools/linuxdeploy/linuxdeploy-x86_64.AppImage --appdir hsmide --executable ./bin/hsm_ide --plugin qt