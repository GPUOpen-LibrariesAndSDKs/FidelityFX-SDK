#include "ml2code_runtime/tensor_int8.hlsli"
#include "ml2code_runtime/tensor_float16.hlsli"


template<typename BatchType, uint BatchSize, typename TensorType>
class TensorBatchOperation
{
    void Execute(TensorType tensor, int3 po)
    {
        return;
    }

    uint GetBatchSize(){return BatchSize;}
};

template<typename BatchType, uint BatchSize, typename TensorType>
class StoreConstBatchOperation : TensorBatchOperation<BatchType, BatchSize, TensorType>
{
    void Execute(TensorType tensor, int3 po)
    {
        return;
    }
    //TODO Figure out why we can't just use it via inheritance
    uint GetBatchSize()
    {
        return BatchSize;
    }
};

static const uint DefaultDword = 0;

template<uint BatchSize, typename TensorType>
class StoreDefaultConstBatchOperation : TensorBatchOperation<uint, BatchSize, TensorType>
{
    uint DWord;
    void Execute(TensorType tensor, int3 po)
    {
        uint4 ZeroDwords = { DWord, DWord, DWord, DWord };
        uint storeIndex = tensor.OffsetOf(po);
        tensor.storage.Store4(storeIndex, ZeroDwords);
    }
    //TODO Figure out why we can't just use it via inheritance
    uint GetBatchSize()
    {
        return BatchSize;
    }
};

//Partial Specialization for int8
template<typename TensorType>
class StoreConstBatchOperation<int8_t4_packed, 16, TensorType>
{
    void Execute(TensorType tensor, int3 po)
    {
        uint4 ZeroDwords = { pack_clamp_s8(int4(0, 0, 0, 0)), pack_clamp_s8(int4(0, 0, 0, 0)), pack_clamp_s8(int4(0, 0, 0, 0)), pack_clamp_s8(int4(0, 0, 0, 0)) };
        uint storeIndex = tensor.OffsetOf(po);
        tensor.storage.Store4(storeIndex, ZeroDwords);
    }
    uint GetBatchSize()
    {
        return 16;
    }
};

//Partial Specialization for halfs
template<typename TensorType>
struct StoreConstBatchOperation<half4, 8, TensorType>
{
    void Execute(TensorType tensor, int3 po)
    {
        half4 zeroValues = { .0f, .0f, .0f, .0f };
        uint4 ZeroDwords = uint4(Pack4h(zeroValues), Pack4h(zeroValues));
        uint storeIndex = tensor.OffsetOf(po);
        //Store4hA(tensor, po, ZeroDwords);
        tensor.storage.Store4(storeIndex, ZeroDwords);
    }
    uint GetBatchSize()
    {
        return 8;
    }
};

template<typename TensorType, typename BatchOp>
void ResetPaddingFromBorderPixels(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, BatchOp resetOp, int2 stride = int2(1, 1))
{
    for (int y = 0; y < stride.y; ++y)
    {
        for (int x = 0; x < stride.x; ++x)
        {
            int3 pos = pThreadBase + int3(x, y, 0);
            if (any(pos.xy > tensor.logicalSize.xy))
                continue;
            //Reset top padding to 0.
            if (vertical && (pos.y == 0) && pos.y < tensor.logicalSize.y)
            {
                //Reset top left padding to 0.
                if (pos.x == 0)
                {
                    [unroll]
                    for (uint padY = 0; padY < tensor.paddingBegin.y; ++padY)
                    {
                        [unroll]
                        for (uint padX = 0; padX < tensor.paddingBegin.x; ++padX)
                        {
                            [unroll]
                            for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                            {
                                resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x + padX, -tensor.paddingBegin.y + padY, c));
                            }
                        }
                    }
                }

                [unroll]
                for (uint pad = 0; pad < tensor.paddingBegin.y; ++pad)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(0, -tensor.paddingBegin.y + pad, c));
                    }
                }


                //Reset top right padding to 0.
                if (pos.x == tensor.logicalSize.x - 1 && pos.x < tensor.logicalSize.x)
                {
                    [unroll]
                    for (uint padY = 0; padY < tensor.paddingBegin.y; ++padY)
                    {
                        [unroll]
                        for (uint padX = 0; padX < tensor.paddingEnd.x; ++padX)
                        {
                            [unroll]
                            for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                            {
                                resetOp.Execute(tensor, pos + int3(1 + padX, -tensor.paddingBegin.y + padY, c));
                            }
                        }
                    }
                }
            }

            //Reset left padding to 0.
            if (horizontal && (pos.x == 0) && pos.x < tensor.logicalSize.x)
            {
                [unroll]
                for (uint pad = 0; pad < tensor.paddingBegin.x; ++pad)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x + pad, 0, c));
                    }
                }
            }

            //Reset right padding to 0.
            if (horizontal && (pos.x == tensor.logicalSize.x - 1) && pos.x < tensor.logicalSize.x)
            {
                [unroll]
                for (uint pad = 0; pad < tensor.paddingEnd.x; ++pad)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(1 + pad, 0, c));
                    }
                }
            }


            //Reset bottom padding to 0.
            if (vertical && (pos.y == tensor.logicalSize.y - 1) && pos.y < tensor.logicalSize.y)
            {
                //Reset bottom left padding to 0.
                if (pos.x == 0 && pos.x < tensor.logicalSize.x)
                {
                    [unroll]
                    for (uint padY = 0; padY < tensor.paddingEnd.y; ++padY)
                    {
                        [unroll]
                        for (uint padX = 0; padX < tensor.paddingBegin.x; ++padX)
                        {
                            [unroll]
                            for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                            {
                                resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x + padX, 1 + padY, c));
                            }
                        }
                    }
                }

                [unroll]
                for (uint pad = 0; pad < tensor.paddingEnd.y; ++pad)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(0, 1 + pad, c));
                    }
                }


                //Reset bottom right padding to 0.
                if (pos.x == tensor.logicalSize.x - 1)
                {
                    [unroll]
                    for (uint padY = 0; padY < tensor.paddingEnd.y; ++padY)
                    {
                        [unroll]
                        for (uint padX = 0; padX < tensor.paddingEnd.x; ++padX)
                        {
                            [unroll]
                            for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                            {
                                resetOp.Execute(tensor, pos + int3(1 + padX, 1 + padY, c));
                            }
                        }
                    }
                }
            }
        }
    }
}

template<typename TensorType, typename BatchOp>
void ResetPaddingFromBorderRegion(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, BatchOp resetOp, int2 stride = int2(1, 1))
{
    for (int y = 0; y < stride.y; ++y)
    {
        for (int x = 0; x < stride.x; ++x)
        {
            int3 pos = pThreadBase + int3(x, y, 0);
            if (any(pos.xy > tensor.logicalSize.xy))
                continue;

            //Reset top padding to 0.
            if (vertical && (pos.y < tensor.paddingBegin.y))
            {
                //Reset top left padding to 0.
                if (pos.x < tensor.paddingBegin.x)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x, -tensor.paddingBegin.y, c));
                    }
                }

                [unroll]
                for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                {
                    resetOp.Execute(tensor, pos + int3(0, -tensor.paddingBegin.y, c));
                }


                //Reset top right padding to 0.
                if (pos.x >= tensor.logicalSize.x - tensor.paddingEnd.x)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(tensor.paddingEnd.x, -tensor.paddingBegin.y, c));
                    }
                }
            }

            //Reset left padding to 0.
            if (horizontal && (pos.x < tensor.paddingBegin.x))
            {
                [unroll]
                for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                {
                    resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x, 0, c));
                }
            }

            //Reset right padding to 0.
            if (horizontal && (pos.x >= tensor.logicalSize.x - tensor.paddingEnd.x))
            {
                [unroll]
                for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                {
                    resetOp.Execute(tensor, pos + int3(tensor.paddingEnd.x, 0, c));
                }
            }


            //Reset bottom padding to 0.
            if (vertical && (pos.y >= tensor.logicalSize.y - tensor.paddingEnd.y))
            {
                //Reset bottom left padding to 0.
                if (pos.x < tensor.paddingBegin.x)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(-tensor.paddingBegin.x, tensor.paddingEnd.y, c));
                    }
                }

                [unroll]
                for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                {
                    resetOp.Execute(tensor, pos + int3(0, tensor.paddingEnd.y, c));
                }


                //Reset bottom right padding to 0.
                if (pos.x >= tensor.logicalSize.x - tensor.paddingEnd.x)
                {
                    [unroll]
                    for (uint c = 0; c < tensor.logicalSize.z; c += resetOp.GetBatchSize())
                    {
                        resetOp.Execute(tensor, pos + int3(tensor.paddingEnd.x, tensor.paddingEnd.y, c));
                    }
                }
            }

        }
    }
}

template<typename TensorType, typename BatchOp>
void ResetPadding(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, BatchOp resetOp, int2 stride = int2(1, 1))
{
    //ResetPaddingFromBorderPixels(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
    ResetPaddingFromBorderRegion(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
}

template<typename TensorType>
void ResetPaddingInt8(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, int2 stride = int2(1, 1))
{
    StoreConstBatchOperation < int8_t4_packed, 16, TensorType > resetOp;
    ResetPaddingFromBorderRegion(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
    //ResetPaddingFromBorderPixels(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
}

template<typename TensorType>
void ResetPaddingFloat16(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, int2 stride = int2(1, 1))
{
    StoreConstBatchOperation<half4, 8, TensorType> resetOp;
    ResetPaddingFromBorderRegion(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
    //ResetPaddingFromBorderPixels(tensor, pThreadBase, vertical, horizontal, resetOp, stride);
}

template<typename TensorType, typename BatchOp>
void ResetPaddingSeparate(TensorType tensor, int3 pThreadBase, bool vertical, bool horizontal, BatchOp resetOp, int2 stride = int2(1, 1))
{
    //Imitate the threadbase coordinates without having to dispatch threads for the inner part of the tensor.
    //All of these are on the border region (but still inside) of the tensor and never exceed its boundaries.
    const uint2 totalPadding = tensor.paddingBegin.xy + tensor.paddingEnd.xy;//Only x/y padding so far
    const uint numTop = tensor.logicalSize.x * tensor.paddingBegin.y;
    const uint numCenter = (tensor.logicalSize.y - totalPadding.y) * (totalPadding.x);
    const uint numBottom = tensor.logicalSize.x * tensor.paddingEnd.y;

    //Early out necessary if amount of padding is not a multiple of 32
    if (pThreadBase.x >= numTop + numCenter + numBottom)
        return;

    int3 borderThreadBase /*= pThreadBase*/;
    if (pThreadBase.x < numTop)
    {
        borderThreadBase = int3(pThreadBase.x % tensor.logicalSize.x, pThreadBase.x / tensor.logicalSize.x, 0);
    }
    else if (pThreadBase.x < numTop + numCenter)
    {
        const int centerX = pThreadBase.x - numTop;
        borderThreadBase = int3(centerX % totalPadding.x, tensor.paddingBegin.y + centerX / totalPadding.x, 0);
        if (borderThreadBase.x >= tensor.paddingBegin.x)
            borderThreadBase.x = (tensor.logicalSize.x - tensor.paddingEnd.x) + (borderThreadBase.x - tensor.paddingBegin.x); //
    }
    else
    {
        const int bottomX = pThreadBase.x - numTop - numCenter;
        borderThreadBase = int3(bottomX % tensor.logicalSize.x, tensor.logicalSize.y - tensor.paddingEnd.y + bottomX / tensor.logicalSize.x, 0);
    }

    ResetPaddingFromBorderRegion(tensor, borderThreadBase, vertical, horizontal, resetOp, stride);

}
