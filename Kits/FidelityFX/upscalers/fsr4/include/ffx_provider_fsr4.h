#pragma once
#include "../../../api/internal/ffx_provider.h"
#include <string>

class ffxProvider_FSR4: public ffxProvider
{
public:
    ffxProvider_FSR4();
    virtual ~ffxProvider_FSR4() = default;

    virtual bool CanProvide(uint64_t type) const override;

    virtual bool IsSupported(void* device) const override;

    virtual uint64_t GetId() const override;

    virtual const char* GetVersionName() const override;

    virtual ffxReturnCode_t CreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, Allocator& alloc) const override;

    virtual ffxReturnCode_t DestroyContext(ffxContext* context, Allocator& alloc) const override;

    virtual ffxReturnCode_t Configure(ffxContext* context, const ffxConfigureDescHeader* desc) const override;

    virtual ffxReturnCode_t Query(ffxContext* context, ffxQueryDescHeader* desc) const override;

    virtual ffxReturnCode_t Dispatch(ffxContext* context, const ffxDispatchDescHeader* desc) const override;

    static ffxProvider_FSR4 Instance;
    
};
