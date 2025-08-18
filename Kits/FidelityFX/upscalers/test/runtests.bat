set CL=/nologo /std:c++17 /W4 /WX /I ../../../OpenSource/utest /D_WINDOWS /EHsc 
cl abi.cpp
cl runtime_tests_upscale_no_ctx.cpp

abi.exe --output=ffx-api-test_abi_upscale_report.xml
runtime_tests_upscale_no_ctx.exe --output=ffx-api-runtime_tests_upscale_no_ctx_report.xml
