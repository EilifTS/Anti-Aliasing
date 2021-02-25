#pragma once
#pragma comment(lib, "DirectML.lib")

#include <DirectML.h>
#include <array>
#include "graphics/internal/egx_common.h"

namespace egx
{
	struct DMLDims
	{
		UINT dims[4];
	};
	struct DMLFilterDims
	{
        UINT dims[4];
	};

    inline UINT64 DMLCalcBufferTensorSize(
        DML_TENSOR_DATA_TYPE dataType,
        UINT dimensionCount,
        _In_reads_(dimensionCount) const UINT* sizes,
        _In_reads_opt_(dimensionCount) const UINT* strides
    )
    {
        UINT elementSizeInBytes = 0;
        switch (dataType)
        {
        case DML_TENSOR_DATA_TYPE_FLOAT32:
        case DML_TENSOR_DATA_TYPE_UINT32:
        case DML_TENSOR_DATA_TYPE_INT32:
            elementSizeInBytes = 4;
            break;

        case DML_TENSOR_DATA_TYPE_FLOAT16:
        case DML_TENSOR_DATA_TYPE_UINT16:
        case DML_TENSOR_DATA_TYPE_INT16:
            elementSizeInBytes = 2;
            break;

        case DML_TENSOR_DATA_TYPE_UINT8:
        case DML_TENSOR_DATA_TYPE_INT8:
            elementSizeInBytes = 1;
            break;

        default:
            return 0; // Invalid data type
        }

        UINT64 minimumImpliedSizeInBytes = 0;
        if (!strides)
        {
            minimumImpliedSizeInBytes = sizes[0];
            for (UINT i = 1; i < dimensionCount; ++i)
            {
                minimumImpliedSizeInBytes *= sizes[i];
            }
            minimumImpliedSizeInBytes *= elementSizeInBytes;
        }
        else
        {
            UINT indexOfLastElement = 0;
            for (UINT i = 0; i < dimensionCount; ++i)
            {
                indexOfLastElement += (sizes[i] - 1) * strides[i];
            }

            minimumImpliedSizeInBytes = (indexOfLastElement + 1) * elementSizeInBytes;
        }

        // Round up to the nearest 4 bytes.
        minimumImpliedSizeInBytes = (minimumImpliedSizeInBytes + 3) & ~3ui64;

        return minimumImpliedSizeInBytes;
    }

    enum class Layout
    {
        NCHW,
        NHWC
    };

    // Given dimension sizes (in NCHW order), calculates the strides to achieve a desired layout.
    inline std::array<uint32_t, 4> CalculateStrides(
        Layout layout,
        std::array<uint32_t, 4> sizes,
        std::array<bool, 4> broadcast)
    {
        enum DML_ORDER { N, C, H, W };

        uint32_t n = broadcast[N] ? 1 : sizes[N];
        uint32_t c = broadcast[C] ? 1 : sizes[C];
        uint32_t h = broadcast[H] ? 1 : sizes[H];
        uint32_t w = broadcast[W] ? 1 : sizes[W];

        uint32_t nStride = 0, cStride = 0, hStride = 0, wStride = 0;

        switch (layout)
        {
        case Layout::NCHW:
            nStride = broadcast[N] ? 0 : c * h * w;
            cStride = broadcast[C] ? 0 : h * w;
            hStride = broadcast[H] ? 0 : w;
            wStride = broadcast[W] ? 0 : 1;
            break;

        case Layout::NHWC:
            nStride = broadcast[N] ? 0 : h * w * c;
            hStride = broadcast[H] ? 0 : w * c;
            wStride = broadcast[W] ? 0 : c;
            cStride = broadcast[C] ? 0 : 1;
            break;
        }

        return { nStride, cStride, hStride, wStride };
    }
}