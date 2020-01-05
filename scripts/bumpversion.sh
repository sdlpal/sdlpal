NEWY=$(date "+%Y")
! [ -z $NEWY ] && find . \( -iname '*.h' -or -iname '*.c' -or -iname '*.cpp' -or -iname '*.hpp' -or -iname '*.m' -or -iname '*.mm' -or -iname '*.java' \) -exec gsed -i "s/Copyright (c) 2011-[0-9]*, SDLPAL development team/Copyright (c) 2011-$NEWY, SDLPAL development team/g" {} \;
