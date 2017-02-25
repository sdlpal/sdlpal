#include <gtest/gtest.h>

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#ifdef __MINGW32__
#include <windows.h>
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    return main(__argc, __argv);
}
#endif