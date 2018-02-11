#!/usr/bin/env sh
js-beautify sdlpal.js > temp.js 
mv temp.js sdlpal.js
patch < mouse.patch