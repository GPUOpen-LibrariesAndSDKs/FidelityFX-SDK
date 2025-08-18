# FidelityFX API Tests

## ABI Tests

These tests ensure the ABI as defined in the ffx_api headers does not change.

**Do not change these tests!**

Do add new tests when adding new structs: one for struct size, one for member offsets.

Do add new tests when adding new enums or enum values: one test case for each enum, one `EXPECT` for each enum value.

## Runtime Tests

These tests are to ensure basic runtime functionality of the provider selection and external/driver override mechanism.

They do not test effect functionality.

testffx.dll is a mock driver that supports the driver override interface. Source code for it is probably at http://isvgit.amd.com/criley/amdxc64_ffxmock.

amdxcffx64_mock.dll is a provider dll like the one shipped in the driver. It includes the FSR2 provider, although it has been renamed. Source code is found in [this commit](http://isvgit.amd.com/core-tech-group/sdks/fidelityfx-sdk/-/commit/025ca9f3d10f5d45dc7b08e9b5819706abb24a2d).
