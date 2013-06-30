#pragma once

#include <Windows.h>
#include <wrl\client.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d2d1_1.h>
#include <dwrite_1.h>

#include <strsafe.h>

#include <memory>
#include <string>
#include <cstdio>
#include <cstdint>
#include <ios>
#include <iomanip>
#include <sstream>
#include <vector>
#include <utility>
#include <algorithm>
#include <exception>

#include "DirectXHelper.h"

namespace DX
{
    // Helper function to turn a DXGI_FORMAT value into a unicode string. An unknown value will return DXGI_FORMAT_??? or ??? (depending on the value of includePrefix).
    // format - The format value to transform.
    // includePrefix - If set to false, the "DXGI_FORMAT_" prefix will not be included (e.g. DXGI_FORMAT_B8G8R8A8_UNORM would just be B8G8R8A8_UNORM). 
    inline std::wstring DXGIFormatString(_In_ DXGI_FORMAT format, _In_ bool includePrefix = false)
    {
        std::wstring str;
        switch (format)
        {
        case DXGI_FORMAT_UNKNOWN:
            str.append(L"DXGI_FORMAT_UNKNOWN");
            break;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            str.append(L"DXGI_FORMAT_R32G32B32A32_TYPELESS");
            break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            str.append(L"DXGI_FORMAT_R32G32B32A32_FLOAT");
            break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            str.append(L"DXGI_FORMAT_R32G32B32A32_UINT");
            break;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            str.append(L"DXGI_FORMAT_R32G32B32A32_SINT");
            break;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            str.append(L"DXGI_FORMAT_R32G32B32_TYPELESS");
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            str.append(L"DXGI_FORMAT_R32G32B32_FLOAT");
            break;
        case DXGI_FORMAT_R32G32B32_UINT:
            str.append(L"DXGI_FORMAT_R32G32B32_UINT");
            break;
        case DXGI_FORMAT_R32G32B32_SINT:
            str.append(L"DXGI_FORMAT_R32G32B32_SINT");
            break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            str.append(L"DXGI_FORMAT_R16G16B16A16_TYPELESS");
            break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            str.append(L"DXGI_FORMAT_R16G16B16A16_FLOAT");
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            str.append(L"DXGI_FORMAT_R16G16B16A16_UNORM");
            break;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            str.append(L"DXGI_FORMAT_R16G16B16A16_UINT");
            break;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            str.append(L"DXGI_FORMAT_R16G16B16A16_SNORM");
            break;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            str.append(L"DXGI_FORMAT_R16G16B16A16_SINT");
            break;
        case DXGI_FORMAT_R32G32_TYPELESS:
            str.append(L"DXGI_FORMAT_R32G32_TYPELESS");
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            str.append(L"DXGI_FORMAT_R32G32_FLOAT");
            break;
        case DXGI_FORMAT_R32G32_UINT:
            str.append(L"DXGI_FORMAT_R32G32_UINT");
            break;
        case DXGI_FORMAT_R32G32_SINT:
            str.append(L"DXGI_FORMAT_R32G32_SINT");
            break;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            str.append(L"DXGI_FORMAT_R32G8X24_TYPELESS");
            break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            str.append(L"DXGI_FORMAT_D32_FLOAT_S8X24_UINT");
            break;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            str.append(L"DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS");
            break;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            str.append(L"DXGI_FORMAT_X32_TYPELESS_G8X24_UINT");
            break;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            str.append(L"DXGI_FORMAT_R10G10B10A2_TYPELESS");
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            str.append(L"DXGI_FORMAT_R10G10B10A2_UNORM");
            break;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            str.append(L"DXGI_FORMAT_R10G10B10A2_UINT");
            break;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            str.append(L"DXGI_FORMAT_R11G11B10_FLOAT");
            break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            str.append(L"DXGI_FORMAT_R8G8B8A8_TYPELESS");
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            str.append(L"DXGI_FORMAT_R8G8B8A8_UNORM");
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_R8G8B8A8_UNORM_SRGB");
            break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            str.append(L"DXGI_FORMAT_R8G8B8A8_UINT");
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            str.append(L"DXGI_FORMAT_R8G8B8A8_SNORM");
            break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            str.append(L"DXGI_FORMAT_R8G8B8A8_SINT");
            break;
        case DXGI_FORMAT_R16G16_TYPELESS:
            str.append(L"DXGI_FORMAT_R16G16_TYPELESS");
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
            str.append(L"DXGI_FORMAT_R16G16_FLOAT");
            break;
        case DXGI_FORMAT_R16G16_UNORM:
            str.append(L"DXGI_FORMAT_R16G16_UNORM");
            break;
        case DXGI_FORMAT_R16G16_UINT:
            str.append(L"DXGI_FORMAT_R16G16_UINT");
            break;
        case DXGI_FORMAT_R16G16_SNORM:
            str.append(L"DXGI_FORMAT_R16G16_SNORM");
            break;
        case DXGI_FORMAT_R16G16_SINT:
            str.append(L"DXGI_FORMAT_R16G16_SINT");
            break;
        case DXGI_FORMAT_R32_TYPELESS:
            str.append(L"DXGI_FORMAT_R32_TYPELESS");
            break;
        case DXGI_FORMAT_D32_FLOAT:
            str.append(L"DXGI_FORMAT_D32_FLOAT");
            break;
        case DXGI_FORMAT_R32_FLOAT:
            str.append(L"DXGI_FORMAT_R32_FLOAT");
            break;
        case DXGI_FORMAT_R32_UINT:
            str.append(L"DXGI_FORMAT_R32_UINT");
            break;
        case DXGI_FORMAT_R32_SINT:
            str.append(L"DXGI_FORMAT_R32_SINT");
            break;
        case DXGI_FORMAT_R24G8_TYPELESS:
            str.append(L"DXGI_FORMAT_R24G8_TYPELESS");
            break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            str.append(L"DXGI_FORMAT_D24_UNORM_S8_UINT");
            break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            str.append(L"DXGI_FORMAT_R24_UNORM_X8_TYPELESS");
            break;
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            str.append(L"DXGI_FORMAT_X24_TYPELESS_G8_UINT");
            break;
        case DXGI_FORMAT_R8G8_TYPELESS:
            str.append(L"DXGI_FORMAT_R8G8_TYPELESS");
            break;
        case DXGI_FORMAT_R8G8_UNORM:
            str.append(L"DXGI_FORMAT_R8G8_UNORM");
            break;
        case DXGI_FORMAT_R8G8_UINT:
            str.append(L"DXGI_FORMAT_R8G8_UINT");
            break;
        case DXGI_FORMAT_R8G8_SNORM:
            str.append(L"DXGI_FORMAT_R8G8_SNORM");
            break;
        case DXGI_FORMAT_R8G8_SINT:
            str.append(L"DXGI_FORMAT_R8G8_SINT");
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            str.append(L"DXGI_FORMAT_R16_TYPELESS");
            break;
        case DXGI_FORMAT_R16_FLOAT:
            str.append(L"DXGI_FORMAT_R16_FLOAT");
            break;
        case DXGI_FORMAT_D16_UNORM:
            str.append(L"DXGI_FORMAT_D16_UNORM");
            break;
        case DXGI_FORMAT_R16_UNORM:
            str.append(L"DXGI_FORMAT_R16_UNORM");
            break;
        case DXGI_FORMAT_R16_UINT:
            str.append(L"DXGI_FORMAT_R16_UINT");
            break;
        case DXGI_FORMAT_R16_SNORM:
            str.append(L"DXGI_FORMAT_R16_SNORM");
            break;
        case DXGI_FORMAT_R16_SINT:
            str.append(L"DXGI_FORMAT_R16_SINT");
            break;
        case DXGI_FORMAT_R8_TYPELESS:
            str.append(L"DXGI_FORMAT_R8_TYPELESS");
            break;
        case DXGI_FORMAT_R8_UNORM:
            str.append(L"DXGI_FORMAT_R8_UNORM");
            break;
        case DXGI_FORMAT_R8_UINT:
            str.append(L"DXGI_FORMAT_R8_UINT");
            break;
        case DXGI_FORMAT_R8_SNORM:
            str.append(L"DXGI_FORMAT_R8_SNORM");
            break;
        case DXGI_FORMAT_R8_SINT:
            str.append(L"DXGI_FORMAT_R8_SINT");
            break;
        case DXGI_FORMAT_A8_UNORM:
            str.append(L"DXGI_FORMAT_A8_UNORM");
            break;
        case DXGI_FORMAT_R1_UNORM:
            str.append(L"DXGI_FORMAT_R1_UNORM");
            break;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            str.append(L"DXGI_FORMAT_R9G9B9E5_SHAREDEXP");
            break;
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            str.append(L"DXGI_FORMAT_R8G8_B8G8_UNORM");
            break;
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            str.append(L"DXGI_FORMAT_G8R8_G8B8_UNORM");
            break;
        case DXGI_FORMAT_BC1_TYPELESS:
            str.append(L"DXGI_FORMAT_BC1_TYPELESS");
            break;
        case DXGI_FORMAT_BC1_UNORM:
            str.append(L"DXGI_FORMAT_BC1_UNORM");
            break;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_BC1_UNORM_SRGB");
            break;
        case DXGI_FORMAT_BC2_TYPELESS:
            str.append(L"DXGI_FORMAT_BC2_TYPELESS");
            break;
        case DXGI_FORMAT_BC2_UNORM:
            str.append(L"DXGI_FORMAT_BC2_UNORM");
            break;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_BC2_UNORM_SRGB");
            break;
        case DXGI_FORMAT_BC3_TYPELESS:
            str.append(L"DXGI_FORMAT_BC3_TYPELESS");
            break;
        case DXGI_FORMAT_BC3_UNORM:
            str.append(L"DXGI_FORMAT_BC3_UNORM");
            break;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_BC3_UNORM_SRGB");
            break;
        case DXGI_FORMAT_BC4_TYPELESS:
            str.append(L"DXGI_FORMAT_BC4_TYPELESS");
            break;
        case DXGI_FORMAT_BC4_UNORM:
            str.append(L"DXGI_FORMAT_BC4_UNORM");
            break;
        case DXGI_FORMAT_BC4_SNORM:
            str.append(L"DXGI_FORMAT_BC4_SNORM");
            break;
        case DXGI_FORMAT_BC5_TYPELESS:
            str.append(L"DXGI_FORMAT_BC5_TYPELESS");
            break;
        case DXGI_FORMAT_BC5_UNORM:
            str.append(L"DXGI_FORMAT_BC5_UNORM");
            break;
        case DXGI_FORMAT_BC5_SNORM:
            str.append(L"DXGI_FORMAT_BC5_SNORM");
            break;
        case DXGI_FORMAT_B5G6R5_UNORM:
            str.append(L"DXGI_FORMAT_B5G6R5_UNORM");
            break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            str.append(L"DXGI_FORMAT_B5G5R5A1_UNORM");
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            str.append(L"DXGI_FORMAT_B8G8R8A8_UNORM");
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            str.append(L"DXGI_FORMAT_B8G8R8X8_UNORM");
            break;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            str.append(L"DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM");
            break;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            str.append(L"DXGI_FORMAT_B8G8R8A8_TYPELESS");
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_B8G8R8A8_UNORM_SRGB");
            break;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            str.append(L"DXGI_FORMAT_B8G8R8X8_TYPELESS");
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_B8G8R8X8_UNORM_SRGB");
            break;
        case DXGI_FORMAT_BC6H_TYPELESS:
            str.append(L"DXGI_FORMAT_BC6H_TYPELESS");
            break;
        case DXGI_FORMAT_BC6H_UF16:
            str.append(L"DXGI_FORMAT_BC6H_UF16");
            break;
        case DXGI_FORMAT_BC6H_SF16:
            str.append(L"DXGI_FORMAT_BC6H_SF16");
            break;
        case DXGI_FORMAT_BC7_TYPELESS:
            str.append(L"DXGI_FORMAT_BC7_TYPELESS");
            break;
        case DXGI_FORMAT_BC7_UNORM:
            str.append(L"DXGI_FORMAT_BC7_UNORM");
            break;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            str.append(L"DXGI_FORMAT_BC7_UNORM_SRGB");
            break;
        case DXGI_FORMAT_AYUV:
            str.append(L"DXGI_FORMAT_AYUV");
            break;
        case DXGI_FORMAT_Y410:
            str.append(L"DXGI_FORMAT_Y410");
            break;
        case DXGI_FORMAT_Y416:
            str.append(L"DXGI_FORMAT_Y416");
            break;
        case DXGI_FORMAT_NV12:
            str.append(L"DXGI_FORMAT_NV12");
            break;
        case DXGI_FORMAT_P010:
            str.append(L"DXGI_FORMAT_P010");
            break;
        case DXGI_FORMAT_P016:
            str.append(L"DXGI_FORMAT_P016");
            break;
        case DXGI_FORMAT_420_OPAQUE:
            str.append(L"DXGI_FORMAT_420_OPAQUE");
            break;
        case DXGI_FORMAT_YUY2:
            str.append(L"DXGI_FORMAT_YUY2");
            break;
        case DXGI_FORMAT_Y210:
            str.append(L"DXGI_FORMAT_Y210");
            break;
        case DXGI_FORMAT_Y216:
            str.append(L"DXGI_FORMAT_Y216");
            break;
        case DXGI_FORMAT_NV11:
            str.append(L"DXGI_FORMAT_NV11");
            break;
        case DXGI_FORMAT_AI44:
            str.append(L"DXGI_FORMAT_AI44");
            break;
        case DXGI_FORMAT_IA44:
            str.append(L"DXGI_FORMAT_IA44");
            break;
        case DXGI_FORMAT_P8:
            str.append(L"DXGI_FORMAT_P8");
            break;
        case DXGI_FORMAT_A8P8:
            str.append(L"DXGI_FORMAT_A8P8");
            break;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            str.append(L"DXGI_FORMAT_B4G4R4A4_UNORM");
            break;
        case DXGI_FORMAT_FORCE_UINT:
            str.append(L"DXGI_FORMAT_FORCE_UINT");
            break;
        default:
            str.append(L"DXGI_FORMAT_???");
            break;
        }

        if (!includePrefix)
        {
            auto length = static_cast<long>(std::wstring(L"DXGI_FORMAT_").length());
            str.erase(str.begin(), str.begin() + length);
        }
        return str;
    }

    // A helper function to turn a D3D_DRIVER_TYPE value into a unicode string. An unknown value will result in D3D_DRIVER_TYPE_???.
    // (Note: The return value is a constant string so you do not need to free it.)
    // deviceType - The D3D_DRIVER_TYPE value to convert to a string.
    inline LPCWSTR D3DDriverTypeToString(_In_ D3D_DRIVER_TYPE deviceType)
    {
        switch (deviceType)
        {
        case D3D_DRIVER_TYPE_HARDWARE:
            return L"D3D_DRIVER_TYPE_HARDWARE";
        case D3D_DRIVER_TYPE_REFERENCE:
            return L"D3D_DRIVER_TYPE_REFERENCE";
        case D3D_DRIVER_TYPE_NULL:
            return L"D3D_DRIVER_TYPE_NULL";
        case D3D_DRIVER_TYPE_WARP:
            return L"D3D_DRIVER_TYPE_WARP";
        case D3D_DRIVER_TYPE_SOFTWARE:
            return L"D3D_DRIVER_TYPE_SOFTWARE";
        case D3D_DRIVER_TYPE_UNKNOWN:
            return L"D3D_DRIVER_TYPE_UNKNOWN";
        default:
            return L"D3D_DRIVER_TYPE_???";
        }
    }

    // A helper function to turn a D3D_FEATURE_LEVEL value into a unicode string. An unknown value will result in D3D_FEATURE_LEVEL_???.
    // (Note: The return value is a constant string so you do not need to free it.)
    // featureLevel - The D3D_FEATURE_LEVEL value to convert to a string.
    inline LPCWSTR D3DFeatureLevelToString(_In_ D3D_FEATURE_LEVEL featureLevel)
    {
        switch (featureLevel)
        {
        case D3D_FEATURE_LEVEL_9_1:
            return L"D3D_FEATURE_LEVEL_9_1";
        case D3D_FEATURE_LEVEL_9_2:
            return L"D3D_FEATURE_LEVEL_9_2";
        case D3D_FEATURE_LEVEL_9_3:
            return L"D3D_FEATURE_LEVEL_9_3";
        case D3D_FEATURE_LEVEL_10_0:
            return L"D3D_FEATURE_LEVEL_10_0";
        case D3D_FEATURE_LEVEL_10_1:
            return L"D3D_FEATURE_LEVEL_10_1";
        case D3D_FEATURE_LEVEL_11_0:
            return L"D3D_FEATURE_LEVEL_11_0";
        case D3D_FEATURE_LEVEL_11_1:
            return L"D3D_FEATURE_LEVEL_11_1";
        default:
            return L"D3D_FEATURE_LEVEL_???";
        }
    }


    // A typedef for easily grouping supported multisample count & quality pairs.
    // The first value of each pair is the count and the second value is the quality.
    typedef std::vector<std::pair<unsigned int, unsigned int>> MultisampleCountQualityVector;

    // Check for supported multisample settings. The resulting vector will be empty if multisampling is not supported.
    // Returns a vector of std::pair<uint, uint> where the first value is the count and the second is the quality. If MSAA is unsupported, the vector will be empty.
    inline MultisampleCountQualityVector GetSupportedMultisampleSettings(ID3D11Device* device, DXGI_FORMAT backBufferFormat)
    {
        MultisampleCountQualityVector vec = MultisampleCountQualityVector();

        unsigned int quality = 0;

        for (unsigned int count = 1; count < D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++count)
        {
            if SUCCEEDED(device->CheckMultisampleQualityLevels(backBufferFormat, count, &quality))
            {
                if (quality > 0)
                {
                    vec.emplace_back(count, quality);
                }
            }
        }

        return vec;
    }

	// Validates multisample count and quality values for the given DXGI_FORMAT for the graphics card.
	// Returns true if the values were valid, false if the values had to be modified. Note that 
	// multisampleCount and multisampleQuality are passed as unsigned int pointers because they are 
	// subject to modification. For C# programmers, this is much the same as a 'ref' parameter.
	inline bool ValidateMultisampleValues(
		_In_ ID3D11Device* device,
		_In_ DXGI_FORMAT targetFormat,
		_Inout_ unsigned int* multisampleCount,
		_Inout_ unsigned int* multisampleQuality
		)
	{
		// MSAA only happens when the count is greater than one. So we do different validation if count is not greater than 1. 
		if (*multisampleCount > 1)
		{
			UINT maxQuality;
			DX::ThrowIfFailed(
				device->CheckMultisampleQualityLevels(targetFormat, *multisampleCount, &maxQuality), __FILEW__, __LINE__
				);
			if (maxQuality == 0)
			{
#if defined(_DEBUG)
				std::wstringstream debugMessage;
				debugMessage << L"Invalid multisample count (" << *multisampleCount << L") and quality (" << *multisampleQuality << L") combination.\n" <<
					L"Defaulting to no multisampling because count is invalid as a sample count for this render target format '" << DX::DXGIFormatString(targetFormat) << L"'.\n";
				OutputDebugStringW(debugMessage.str().c_str());
#endif
				*multisampleCount = 1;
				*multisampleQuality = 0;
				return false;
			}
			else
			{
				// For why it's maxQuality - 1, see: http://msdn.microsoft.com/en-us/library/bb173072(VS.85).aspx
				if (maxQuality - 1 != *multisampleQuality)
				{
#if defined(_DEBUG)
					std::wstringstream debugMessage;
					debugMessage << L"Invalid multisample quality (" << *multisampleQuality << L") for specified count of " << *multisampleCount <<
						L".\nDefaulting to max quality of " << maxQuality - 1 << L".\n";
					OutputDebugStringW(debugMessage.str().c_str());
#endif
					*multisampleQuality = maxQuality - 1;
					return false;
				}
				else
				{
					return true;
				}
			}
		}
		else
		{
			// If multisampleCount is not greater than 1 then the only valid values are multisampleCount == 1 and multisampleQuality == 0.
			// We still do validation and output debug runtime warnings if the values are different since the developer likely either
			// misunderstood what count and quality are supposed to be or else mistakenly assigned wrong values (e.g. swapping count and quality).
			bool result = true;

			if (*multisampleCount == 0)
			{
#if defined (_DEBUG)
				std::wstringstream debugMessage;
				debugMessage << L"Warning: Disregarding improper multisample count value of '" << *multisampleCount << L"' and defaulting to msaa count of '1'.\n";
				OutputDebugStringW(debugMessage.str().c_str());
#endif
				// A count of 0 is wrong so adjust it to 1.
				*multisampleCount = 1;
				result = false;
			}
			if (*multisampleQuality != 0)
			{
#if defined (_DEBUG)
				std::wstringstream debugMessage;
				debugMessage << L"Warning: Disregarding improper multisample quality value of '" << *multisampleQuality << L"' and defaulting to msaa quality of '0'.\n";
				OutputDebugStringW(debugMessage.str().c_str());
#endif
				// Since quality is set to disable MSAA, we ignore quality and just pass 0 since it's the only correct value.
				*multisampleQuality = 0;
				result = false;
			}
			return result;
		}
	}
}