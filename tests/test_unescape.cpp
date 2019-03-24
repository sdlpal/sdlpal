#include <gtest/gtest.h>
extern "C"{
#include "common.h"
#include "palcommon.h"
#include "text.h"
}

TEST(sdlpal, PAL_UnescapeText) {
    WCHAR *t=PAL_UnescapeText(LR"(Sa\'\\\-b-\'t)");
    EXPECT_EQ(0, wcscmp( t, LR"(Sa'\-b't)"));
}
