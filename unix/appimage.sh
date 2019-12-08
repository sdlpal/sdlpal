#!/bin/bash

########################################################################
# Package the binaries built on Travis-CI as an AppImage
# By palxex 2019
# For more information, see http://appimage.org/
########################################################################

export ARCH=$(arch)

APP=SDLPal

mkdir -p $APP.AppDir
cd $APP.AppDir

mkdir -p ./usr/{bin,share/applications,share/icons/hicolor/256x256/apps}
cp $TRAVIS_BUILD_DIR/unix/sdlpal ./usr/bin/
cp $TRAVIS_BUILD_DIR/unix/sdlpal.desktop ./usr/share/applications/
convert $TRAVIS_BUILD_DIR/Icon.png -resize 256x256 sdlpal.png
cp sdlpal.png ./usr/share/icons/hicolor/256x256/apps/

cat > AppRun <<\EOF
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
cd "${HERE}/usr/bin/"
exec "${HERE}/usr/bin/sdlpal" "$@"
EOF

chmod +x AppRun

########################################################################
# AppDir complete
# Now packaging it as an AppImage
########################################################################

cd .. # Go out of AppImage
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
./linuxdeployqt-continuous-x86_64.AppImage $APP.AppDir/usr/share/applications/*.desktop -appimage

########################################################################
# move to deploy folder, waiting upload
########################################################################

mv SDLPal*.AppImage* $TRAVIS_BUILD_DIR/deploy
