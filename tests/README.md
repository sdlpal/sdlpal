How to write unit tests
=======================

***SDLPAL*** adopts the [Google C++ testing framework](https://github.com/google/googletest) for unit tests.

Writing a unit test using Google C++ testing framework is easy as 1-2-3:


Step 1
------

Create a new c++ code file containing test codes in the **`tests`** directory, including necessary header files such that the stuff your test logic needs is declared. If you dont't know which appropriate header to be included, just follow the code snippet here, which should be sufficient in most cases: 

```c++
// Include the Google C++ testing framework
#include <gtest/gtest.h>
// Inlucde the functions to be tested
extern "C"{
    #include "main.h"
}
```

### Notes for file naming convention

To avoid any potential file naming conflicts, please name your test code files starting as ***test_***.


Step 2
------

Use the **`TEST`** macro to define your tests. **`TEST`** has two parameters: the first is test case name and the second is the test name. After using the macro, you should define your test logic between a pair of braces. You can use a bunch of macros such as ***EXPECT_EQ*** to indicate the success or failure of a test. The following code snippet demonstrates how to create a test named ***PALswprintf*** in test case ***sdlpal*** to test the ***`PAL_swprintf`*** function:

```c++
TEST(sdlpal, PALswprintf) {
    WCHAR test_buf[256];
    EXPECT_EQ(0, wcsncmp( L"测试",    test_buf, PAL_swprintf(test_buf, sizeof(test_buf)/sizeof(WCHAR), L"%ls%ls", L"测", L"试")));
}
```

As tests are grouped into test cases in Google Test, please put logically related tests into the same test case. The test case name and the test name should both be valid C++ identifiers, and you should avoid to use underscore (_) in these names. Google Test guarantees that each test you define is run exactly once, but it makes no guarantee on the order the tests are executed. Therefore, you should write your tests in such a way that their results don't depend on their order.


Step 3
------

When you finished writting test codes, you can use ***`make check`*** to compile and run tests. If you are using ***Visual Studio*** under Windows, please make sure you've actived the **Test** configuration before you launch the program.

### Special notes for ***Visual Studio*** users

You should put all test files into the ***Test Files*** filter, and make sure you've excluded them for compilation in configurations other than **Test**.
