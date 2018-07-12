//
//  video_glsl.h
//  Pal
//
//  Created by palxex on 27/2/18.
//

#ifndef video_glsl_h
#define video_glsl_h

#include "mini_glloader.h"
#include "main.h"

PAL_C_LINKAGE_BEGIN

extern SDL_Texture *VIDEO_GLSL_CreateTexture(int width, int height);
extern void VIDEO_GLSL_RenderCopy();

extern void VIDEO_GLSL_Init();
extern void VIDEO_GLSL_Setup();

extern void VIDEO_GLSL_Destroy();

PAL_C_LINKAGE_END

#endif /* video_glsl_h */
