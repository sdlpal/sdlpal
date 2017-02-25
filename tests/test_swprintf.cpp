#include <gtest/gtest.h>
extern "C"{
    #include "common.h"
    #include "palcommon.h"
    #include "text.h"
}

#define swprintf_wrapper( buffer, count, format, ...) ( \
PAL_swprintf( buffer, count, format, ##__VA_ARGS__ ), \
buffer \
)

TEST(sdlpal, PAL_swprintf) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试",    swprintf_wrapper( test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls", L"测", L"试"), wcslen(L"测试")));
}

TEST(sdlpal, PAL_swprintf2) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试 2",  swprintf_wrapper( test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls %d", L"测", L"试", 2), wcslen(L"测试 2")));
}

TEST(sdlpal, PAL_swprintf3) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试 3",  swprintf_wrapper( test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls %c", L"测", L"试", '3'), wcslen(L"测试 3")));
}
