set CL=/nologo /std:c++17 /W4 /WX /I ../../../OpenSource/utest /D_WINDOWS /EHsc 
cl abi.cpp
cl runtime_tests.cpp
cl runtime_tests_externalprovider.cpp

abi.exe --output=ffx-api-test_abi_report.xml
runtime_tests.exe --output=ffx-api-runtime_tests_report.xml
runtime_tests_externalprovider.exe --output=ffx-api-runtime_tests_externalprovider_report.xml
