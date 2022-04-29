#pragma once

#include "stdafx.h"

class FeaturesCollector
{
    ComPtr<ID3D12Device> _device = nullptr;

public:
    FeaturesCollector(ComPtr<ID3D12Device> pDevice)
        : _device(pDevice)
    {
    }

    void CollectFeatures(const std::string& fileName)
    {
        if (!_device)
            return;

        std::ostringstream ss;
        // architecture features
        D3D12_FEATURE_DATA_ARCHITECTURE1 arch = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));
        ss << "Hardware and driver support for cache-coherent UMA: " << arch.CacheCoherentUMA << std::endl;
        ss << "Hardware and driver support for UMA: " << arch.UMA << std::endl;
        ss << "Tile-base rendering support: " << arch.TileBasedRenderer << std::endl;
        ss << "Isolated MMU" << arch.IsolatedMMU << std::endl;
        ss << std::endl;

        // d3d12 options
        D3D12_FEATURE_DATA_D3D12_OPTIONS opts = {};
        ThrowIfFailed(
            _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)));
        ss << "Conservative Rasterization Tier: " << opts.ConservativeRasterizationTier << std::endl;
        ss << "Cross Adapter Row Major Texture Supported: " << opts.CrossAdapterRowMajorTextureSupported << std::endl;
        ss << "Cross Node Sharing Tier: " << opts.CrossNodeSharingTier << std::endl;
        ss << "Double Precision Float Shader Operations: " << opts.DoublePrecisionFloatShaderOps << std::endl;
        ss << "Min Precision Support: " << opts.MinPrecisionSupport << std::endl;
        ss << "Output Merger Logic Operations: " << opts.OutputMergerLogicOp << std::endl;
        ss << "PS Specified Stencil Ref Supported: " << opts.PSSpecifiedStencilRefSupported << std::endl;
        ss << "Resource Binding Tier: " << opts.ResourceBindingTier << std::endl;
        ss << "Resource Heap Tier: " << opts.ResourceHeapTier << std::endl;
        ss << "ROVs Supported: " << opts.ROVsSupported << std::endl;
        ss << "Standard Swizzle 64KB Supported: " << opts.StandardSwizzle64KBSupported << std::endl;
        ss << "Tiled Resources Tier: " << opts.TiledResourcesTier << std::endl;
        ss << "Typed UAV Load Additional Formats: " << opts.TypedUAVLoadAdditionalFormats << std::endl;
        ss << "VP And RT Array Index From Any Shader Feeding Rasterizer Supported Without GS Emulation: "
           << opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation << std::endl;
        ss << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS1 opts1 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &opts1,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1)));
        ss << "Wave Ops: " << opts1.WaveOps << std::endl;
        ss << "Wave Lane Count Min: " << opts1.WaveLaneCountMin << std::endl;
        ss << "Wave Lane Count Max: " << opts1.WaveLaneCountMax << std::endl;
        ss << "Total Lane Count: " << opts1.TotalLaneCount << std::endl;
        ss << "Expanded Compute Resource States: " << opts1.ExpandedComputeResourceStates << std::endl;
        ss << "Int64 Shader Ops: " << opts1.Int64ShaderOps << std::endl;
        ss << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS2 opts2 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &opts2,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2)));
        ss << "DepthBounds Test Supported: " << opts2.DepthBoundsTestSupported << std::endl;
        ss << "Programmable Sample Positions Tier: " << opts2.ProgrammableSamplePositionsTier << std::endl;
        
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 opts3 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &opts3,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3)));
        ss << "Copy Queue Timestamp Queries Supported: " << opts3.CopyQueueTimestampQueriesSupported << std::endl;
        ss << "Casting Fully Typed Format Supported: " << opts3.CastingFullyTypedFormatSupported << std::endl;
        ss << "WriteBufferImmediate Support Flags: " << opts3.WriteBufferImmediateSupportFlags << std::endl;
        ss << "View Instancing Tier: " << opts3.ViewInstancingTier << std::endl;
        ss << "Barycentrics Supported: " << opts3.BarycentricsSupported << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS4 opts4 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &opts4,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4)));
        ss << "MSAA 64KB Aligned Texture Supported: " << opts4.MSAA64KBAlignedTextureSupported << std::endl;
        ss << "Shared Resource Compatibility Tier: " << opts4.SharedResourceCompatibilityTier << std::endl;
        ss << "Native 16-Bit Shader Ops Supported: " << opts4.Native16BitShaderOpsSupported << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 opts5 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opts5,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)));
        ss << "SRV Only Tiled Resource Tier 3: " << opts5.SRVOnlyTiledResourceTier3 << std::endl;
        ss << "Render Passes Tier: " << opts5.RenderPassesTier << std::endl;
        ss << "Raytracing Tier: " << opts5.RaytracingTier << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS6 opts6 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &opts6,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6)));
        ss << "Additional Shading Rates Supported: " << opts6.AdditionalShadingRatesSupported << std::endl;
        ss << "Per-Primitive Shading Rate Supported With Viewport Indexing: " << opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing << std::endl;
        ss << "Variable Shading Rate Tier: " << opts6.VariableShadingRateTier << std::endl;
        ss << "Shading Rate Image Tile Size: " << opts6.ShadingRateImageTileSize << std::endl;
        ss << "Background Processing Supported: " << opts6.BackgroundProcessingSupported << std::endl;

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 opts7 = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &opts7,
                                                   sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7)));
        
        ss << "Mesh Shader Tier: " << opts7.MeshShaderTier << std::endl;
        ss << "Sampler Feedback Tier: " << opts7.SamplerFeedbackTier << std::endl;

        // virtual adressation support
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT vasupport = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &vasupport,
                                                   sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT)));
        ss << "Max GPU Virtual Address Bits Per Process: " << vasupport.MaxGPUVirtualAddressBitsPerProcess << std::endl;
        ss << "Max GPU Virtual Address Bits Per Resource: " << vasupport.MaxGPUVirtualAddressBitsPerResource << std::endl;
        ss << std::endl;

        std::ofstream file{fileName};
        file << ss.str();
        file.close();
    }
};
