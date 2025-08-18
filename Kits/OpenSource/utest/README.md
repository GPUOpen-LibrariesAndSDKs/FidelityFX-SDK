# 🧪 utest.h

[![Actions Status](https://github.com/sheredom/utest.h/workflows/CMake/badge.svg)](https://github.com/sheredom/utest.h/actions)
[![Build status](https://ci.appveyor.com/api/projects/status/i2u3a0pw4pxprrcv?svg=true)](https://ci.appveyor.com/project/sheredom/utest-h)
[![Sponsor](https://img.shields.io/badge/💜-sponsor-blueviolet)](https://github.com/sponsors/sheredom)

A simple one header solution to unit testing for C/C++.

## Usage

Just `#include "utest.h"` in your code!

The current supported platforms are Linux, macOS and Windows.

The current supported compilers are gcc, clang, MSVC's cl.exe, and clang-cl.exe.

It also works with tcc but with a caveat: the latest release of the tcc compiler (version 0.9.27) lacks a feature for UTEST to work. Make sure to use a tcc that is patched with the constructor attribute extension. Recent Ubuntu and Debian Linux distros ship tcc with that patch already included. If you compile tcc yourself, use the trunk version and it will work as expected.

## Command Line Options

utest.h supports some command line options:

* `--help` to output the help message
* `--filter=<filter>` will filter the test cases to run (useful for re-running one
  particular offending test case).
* `--list-tests` will list testnames, one per line. Output names can be passed to `--filter`.
* `--output=<output>` will output an xunit XML file with the test results (that
  Jenkins, travis-ci, and appveyor can parse for the test results).
* `--enable-mixed-units` will enable the per-test output to contain mixed units (s/ms/us/ns).
* `--random-order[=<seed>]` will randomize the order that the tests are ran in. If the optional <seed> argument is not provided, then a random starting seed is used.

## Design

UTest is a single header library to enable all the fun of unit testing in C and
C++. The library has been designed to provide an output similar to Google's
googletest framework:

```
[==========] Running 1 test cases.
[ RUN      ] foo.bar
[       OK ] foo.bar (631ns)
[==========] 1 test cases ran.
[  PASSED  ] 1 tests.
```

## UTEST_MAIN

In one C or C++ file, you must call the macro UTEST_MAIN:

```c
UTEST_MAIN()
```

This will call into utest.h, instantiate all the testcases and run the unit test
framework.

Alternatively, if you want to write your own main and call into utest.h, you can
instead, in one C or C++ file call:

```c
UTEST_STATE();
```

And then when you are ready to call into the utest.h framework do:

```c
int main(int argc, const char *const argv[]) {
  // do your own thing
  return utest_main(argc, argv);
}
```

## Define a Testcase

To define a test case to run, you can do the following;

```c
#include "utest.h"

UTEST(foo, bar) {
  ASSERT_TRUE(1);
}
```

The UTEST macro takes two parameters - the first being the set that the test
case belongs to, the second being the name of the test. This allows tests to be
grouped for convenience.

## Define a Fixtured Testcase

A fixtured testcase is one in which there is a struct that is instantiated that
can be shared across multiple testcases.

```c
struct MyTestFixture {
  char c;
  int i;
  float f;
};

UTEST_F_SETUP(MyTestFixture) {
  utest_fixture->c = 'a';
  utest_fixture->i = 42;
  utest_fixture->f = 3.14f;

  // we can even assert and expect in setup!
  ASSERT_EQ(42, utest_fixture->i);
  EXPECT_TRUE(true);
}

UTEST_F_TEARDOWN(MyTestFixture) {
  // and also assert and expect in teardown!
  ASSERT_EQ(13, utest_fixture->i);
}

UTEST_F(MyTestFixture, a) {
  utest_fixture->i = 13;
  // teardown will succeed because i is 13...
}

UTEST_F(MyTestFixture, b) {
  utest_fixture->i = 83;
  // teardown will fail because i is not 13!
}
```

Some things to note that were demonstrated above:
* We have this new implicit variable within our macros - utest_fixture. This is
  a pointer to the struct you decided as your fixture (so MyTestFixture in the
  above code).
* Instead of specifying a testcase set (like we do with the UTEST macro), we
  instead specify the name of the fixture struct we are using.
* Every fixture has to have a `UTEST_F_SETUP` and `UTEST_F_TEARDOWN` macro -
  even if they do nothing in the body.
* Multiple testcases (UTEST_F's) can use the same fixture.
* You can use EXPECT_* and ASSERT_* macros within the body of both the fixture's
  setup and teardown macros.

## Define an Indexed Testcase

Sometimes you want to use the same fixture _and_ testcase repeatedly, but
perhaps subtly change one variable within. This is where indexed testcases come
in.

```c
struct MyTestIndexedFixture{
  bool x;
  bool y;
};

UTEST_I_SETUP(MyTestIndexedFixture) {
  if (utest_index < 30) {
    utest_fixture->x = utest_index & 1;
    utest_fixture->y = (utest_index + 1) & 1;
  }
}

UTEST_I_TEARDOWN(MyTestIndexedFixture) {
  EXPECT_LE(0, utest_index);
}

UTEST_I(MyTestIndexedFixture, a, 2) {
  ASSERT_TRUE(utest_fixture->x | utest_fixture->y);
}

UTEST_I(MyTestIndexedFixture, b, 42) {
  // this will fail when the index is >= 30
  ASSERT_TRUE(utest_fixture->x | utest_fixture->y);
}
```

Note:
* We use UTEST_I_* as the prefix for the setup and teardown functions now.
* We use UTEST_I to declare the testcases.
* We have access to a new variable utest_index in our setup and teardown
  functions, that we can use to slightly vary our fixture.
* We provide a number as the third parameter of the UTEST_I macro - this is the
  number of times we should run the test case for that index. It must be a
  literal.

## Testing Macros

Matching what googletest has, we provide two variants of each of the error
checking conditions - ASSERTs and EXPECTs. If an ASSERT fails, the test case
will cease execution, and utest.h will continue with the next test case to be
run. If an EXPECT fails, the remainder of the test case will still be executed,
allowing for further checks to be carried out.

We currently provide the following macros to be used within UTESTs:

### ASSERT_TRUE(x)

Asserts that x evaluates to true (EG. non-zero).

```c
UTEST(foo, bar) {
  int i = 1;
  ASSERT_TRUE(i);  // pass!
  ASSERT_TRUE(42); // pass!
  ASSERT_TRUE(0);  // fail!
}
```

### ASSERT_FALSE(x)

Asserts that x evaluates to false (EG. zero).

```c
UTEST(foo, bar) {
  int i = 0;
  ASSERT_FALSE(i); // pass!
  ASSERT_FALSE(1); // fail!
}
```

### ASSERT_EQ(x, y)

Asserts that x and y are equal.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 42;
  ASSERT_EQ(a, b);     // pass!
  ASSERT_EQ(a, 42);    // pass!
  ASSERT_EQ(42, b);    // pass!
  ASSERT_EQ(42, 42);   // pass!
  ASSERT_EQ(a, b + 1); // fail!
}
```

### ASSERT_NE(x, y)

Asserts that x and y are not equal.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  ASSERT_NE(a, b);   // pass!
  ASSERT_NE(a, 27);  // pass!
  ASSERT_NE(69, b);  // pass!
  ASSERT_NE(42, 13); // pass!
  ASSERT_NE(a, 42);  // fail!
}
```

### ASSERT_LT(x, y)

Asserts that x is less than y.

```c
UTEST(foo, bar) {
  int a = 13;
  int b = 42;
  ASSERT_LT(a, b);   // pass!
  ASSERT_LT(a, 27);  // pass!
  ASSERT_LT(27, b);  // pass!
  ASSERT_LT(13, 42); // pass!
  ASSERT_LT(b, a);   // fail!
}
```

### ASSERT_LE(x, y)

Asserts that x is less than or equal to y.

```c
UTEST(foo, bar) {
  int a = 13;
  int b = 42;
  ASSERT_LE(a, b);   // pass!
  ASSERT_LE(a, 27);  // pass!
  ASSERT_LE(a, 13);  // pass!
  ASSERT_LE(27, b);  // pass!
  ASSERT_LE(42, b);  // pass!
  ASSERT_LE(13, 13); // pass!
  ASSERT_LE(13, 42); // pass!
  ASSERT_LE(b, a);   // fail!
}
```

### ASSERT_GT(x, y)

Asserts that x is greater than y.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  ASSERT_GT(a, b);   // pass!
  ASSERT_GT(a, 27);  // pass!
  ASSERT_GT(27, b);  // pass!
  ASSERT_GT(42, 13); // pass!
  ASSERT_GT(b, a);   // fail!
}
```

### ASSERT_GE(x, y)

Asserts that x is greater than or equal to y.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  ASSERT_GE(a, b);   // pass!
  ASSERT_GE(a, 27);  // pass!
  ASSERT_GE(a, 13);  // pass!
  ASSERT_GE(27, b);  // pass!
  ASSERT_GE(42, b);  // pass!
  ASSERT_GE(13, 13); // pass!
  ASSERT_GE(42, 13); // pass!
  ASSERT_GE(b, a);   // fail!
}
```

### ASSERT_STREQ(x, y)

Asserts that the strings x and y are equal.

```c
UTEST(foo, bar) {
  char* a = "foo";
  char* b = "bar";
  ASSERT_STREQ(a, a); // pass!
  ASSERT_STREQ(b, b); // pass!
  ASSERT_STREQ(a, b); // fail!
}
```

### ASSERT_STRNE(x, y)

Asserts that the strings x and y are not equal.

```c
UTEST(foo, bar) {
  char* a = "foo";
  char* b = "bar";
  ASSERT_STRNE(a, b); // pass!
  ASSERT_STRNE(a, a); // fail!
}
```

### ASSERT_STRNEQ(x, y)

Asserts that the strings x and y are equal up to the length of the string x.

```c
UTEST(foo, bar) {
  char* a = "foobar";
  char* b = "foo";
  ASSERT_STRNEQ(a, a); // pass!
  ASSERT_STRNEQ(b, b); // pass!
  ASSERT_STRNEQ(a, b); // pass!
}
```

### ASSERT_STRNNE(x, y)

Asserts that the strings x and y are not equal up to the length of the string x.

```c
UTEST(foo, bar) {
  char* a = "foobar";
  char* b = "bar";
  ASSERT_STRNNE(a, b); // pass!
  ASSERT_STRNNE(a, a); // fail!
}
```

### ASSERT_NEAR(x, y, epsilon)

Asserts that the floating-point values x and y are within epsilon distance of
each other.

```c
UTEST(foo, bar) {
  float a = 42.0f;
  float b = 42.01f;
  ASSERT_NEAR(a, b, 0.01f);  // pass!
  ASSERT_NEAR(a, b, 0.001f); // fail!
}
```

### ASSERT_EXCEPTION(x, exception_type)

Asserts that exception_type will be thrown when code x is executed.

```cpp
void foo(int bar) {
  if (bar == 1)
    throw std::range_error;
}

UTEST(foo, bar) {
  ASSERT_EXCEPTION(foo(1), std::range_error); // pass!
  ASSERT_EXCEPTION(foo(2), std::range_error); // fail!
  ASSERT_EXCEPTION(foo(1), std::exception);   // fail!
}
```

### EXPECT_TRUE(x)

Expects that x evaluates to true (i.e. non-zero).

```c
UTEST(foo, bar) {
  int i = 1;
  EXPECT_TRUE(i);  // pass!
  EXPECT_TRUE(42); // pass!
  EXPECT_TRUE(0);  // fail!
}
```

### EXPECT_FALSE(x)

Expects that x evaluates to false (i.e. zero).

```c
UTEST(foo, bar) {
  int i = 0;
  EXPECT_FALSE(i); // pass!
  EXPECT_FALSE(1); // fail!
}
```

### EXPECT_EQ(x, y)

Expects that x and y are equal.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 42;
  EXPECT_EQ(a, b);     // pass!
  EXPECT_EQ(a, 42);    // pass!
  EXPECT_EQ(42, b);    // pass!
  EXPECT_EQ(42, 42);   // pass!
  EXPECT_EQ(a, b + 1); // fail!
}
```

### EXPECT_NE(x, y)

Expects that x and y are not equal.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  EXPECT_NE(a, b);   // pass!
  EXPECT_NE(a, 27);  // pass!
  EXPECT_NE(69, b);  // pass!
  EXPECT_NE(42, 13); // pass!
  EXPECT_NE(a, 42);  // fail!
}
```

### EXPECT_LT(x, y)

Expects that x is less than y.

```c
UTEST(foo, bar) {
  int a = 13;
  int b = 42;
  EXPECT_LT(a, b);   // pass!
  EXPECT_LT(a, 27);  // pass!
  EXPECT_LT(27, b);  // pass!
  EXPECT_LT(13, 42); // pass!
  EXPECT_LT(b, a);   // fail!
}
```

### EXPECT_LE(x, y)

Expects that x is less than or equal to y.

```c
UTEST(foo, bar) {
  int a = 13;
  int b = 42;
  EXPECT_LE(a, b);   // pass!
  EXPECT_LE(a, 27);  // pass!
  EXPECT_LE(a, 13);  // pass!
  EXPECT_LE(27, b);  // pass!
  EXPECT_LE(42, b);  // pass!
  EXPECT_LE(13, 13); // pass!
  EXPECT_LE(13, 42); // pass!
  EXPECT_LE(b, a);   // fail!
}
```

### EXPECT_GT(x, y)

Expects that x is greater than y.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  EXPECT_GT(a, b);   // pass!
  EXPECT_GT(a, 27);  // pass!
  EXPECT_GT(27, b);  // pass!
  EXPECT_GT(42, 13); // pass!
  EXPECT_GT(b, a);   // fail!
}
```

### EXPECT_GE(x, y)

Expects that x is greater than or equal to y.

```c
UTEST(foo, bar) {
  int a = 42;
  int b = 13;
  EXPECT_GE(a, b);   // pass!
  EXPECT_GE(a, 27);  // pass!
  EXPECT_GE(a, 13);  // pass!
  EXPECT_GE(27, b);  // pass!
  EXPECT_GE(42, b);  // pass!
  EXPECT_GE(13, 13); // pass!
  EXPECT_GE(42, 13); // pass!
  EXPECT_GE(b, a);   // fail!
}
```

### EXPECT_STREQ(x, y)

Expects that the strings x and y are equal.

```c
UTEST(foo, bar) {
  char* a = "foo";
  char* b = "bar";
  EXPECT_STREQ(a, a); // pass!
  EXPECT_STREQ(b, b); // pass!
  EXPECT_STREQ(a, b); // fail!
}
```

### EXPECT_STRNE(x, y)

Expects that the strings x and y are not equal.

```c
UTEST(foo, bar) {
  char* a = "foo";
  char* b = "bar";
  EXPECT_STRNE(a, b); // pass!
  EXPECT_STRNE(a, a); // fail!
}
```

### EXPECT_STRNEQ(x, y)

Expects that the strings x and y are equal up to the length of the string x.

```c
UTEST(foo, bar) {
  char* a = "foobar";
  char* b = "foo";
  EXPECT_STRNEQ(a, a); // pass!
  EXPECT_STRNEQ(b, b); // pass!
  EXPECT_STRNEQ(a, b); // pass!
}
```

### EXPECT_STRNNE(x, y)

Expects that the strings x and y are not equal up to the length of the string x.

```c
UTEST(foo, bar) {
  char* a = "foobar";
  char* b = "bar";
  EXPECT_STRNNE(a, b); // pass!
  EXPECT_STRNNE(a, a); // fail!
}
```

### EXPECT_NEAR(x, y, epsilon)

Expects that the floating-point values x and y are within epsilon distance of
each other.

```c
UTEST(foo, bar) {
  float a = 42.0f;
  float b = 42.01f;
  EXPECT_NEAR(a, b, 0.01f);  // pass!
  EXPECT_NEAR(a, b, 0.001f); // fail!
}
```

### EXPECT_EXCEPTION(x, exception_type)

Expects that exception_type will be thrown when code x is executed.

```cpp
void foo(int bar) {
  if (bar == 1)
    throw std::range_error;
}

UTEST(foo, bar) {
  EXPECT_EXCEPTION(foo(1), std::range_error); // pass!
  EXPECT_EXCEPTION(foo(2), std::range_error); // fail!
  EXPECT_EXCEPTION(foo(1), std::exception);   // fail!
}
```

### EXPECT_EXCEPTION_WITH_MESSAGE(x, exception_type, exception_message)

Expects that exception_type will be thrown with message exception_message when code x is executed.

```cpp
void foo(int bar) {
  if (bar == 1)
    throw std::range_error("bad bar");
}

UTEST(foo, bar) {
  EXPECT_EXCEPTION_WITH_MESSAGE(foo(1), std::range_error, "bad bar"); // pass!
  EXPECT_EXCEPTION_WITH_MESSAGE(foo(2), std::range_error, "bad bar2"); // fail!
  EXPECT_EXCEPTION_WITH_MESSAGE(foo(1), std::exception, "bad bar");   // fail!
}
```

### UTEST_SKIP(msg)

This macro lets you mark a test case as being skipped - eg. that the test case
is not to be executed. The test will stop running as you execute the macro,
report the `msg` as the reason for the skipped, and mark the test as
_'skipped'_. These will be reported at the end of execution before failures, and
skipped test cases will **not** cause the process to exit with a non-zero code.

```c
UTEST(foo, bar) {
  UTEST_SKIP("Need to implement this test!");
}
```

### Testing macros with custom message

In addition, to give the possibility of having custom messages in the fault
tests, all macros can be used with a suffix called "_MSG", which receives an
extra parameter, which is the string with the custom message to print in case
of failure.

For example:

```c
UTEST(foo, bar) {
  int i = 1;
  EXPECT_TRUE_MSG(i, "custom message");  // pass!
  EXPECT_TRUE_MSG(42, "custom message"); // pass!
  EXPECT_TRUE_MSG(0, "custom message");  // fail! (with the following output)
}
```

```
test.cpp:42: Failure
  Expected : true
    Actual : false
   Message : custom message
[  FAILED  ] foo.bar (8086ns)
```

## Types Supported for Checks

The library supports asserting on any builtin integer, floating-point, or
pointer type.

## License

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
