/* Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

/// Specifies a particular block on the GPU to gather counters for.
enum class AmdExtGpuBlock : UINT32
{
    Cpf    = 0x0,
    Ia     = 0x1,
    Vgt    = 0x2,
    Pa     = 0x3,
    Sc     = 0x4,
    Spi    = 0x5,
    Sq     = 0x6,
    Sx     = 0x7,
    Ta     = 0x8,
    Td     = 0x9,
    Tcp    = 0xA,
    Tcc    = 0xB,
    Tca    = 0xC,
    Db     = 0xD,
    Cb     = 0xE,
    Gds    = 0xF,
    Srbm   = 0x10,
    Grbm   = 0x11,
    GrbmSe = 0x12,
    Rlc    = 0x13,
    Dma    = 0x14,
    Mc     = 0x15,
    Cpg    = 0x16,
    Cpc    = 0x17,
    Wd     = 0x18,
    Tcs    = 0x19,
    Atc    = 0x1A,
    AtcL2  = 0x1B,
    McVmL2 = 0x1C,
    Ea     = 0x1D,
    Rpb    = 0x1E,
    Rmi    = 0x1F,
    Umcch  = 0x20,
    Ge     = 0x21,
    Gl1a   = 0x22,
    Gl1c   = 0x23,
    Gl1cg  = 0x24,
    Gl2a   = 0x25,
    Gl2c   = 0x26,
    Cha    = 0x27,
    Chc    = 0x28,
    Chcg   = 0x29,
    Gus    = 0x2A,
    Gcr    = 0x2B,
    Ph     = 0x2C,
    UtcL1  = 0x2D,
    Ge1    = Ge,
    GeDist = 0x2E,
    GeSe   = 0x2F,
    DfMall = 0x30,
    SqWgp  = 0x31,
    Pc     = 0x32,
    Gl1xa  = 0x33,
    Gl1xc  = 0x34,
    Wgs    = 0x35,
    EaCpwd = 0x36,
    EaSe   = 0x37,
    RlcUser = 0x38,
    Count
};

/// Mask values ORed together to choose which shader stages a performance experiment should sample.
enum AmdExtPerfExperimentShaderFlags
{
    PerfShaderMaskPs  = 0x01,
    PerfShaderMaskVs  = 0x02,
    PerfShaderMaskGs  = 0x04,
    PerfShaderMaskEs  = 0x08,
    PerfShaderMaskHs  = 0x10,
    PerfShaderMaskLs  = 0x20,
    PerfShaderMaskCs  = 0x40,
    PerfShaderMaskAll = 0x7f,
};

/// Specifies the point in the GPU pipeline where an action should take place.
enum AmdExtHwPipePoint : UINT32
{
    HwPipeTop              = 0x0,                   ///< Earliest possible point in the GPU pipeline (CP PFP).
    HwPipeBottom           = 0x7,                   ///< All prior GPU work (graphics, compute, or BLT) has completed.
};

/// Reports properties of a specific GPU block required for interpreting performance experiment data from that block.
/// See @ref AmdExtPerfExperimentProperties.
struct AmdExtGpuBlockPerfProperties
{
    bool   available;                ///< If performance data is available for this block.
    UINT32 instanceCount;            ///< How many instances of this block are in the device.
    UINT32 maxEventId;               ///< Maximum event ID for this block.
    UINT32 maxGlobalOnlyCounters;    ///< Number of counters available only for global counts.
    UINT32 maxCounters;              ///< Total counters available.
    UINT32 maxSpmCounters;           ///< Counters available for streaming only.
};

/// Reports performance experiment capabilities of a device.  Returned by GetPerfExperimentProperties().
struct AmdExtPerfExperimentProperties
{
    union
    {
        struct
        {
            UINT32 counters    : 1;     ///< Device supports performance counters.
            UINT32 threadTrace : 1;     ///< Device supports thread traces.
            UINT32 spmTrace    : 1;     ///< Device supports streaming perf monitor
                                        ///  traces.
            UINT32 reserved    : 29;    ///< Reserved for future use.
        };
        UINT32 u32All;                  ///< Flags packed as 32-bit uint.
    } features;                         ///< Performance experiment feature flags.

    size_t           maxSqttSeBufferSize; ///< SQTT buffer size per shader engine.
    UINT32           shaderEngineCount;   ///< Number of shader engines.
    AmdExtGpuBlockPerfProperties blocks[size_t(AmdExtGpuBlock::Count)]; ///< Reports availability and
                                                                                     ///  properties of each device
                                                                                     ///  block.
};

/// Specifies basic type of sample to perform - either a normal set of "global" perf counters, or a trace consisting
/// of SQ thread trace and/or streaming performance counters.
enum class AmdExtGpaSampleType : UINT32
{
    Cumulative = 0x0,  ///< One 64-bit result will be returned per performance counter representing the cumulative delta
                       ///  for that counter over the sample period.  Cumulative samples must begin and end in the same
                       ///  command buffer.
    Trace      = 0x1,  ///< A GPU memory buffer will be filled with hw-specific SQ thread trace and/or streaming
                       ///  performance counter data.  Trace samples may span multiple command buffers.
    Timing     = 0x2,  ///< Two 64-bit results will be recorded in beginTs and endTs to gather timestamp data.
    Query      = 0x3,  ///< A set of 11 pipeline stats will be collected.
    None       = 0xf,  ///< No profile will be done.
};

/// Specifies a specific performance counter to be sampled with IAmdExtGpaSession::BeginSample() and
/// IAmdExtGpaSession::EndSample().
///
/// This identifies a specific counter in a particular HW block instance, e.g., TCC instance 3 counter #19.  It is up
/// to the client to know the meaning of a particular counter.
struct AmdExtPerfCounterId
{
    AmdExtGpuBlock block; ///< Which GPU block to reference (e.g., CB, DB, TCC).
    UINT32   instance;    ///< Which instance of the specified GPU block to sample.
                          ///  (this number is returned per-block in the @ref AmdExtGpuBlockPerfProperties structure).
                          ///  There is no shortcut to get results for all instances of block in the whole chip, the
                          ///  client must explicitly sample each instance and sum the results.
    UINT32   eventId;     ///< Counter ID to sample.  Note that the meaning of a particular eventId for a block can
                          ///  change between chips.
};

/// Specifies a specific performance counter to be sampled with IAmdExtGpaSession::BeginSample() and
/// IAmdExtGpaSession::EndSample().
///
/// This identifies a specific counter in a particular HW block instance, e.g., TCC instance 3 counter #19.  It is up
/// to the client to know the meaning of a particular counter.
struct AmdExtPerfCounterId2
{
    AmdExtGpuBlock block; ///< Which GPU block to reference (e.g., CB, DB, TCC).
    UINT32   instance;    ///< Which instance of the specified GPU block to sample.
                          ///  (this number is returned per-block in the @ref AmdExtGpuBlockPerfProperties structure).
                          ///  There is no shortcut to get results for all instances of block in the whole chip, the
                          ///  client must explicitly sample each instance and sum the results.
    UINT32   eventId;     ///< Counter ID to sample.  Note that the meaning of a particular eventId for a block can
                          ///  change between chips.

    // Some blocks have additional per-counter controls. They must be properly programmed when adding counters for
    // the relevant blocks. It's recommended to zero them out when not in use.
    union
    {
        struct
        {
            UINT32 eventQualifier;   ///< The DF counters have an event-specific qualifier bitfield.
        } df;

        struct
        {
            UINT16 eventThreshold;   ///< Threshold value for those UMC counters having event-specific threshold.
            UINT8  eventThresholdEn; ///< Threshold enable (0 for disabled,1 for <threshold,2 for >threshold).
            UINT8  rdWrMask;         ///< Read/Write mask select (1 for Read, 2 for Write).
        } umc;

        UINT32 rs64Cntl; ///< CP blocks CPG and CPC have events that can be further filtered for processor events

        UINT32 u32All; ///< Union value for copying, must be increased in size if any element of the union exceeds
    } subConfig;
};

/// Input structure for CmdBeginGpuProfilerSample.
///
/// Defines a set of global performance counters and/or SQ thread trace data to be sampled.
struct AmdExtGpaSampleConfig
{
    /// Selects what type of data should be gathered for this sample.  This can either be _cumulative_ to gather
    /// simple deltas for the specified set of perf counters over the sample period, or it can be _trace_ to generate
    /// a blob of RGP-formatted data containing SQ thread trace and/or streaming performance monitor data.
    AmdExtGpaSampleType type;

    union
    {
        struct
        {
            UINT32 sampleInternalOperations      : 1;  ///< Include BLTs and internal driver operations in the
                                                       ///  results.
            UINT32 cacheFlushOnCounterCollection : 1;  ///< Insert cache flush and invalidate events before and
                                                       ///  after every sample.
            UINT32 sqShaderMask                  : 1;  ///< Whether or not the contents of sqShaderMask are valid.
            UINT32 sqWgpShaderMask               : 1;  ///< Whether or not the contents of sqWgpShaderMask are valid..
            UINT32 reserved                      : 28; ///< Reserved for future use
        };
        UINT32 u32All;                                 ///< Bit flags packed as uint32.
    } flags;                                           ///< Bit flags controlling sample operation for all sample
                                                       ///  types.

    AmdExtPerfExperimentShaderFlags sqShaderMask;      ///< Indicates which hardware shader stages should be
                                                       ///  sampled. Only valid if flags.sqShaderMask is set to 1.

    struct
    {
        /// Number of entries in pIds.
        UINT32 numCounters;

        /// List of performance counters to be gathered for a sample.  If the sample type is _cumulative_ this will
        /// result in "global" perf counters being sampled at the beginning of the sample period; if the sample type
        /// is _trace_ this will result in SPM data being added to the sample's resulting RGP blob.
        ///
        /// Note that it is up to the client to respect the hardware counter limit per block.  This can be
        /// determined by the maxGlobalOnlyCounters, maxGlobalSharedCounters, and maxSpmCounters fields of
        /// @ref AmdExtGpuBlockPerfProperties.
        const AmdExtPerfCounterId* pIds;

        /// Period for SPM sample collection in cycles.  Only relevant for _trace_ samples.
        UINT32  spmTraceSampleInterval;

        /// Maximum amount of GPU memory in bytes this sample can allocate for SPM data.  Only relevant for _trace_
        /// samples.
        UINT64 gpuMemoryLimit;
    } perfCounters;  ///< Performance counter selection (valid for both _cumulative_ and _trace_ samples).

    struct
    {
        union
        {
            struct
            {
                UINT32 enable                   : 1;  ///< Include SQTT data in the trace.
                UINT32 supressInstructionTokens : 1;  ///< Prevents capturing instruction-level SQTT tokens,
                                                      ///  significantly reducing the amount of sample data.
                UINT32 reserved                 : 30; ///< Reserved for future use.
            };
            UINT32 u32All;                            ///< Bit flags packed as uint32.
        } flags;                                      ///< Bit flags controlling SQTT samples.
        UINT32 seMask;                                ///< Mask that determines which specific SEs to run thread trace on.
                                                      ///  If 0, all SEs are enabled
        UINT64 gpuMemoryLimit;                        ///< Maximum amount of GPU memory in bytes this sample can allocate for the SQTT
                                                      ///  buffer.  If 0, allocate maximum size to prevent dropping tokens toward the
                                                      ///  end of the sample.

    } sqtt;  ///< SQ thread trace configuration (only valid for _trace_ samples).

    struct
    {
        AmdExtHwPipePoint preSample;   ///< The point in the GPU pipeline where the begin timestamp should take place.
        AmdExtHwPipePoint postSample;  ///< The point in the GPU pipeline where the end timestamp should take place.
    } timing;   ///< Timestamp configuration. (only valid for timing samples)

    AmdExtPerfExperimentShaderFlags sqWgpShaderMask;   ///< Indicates which hardware shader stages should be
                                                       ///  sampled. Only valid if flags.sqWgpShaderMask is set to 1.
};

/// Input structure for CmdBeginGpuProfilerSample.
///
/// Defines a set of global performance counters and/or SQ thread trace data to be sampled.
struct AmdExtGpaSampleConfig2
{
    /// Selects what type of data should be gathered for this sample.  This can either be _cumulative_ to gather
    /// simple deltas for the specified set of perf counters over the sample period, or it can be _trace_ to generate
    /// a blob of RGP-formatted data containing SQ thread trace and/or streaming performance monitor data.
    AmdExtGpaSampleType type;

    union
    {
        struct
        {
            UINT32 sampleInternalOperations : 1;  ///< Include BLTs and internal driver operations in the
                                                       ///  results.
            UINT32 cacheFlushOnCounterCollection : 1;  ///< Insert cache flush and invalidate events before and
                                                       ///  after every sample.
            UINT32 sqShaderMask : 1;  ///< Whether or not the contents of sqShaderMask are valid.
            UINT32 sqWgpShaderMask : 1;  ///< Whether or not the contents of sqWgpShaderMask are valid..
            UINT32 reserved : 28; ///< Reserved for future use
        };
        UINT32 u32All;                                 ///< Bit flags packed as uint32.
    } flags;                                           ///< Bit flags controlling sample operation for all sample
                                                       ///  types.

    AmdExtPerfExperimentShaderFlags sqShaderMask;      ///< Indicates which hardware shader stages should be
                                                       ///  sampled. Only valid if flags.sqShaderMask is set to 1.

    struct
    {
        /// Number of entries in pIds.
        UINT32 numCounters;

        /// List of performance counters to be gathered for a sample.  If the sample type is _cumulative_ this will
        /// result in "global" perf counters being sampled at the beginning of the sample period; if the sample type
        /// is _trace_ this will result in SPM data being added to the sample's resulting RGP blob.
        ///
        /// Note that it is up to the client to respect the hardware counter limit per block.  This can be
        /// determined by the maxGlobalOnlyCounters, maxGlobalSharedCounters, and maxSpmCounters fields of
        /// @ref AmdExtGpuBlockPerfProperties.
        const AmdExtPerfCounterId2* pIds;

        /// Period for SPM sample collection in cycles.  Only relevant for _trace_ samples.
        UINT32  spmTraceSampleInterval;

        /// Maximum amount of GPU memory in bytes this sample can allocate for SPM data.  Only relevant for _trace_
        /// samples.
        UINT64 gpuMemoryLimit;
    } perfCounters;  ///< Performance counter selection (valid for both _cumulative_ and _trace_ samples).

    struct
    {
        union
        {
            struct
            {
                UINT32 enable : 1;  ///< Include SQTT data in the trace.
                UINT32 supressInstructionTokens : 1;  ///< Prevents capturing instruction-level SQTT tokens,
                                                      ///  significantly reducing the amount of sample data.
                UINT32 reserved : 30; ///< Reserved for future use.
            };
            UINT32 u32All;                            ///< Bit flags packed as uint32.
        } flags;                                      ///< Bit flags controlling SQTT samples.
        UINT32 seMask;                                ///< Mask that determines which specific SEs to run thread trace on.
                                                      ///  If 0, all SEs are enabled
        UINT64 gpuMemoryLimit;                        ///< Maximum amount of GPU memory in bytes this sample can allocate for the SQTT
                                                      ///  buffer.  If 0, allocate maximum size to prevent dropping tokens toward the
                                                      ///  end of the sample.

    } sqtt;  ///< SQ thread trace configuration (only valid for _trace_ samples).

    struct
    {
        AmdExtHwPipePoint preSample;   ///< The point in the GPU pipeline where the begin timestamp should take place.
        AmdExtHwPipePoint postSample;  ///< The point in the GPU pipeline where the end timestamp should take place.
    } timing;   ///< Timestamp configuration. (only valid for timing samples)

    AmdExtPerfExperimentShaderFlags sqWgpShaderMask;   ///< Indicates which hardware shader stages should be
                                                       ///  sampled. Only valid if flags.sqWgpShaderMask is set to 1.
};

enum class AmdExtDeviceClockMode : UINT32
{
    Default       = 0,  ///< Device clocks and other power settings are restored to default.
    Query         = 1,  ///< Queries the current device clock ratios. Leaves the clock mode of the device unchanged.
    Profiling     = 2,  ///< Scale down from peak ratio. Clocks are set to a constant amount which is
                        ///  known to be power and thermal sustainable. The engine/memory clock ratio
                        ///  will be kept the same as much as possible.
    MinimumMemory = 3,  ///< Memory clock is set to the lowest available level. Engine clock is set to
                        ///  thermal and power sustainable level.
    MinimumEngine = 4,  ///< Engine clock is set to the lowest available level. Memory clock is set to
                        ///  thermal and power sustainable level.
    Peak          = 5,  ///< Clocks set to maximum when possible. Fan set to maximum. Note: Under power
                        ///  and thermal constraints device will clock down.
    Count
};

/// Specifies input argument to IAmdExtGpaInterface::SetClockMode. The caller can read the clock ratios the
/// device is currently running by querying using the mode AmdExtDeviceClockMode::DeviceClockModeQuery.
struct AmdExtSetClockModeOutput
{
    float memoryClockRatioToPeak;  ///< Ratio of current mem clock to peak clock as obtained from
                                   ///  DeviceProperties::maxMemClock.
    float engineClockRatioToPeak;  ///< Ratio of current gpu core clock to peak clock as obtained from
                                   ///  DeviceProperties::maxGpuClock.
};
