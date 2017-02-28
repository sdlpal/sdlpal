#include <gtest/gtest.h>
extern "C"{
    #include "common.h"
    #include "palcommon.h"
    #include "text.h"
}

TEST(sdlpal, PALswprintf) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试",    test_buf, PAL_swprintf(test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls", L"测", L"试")));
}

TEST(sdlpal, PALswprintf2) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试 2",  test_buf, PAL_swprintf(test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls %d", L"测", L"试", 2)));
}

TEST(sdlpal, PALswprintf3) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试 3",  test_buf, PAL_swprintf(test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls %c", L"测", L"试", '3')));
}
