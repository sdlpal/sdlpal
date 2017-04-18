//
// Copyright (c) 2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//

#ifndef AVIPLAY_H
#define AVIPLAY_H

#ifdef WIN32
int PAL_PlayAVI(const char *szFilename);
#else
inline PAL_PlayAVI(const char *ignored) { return -1; }
#endif

#endif

