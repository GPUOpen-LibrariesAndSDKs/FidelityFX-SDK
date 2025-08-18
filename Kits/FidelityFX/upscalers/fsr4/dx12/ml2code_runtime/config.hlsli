// Copyright(C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
#pragma once

#if !defined(ML2C_ENABLE_GLOBAL_BOUNDS)
#define ML2C_ENABLE_GLOBAL_BOUNDS 1
#endif

#if !defined(ML2C_ENABLE_SLICE_BOUNDS)
#define ML2C_ENABLE_SLICE_BOUNDS 1
#endif

#if !defined(ML2C_ENABLE_POISON)
#define ML2C_ENABLE_POISON 1
#endif

// Float OOB macros
#if !defined(ML2C_POISON_ON_TENSOR_OOB_LOAD_F) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_TENSOR_OOB_LOAD_F(p) if (any(or(p < 0, p >= t.logicalSize))) return 0.0/0.0;
#else
#define ML2C_POISON_ON_TENSOR_OOB_LOAD_F(p)
#endif

#if !defined(ML2C_POISON_ON_SLICE_OOB_LOAD_F) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_SLICE_OOB_LOAD_F(p) if (any(or(p < t.threadGroupSliceStart, p >= t.threadGroupSliceStart + t.threadGroupSliceSize))) return 0.0/0.0;
#else
#define ML2C_POISON_ON_SLICE_OOB_LOAD_F(p)
#endif

// Half unaligned macros
#if !defined(ML2C_POISON_ON_UNALIGNED_READS_H) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_READS_H(p) if (!IsDWordAligned(p)) return Unpack1h(0x7c00);
#else
#define ML2C_POISON_ON_UNALIGNED_READS_H(p)
#endif

#if !defined(ML2C_POISON_ON_UNALIGNED_WRITES_H) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_WRITES_H(p, v) if (!IsDWordAligned(p)) { p = AlignDownToDWord(p); v = Unpack1h(0x7c00); }
#else
#define ML2C_POISON_ON_UNALIGNED_WRITES_H(p, v)
#endif

// Int8/uint8 unaligned poisoning
#if !defined(ML2C_POISON_ON_UNALIGNED_READS_U8) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_READS_U8(p) if (!IsDWordAligned(p)) return 0xdeadbeef;
#else
#define ML2C_POISON_ON_UNALIGNED_READS_U8(p)
#endif

#if !defined(ML2C_POISON_ON_UNALIGNED_READS_I8) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_READS_I8(p) if (!IsDWordAligned(p)) return 0xdeadbeef;
#else
#define ML2C_POISON_ON_UNALIGNED_READS_I8(p)
#endif

#if !defined(ML2C_POISON_ON_UNALIGNED_WRITES_U8) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_WRITES_U8(p, v) if (!IsDWordAligned(p)) { p = AlignDownToDWord(p); v = 0xdeadbeef; }
#else
#define ML2C_POISON_ON_UNALIGNED_WRITES_U8(p, v)
#endif

#if !defined(ML2C_POISON_ON_UNALIGNED_WRITES_I8) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_WRITES_I8(p, v) if (!IsDWordAligned(p)) { p = AlignDownToDWord(p); v = 0xdeadbeef; }
#else
#define ML2C_POISON_ON_UNALIGNED_WRITES_I8(p, v)
#endif

// Int32 unaligned poisoning
#if !defined(ML2C_POISON_ON_UNALIGNED_READS_I32) && ML2C_ENABLE_POISON
#define ML2C_POISON_ON_UNALIGNED_READS_I32(p) if (!IsDWordAligned(p)) return 0xdeadbeef;
#else
#define ML2C_POISON_ON_UNALIGNED_READS_I32(p)
#endif
