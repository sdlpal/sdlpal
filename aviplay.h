//
// Copyright (c) 2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//

#ifndef AVIPLAY_H
#define AVIPLAY_H

#if defined(_WIN32) && !defined(__WINRT__)
int PAL_PlayAVI(const char *szFilename);
#else
static inline int PAL_PlayAVI(const char *ignored) { return -1; }
#endif

#endif

