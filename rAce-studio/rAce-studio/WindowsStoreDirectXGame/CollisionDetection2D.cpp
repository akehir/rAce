#include "pch.h"
#include "CollisionDetection2D.h"

std::unique_ptr<uint8> DX::GetTexture2DCollisionDataNoRender(
	_In_ ID3D11Device* device,
	_In_ ID3D11DeviceContext* context,
	_In_ ID3D11ShaderResourceView* textureSRV,
	_In_opt_z_ const wchar_t* filename,
	_In_ unsigned long lineNumber
	)
{
	// If null filename was passed in, use an empty string.
	if (filename == nullptr)
	{
		filename = L"";
	}

	// Extract the ID3D11Texture2D from the SRV so that we can get its desc (and thus its width and height).
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	textureSRV->GetResource(&resource);
	DX::ThrowIfFailed(
		resource.As(&texture), filename, lineNumber
		);

	// Get the texture desc so we know the dimensions.
	D3D11_TEXTURE2D_DESC desc = {};
	texture->GetDesc(&desc);

	if (desc.SampleDesc.Count > 1 || desc.SampleDesc.Quality > 0)
	{
		DX::ThrowIfFailed(E_NOTIMPL, filename, lineNumber);
	}

	// Create a texture that matches the input texture's format but which is configured for CPU read access. This requires that
	// the texture have a D3D11_USAGE_STAGING usage and the only thing you can do with staging usage textures is use them with
	// ID3D11DeviceContext::CopyResource, CopySubresource, Map, and Unmap. Think of staging textures as being bridges between the 
	// CPU and GPU. Note that reading data from the GPU is slow so this should be avoided in time critical areas of your code.
	Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture;
	desc.Usage = D3D11_USAGE_STAGING; // Make it a staging texture.
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // Give the CPU read access (you could also give it write access but we don't need that).
	desc.BindFlags = 0; // Zero out the bind flags since a staging texture can never be bound to the render pipeline in any way.

	// Create the staging texture.
	DX::ThrowIfFailed(
		device->CreateTexture2D(
		&desc,
		nullptr,
		&stagingTexture
		), filename, lineNumber
		);

	// Copy the render target's data to the staging texture.
	context->CopyResource(stagingTexture.Get(), texture.Get());

	// The number of bytes in a B8G8R8A8 pixel.
	const uint32 bytesPerBGRAPixel = 4;

	// Process the data based on the format of the incoming texture.
	switch (desc.Format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		{
			// Calculate the byte size of the data once and reuse the value. Since the texture is B8G8R8A8 (a 32-bit format) we must multiply
			// by 4 to account for the memory size of each pixel.
			auto resultSizeInBytes = desc.Width * desc.Height * bytesPerBGRAPixel;
			auto rowSizeInBytes = desc.Width * bytesPerBGRAPixel;

			// Create and zero out the result smart pointer byte array.
			std::unique_ptr<uint8> result(new uint8[resultSizeInBytes]);
			//ZeroMemory(result.get(), resultSizeInBytes);

			// Map the staging texture so that we can read out its data.
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			DX::ThrowIfFailed(
				context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource), filename, lineNumber
				);

			auto resultPtr = result.get();

			// We have to copy row by row since the RowPitch could be greater than desc.Width * 4 (e.g. if the runtime needed to place padding in between rows for some reason).
			for (unsigned int i = 0; i < desc.Height; ++i)
			{
				// We know that the mapped data can be viewed as byte data so we can use reinterpret_cast to cast it to byte data such that we can offset correctly.
				memcpy_s(resultPtr + (i * rowSizeInBytes), resultSizeInBytes - (i * rowSizeInBytes), reinterpret_cast<uint8 *>(mappedResource.pData) + (i * mappedResource.RowPitch), desc.Width * 4); 
			}

			// Unmap the staging texture now that we are done with it.
			context->Unmap(stagingTexture.Get(), 0);

			// Return the result.
			return std::move(result);
		}

	case DXGI_FORMAT_R8G8B8A8_UNORM:
		{
			// Calculate the byte size of the data once and reuse the value. Since the texture is B8G8R8A8 (a 32-bit format) we must multiply
			// by 4 to account for the memory size of each pixel.
			auto resultSizeInBytes = desc.Width * desc.Height * bytesPerBGRAPixel;
			auto rowSizeInBytes = desc.Width * bytesPerBGRAPixel;

			// Create and zero out the result smart pointer byte array.
			std::unique_ptr<uint8> result(new uint8[resultSizeInBytes]);

			// Map the staging texture so that we can read out its data.
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			DX::ThrowIfFailed(
				context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource), filename, lineNumber
				);
			auto resultPtr = result.get();
			// We have to copy row by row since the RowPitch could be greater than desc.Width * 4 (e.g. if the runtime needed to place padding in between rows for some reason).
			for (unsigned int i = 0; i < desc.Height; ++i)
			{
				auto indexOffset = i * rowSizeInBytes;
				// We know that the mapped data can be viewed as byte data so we can use reinterpret_cast to cast it to byte data such that we can offset correctly.
				memcpy_s(resultPtr + indexOffset, resultSizeInBytes - (i * rowSizeInBytes), reinterpret_cast<uint8 *>(mappedResource.pData) + (i * mappedResource.RowPitch), desc.Width * 4); 
				// Swizzle r and b.
				for (unsigned int j = 0; j < desc.Width; ++j)
				{
					auto widthOffset = indexOffset + (j * bytesPerBGRAPixel);
					auto r = *(resultPtr + widthOffset);
					*(resultPtr + widthOffset) = *(resultPtr + widthOffset + 2);
					*(resultPtr + widthOffset + 2) = r;
				}
			}
			// Unmap the staging texture now that we are done with it.
			context->Unmap(stagingTexture.Get(), 0);

			// Return the result.
			return std::move(result);
		}


	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		{
			// There are four channels in an R32B32G32A32 texture.
			const uint32 componentsPerUnit = 4;
			auto resourceDataSizeInUnitsPerPixel = desc.Width * desc.Height * componentsPerUnit;
			auto unitSizeInBytes = sizeof(float);
			auto resourceDataSizeInBytes = resourceDataSizeInUnitsPerPixel * unitSizeInBytes;
			auto rowSizeInBytes = desc.Width * componentsPerUnit * unitSizeInBytes;

			// Allocate the float array for the extracted data.
			std::unique_ptr<float> resourceData(new float[resourceDataSizeInUnitsPerPixel]);

			// Map the staging texture so that we can read out its data.
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			DX::ThrowIfFailed(
				context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource), filename, lineNumber
				);

			// We have to copy row by row since the RowPitch could be greater than desc.Width * 4 (e.g. if the runtime needed to place padding in between rows for some reason).
			for (unsigned int i = 0; i < desc.Height; i++)
			{
				// We know that the mapped data can be viewed as byte data so we can use reinterpret_cast to cast it to byte data such that we can offset correctly.
				// Because the destination is not a byte (uint8), we need to cast it to uint8 to get proper pointer offsets in order to ensure that all data is written to its correct memory location.
				memcpy_s(reinterpret_cast<uint8 *>(resourceData.get()) + (i * rowSizeInBytes), resourceDataSizeInBytes - (i * rowSizeInBytes), (reinterpret_cast<uint8 *>(mappedResource.pData) + (i * mappedResource.RowPitch)), rowSizeInBytes); 
			}

			// Unmap the staging texture now that we are done with it.
			context->Unmap(stagingTexture.Get(), 0);

			// Calculate the byte size of the data once and reuse the value. Since the texture is B8G8R8A8 (a 32-bit format) we must multiply
			// by 4 to account for the memory size of each pixel.
			auto resultSizeInBytes = desc.Width * desc.Height * 4;

			// Create and zero out the result smart pointer byte array.
			std::unique_ptr<uint8> result(new uint8[resultSizeInBytes]);
			ZeroMemory(result.get(), resultSizeInBytes);

			auto resultDataPtr = result.get();
			auto resourceDataPtr = resourceData.get();
			for (unsigned int i = 0; i < desc.Width * desc.Height; i += bytesPerBGRAPixel)
			{
				DirectX::XMFLOAT4 value(resourceDataPtr[i + 2], resourceDataPtr[i + 1], resourceDataPtr[i + 0], resourceDataPtr[i + 3]);
				DirectX::PackedVector::XMUBYTEN4 byteData;
				DirectX::PackedVector::XMStoreUByteN4(&byteData, DirectX::XMLoadFloat4(&value));
				resultDataPtr[i + 0] = byteData.x;
				resultDataPtr[i + 1] = byteData.y;
				resultDataPtr[i + 2] = byteData.z;
				resultDataPtr[i + 3] = byteData.w;
			}

			// Return the result.
			return std::move(result);
		}

	case DXGI_FORMAT_BC1_UNORM:
		// Intentional fall-through. BC1 and BC3 are the same in most of their color compression (BC1 alpha introduces a slight BC1-specific variant) so we avoid code duplication this way.
	case DXGI_FORMAT_BC3_UNORM:
		{
			// Since we're processing either BC1 or BC3 data we need to know which we're working with.
			const bool isBC3 = desc.Format == DXGI_FORMAT_BC3_UNORM;

			// BC1 and BC3 both operate by compressing and decompressing pixel data in 4x4 blocks. This serves as our recognition of that.
			const uint32 pixelsPerBlockDimension = 4;
			// BC1 has 8 bytes of data per 4x4 block; BC3 has 16 bytes per 4x4 block (the extra 8 are for its alpha encoding).
			uint32 bytesPerBlock = isBC3 ? 16 : 8;

			// Validate that the BC1/3 texture's width and height are multiples of 4. Add in a check for a zero width/height as well even though that shouldn't ever happen.
			if ((desc.Width % 4) != 0 || (desc.Height % 4) != 0 || desc.Width == 0 || desc.Height == 0)
			{
#if defined(_DEBUG)
				OutputDebugStringW(
					std::wstring(L"The dimensions of a ").append(
					isBC3 ? L"BC3" : L"BC1").append(
					L" texture must be greater than zero and multiples of 4. The texture data passed has the following dimensions: ").append(
					std::to_wstring(desc.Width)).append(
					L"x").append(
					std::to_wstring(desc.Height)).append(
					L".").append(
					(desc.Width % 4 != 0) ? L" The width is not divisible by 4." : L"").append(
					(desc.Height % 4 != 0) ? L" The height is not divisible by 4." : L"").append(L"\n").c_str()
					);
#endif
				DX::ThrowIfFailed(E_INVALIDARG, filename, lineNumber);
			}

			// Figure out how many bytes we need for the BC1/BC3 compressed pixel data by calculating the number of blocks and multiplying it by the bytes per block.
			auto resourceDataSizeInBytes = ((desc.Width / pixelsPerBlockDimension) * (desc.Height / pixelsPerBlockDimension)) * bytesPerBlock;				
			// Calculate how many bytes there are in a row. For our purposes here, a row is presumed to have a 4 pixel height such that this is a row of blocks, not of pixels.
			auto rowSizeInBytes = (desc.Width / pixelsPerBlockDimension) * bytesPerBlock;

			// Allocate the byte array for the extracted data.
			std::unique_ptr<uint8> resourceData(new uint8[resourceDataSizeInBytes]);

			// Map the staging texture so that we can read out its data.
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			DX::ThrowIfFailed(
				context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource), filename, lineNumber
				);

			// We have to copy row by row since the RowPitch could be greater than the row size in bytes (e.g. if the runtime needed to place padding in between rows for some reason).
			// Remember that we are operating on rows of blocks here, not rows of pixels.
			for (unsigned int i = 0; i < (desc.Height / pixelsPerBlockDimension); i++)
			{
				// We know that the mapped data can be viewed as byte data so we can use reinterpret_cast to cast it to byte data such that we can offset correctly.
				memcpy_s(resourceData.get() + (i * rowSizeInBytes), resourceDataSizeInBytes - (i * rowSizeInBytes), reinterpret_cast<uint8 *>(mappedResource.pData) + (i * mappedResource.RowPitch), rowSizeInBytes);
			}

			// Unmap the staging texture now that we are done with it.
			context->Unmap(stagingTexture.Get(), 0);

			// Calculate the byte size of the data once and reuse the value. Since the texture is B8G8R8A8 (a 32-bit format) we must multiply
			// by 4 to account for the memory size of each pixel.
			auto resultSizeInBytes = desc.Width * desc.Height * bytesPerBGRAPixel;

			// Create and zero out the result smart pointer byte array.
			std::unique_ptr<uint8> result(new uint8[resultSizeInBytes]);
			ZeroMemory(result.get(), resultSizeInBytes);

			auto resultDataPtr = result.get();
			auto resourceDataPtr = resourceData.get();

			// Loop through the data block by block.
			for (unsigned int i = 0; i < (desc.Width / pixelsPerBlockDimension) * (desc.Height / pixelsPerBlockDimension) * bytesPerBlock; i += bytesPerBlock)
			{
				// This stores the appropriate alpha value for the pixel.
				uint8 alphaValue;

				// BC3 alpha values.
				uint8 alpha0 = 0;
				uint8 alpha1 = 0;
				uint8 alpha2 = 0;
				uint8 alpha3 = 0;
				uint8 alpha4 = 0;
				uint8 alpha5 = 0;
				uint8 alpha6 = 0;
				uint8 alpha7 = 0;

				// Two bytes of the 8 bytes of alpha data are for alpha0 and alpha1. The remaining 6 bytes operate as a 3bpp map to tell us which alpha value to use for each pixel in the block.
				uint64 alphamap = 0ULL;

				if (isBC3)
				{
					// BC3 uses two stored alpha values and either 6 interpolated values or 4 interpolated values (with the remaining two being completely transparent and completely opaque).
					// We initialize these to remove warnings about using possibly uninitialized values since the compiler's analyzer doesn't pick up that these values are all definitely
					// initialized in an "if (isBC3) { ... }" block and are only used in an "if (isBC3) { ... }" block.
					
					// Extract the two stored alpha values.
					alpha0 = resourceDataPtr[i + 0];
					alpha1 = resourceDataPtr[i + 1];

					// Alpha interpolation depends on whether or not the first bytes is larger than the second.
					if (alpha0 > alpha1)
					{
						// If the first is larger than the second then all remaining values are interpolated.
						alpha2 = static_cast<uint8>((6 * alpha0 + 1 * alpha1 + 3) / 7);
						alpha3 = static_cast<uint8>((5 * alpha0 + 2 * alpha1 + 3) / 7);
						alpha4 = static_cast<uint8>((4 * alpha0 + 3 * alpha1 + 3) / 7);
						alpha5 = static_cast<uint8>((3 * alpha0 + 4 * alpha1 + 3) / 7);
						alpha6 = static_cast<uint8>((2 * alpha0 + 5 * alpha1 + 3) / 7);
						alpha7 = static_cast<uint8>((1 * alpha0 + 6 * alpha1 + 3) / 7);
					}
					else
					{
						// Otherwise only four are interpolates with the last two being fully transparent and opaque, respectively.
						alpha2 = static_cast<uint8>((4 * alpha0 + 1 * alpha1 + 2) / 5);
						alpha3 = static_cast<uint8>((3 * alpha0 + 2 * alpha1 + 2) / 5);
						alpha4 = static_cast<uint8>((2 * alpha0 + 3 * alpha1 + 2) / 5);
						alpha5 = static_cast<uint8>((1 * alpha0 + 4 * alpha1 + 2) / 5);
						alpha6 = 0;
						alpha7 = 255;
					}

					// Make the remaining six bytes into a uint64 (unsigned long long) so we can use bit shifting to extract values.
					alphamap =
						static_cast<uint64>(MAKELONG(MAKEWORD(resourceDataPtr[i + 2], resourceDataPtr[i + 3]), MAKEWORD(resourceDataPtr[i + 4], resourceDataPtr[i + 5]))) |
						(static_cast<uint64>(MAKELONG(MAKEWORD(resourceDataPtr[i + 6], resourceDataPtr[i + 7]), MAKEWORD(0, 0))) << 32);
				}

				// For BC3 we extract the color data starting at byte 8 (the first 8 bytes are for alpha). For BC1 we extract it starting at byte 0 (alpha is opaque or completely transparent).
				uint32 bcTypeOffset = isBC3 ? 8 : 0;

				// The first two colors are stored as B5G6R5 UINT data, allowing them to be packed into 2 bytes each (4 bytes total).
				auto color0 = MAKEWORD(resourceDataPtr[i + bcTypeOffset + 0], resourceDataPtr[i + bcTypeOffset + 1]);
				auto color1 = MAKEWORD(resourceDataPtr[i + bcTypeOffset + 2], resourceDataPtr[i + bcTypeOffset + 3]);

				// The remaining 4 bytes are a 2bpp map that tells us which one of the four colors to use for each of the 16 pixels in the block.
				auto colormap = MAKELONG(
					MAKEWORD(resourceDataPtr[i + bcTypeOffset + 4], resourceDataPtr[i + bcTypeOffset + 5]),
					MAKEWORD(resourceDataPtr[i + bcTypeOffset + 6], resourceDataPtr[i + bcTypeOffset + 7])
					);

				// Put the color into an XMU565 so we can load it as a vector and convert it to B8G8R8 UNORM data.
				DirectX::PackedVector::XMU565 pack565Color0(color0);
				DirectX::PackedVector::XMU565 pack565Color1(color1);

				// To convert to B8G8R8X8 UNORM from B5G6R5 UINT, we load a color as an XMVECTOR then divide it by (31, 63, 31) (the max values of a 5 bit number, 6 bit number,
				// and 5 bit number, respectively).
				DirectX::XMVECTOR vecColor0 = DirectX::XMVectorDivide(DirectX::PackedVector::XMLoadU565(&pack565Color0), DirectX::XMVectorSet(31.0f, 63.0f, 31.0f, 1.0f));
				DirectX::XMVECTOR vecColor1 = DirectX::XMVectorDivide(DirectX::PackedVector::XMLoadU565(&pack565Color1), DirectX::XMVectorSet(31.0f, 63.0f, 31.0f, 1.0f));

				// Used to store the B8G8R8X8 UNORM color data.
				DirectX::PackedVector::XMUBYTEN4 c0;
				DirectX::PackedVector::XMUBYTEN4 c1;
				DirectX::PackedVector::XMUBYTEN4 c2;
				DirectX::PackedVector::XMUBYTEN4 c3;

				// The first two color are always the stored colors.
				DirectX::PackedVector::XMStoreUByteN4(&c0, vecColor0);
				DirectX::PackedVector::XMStoreUByteN4(&c1, vecColor1);

				if (isBC3 || (color0 > color1))
				{
					// If we're working with BC3 or if the uint16 value of color0 is greater than color 1, the remaining two colors are interpolated from the stored colors.
					// Color 2 is 1/3 of the way between colors 0 and 1.
					DirectX::PackedVector::XMStoreUByteN4(&c2, DirectX::XMVectorLerp(vecColor0, vecColor1, 1.0f / 3.0f));
					// Color 3 is 2/3 of the way between colors 0 and 1.
					DirectX::PackedVector::XMStoreUByteN4(&c3, DirectX::XMVectorLerp(vecColor0, vecColor1, 2.0f / 3.0f));
				}
				else
				{
					// If we're working with BC1 and the uint16 value of color 0 is less than or equal to color 1, then there is at least one transparent pixel in the block
					// such that only one color is interpolated and the other is (transparent black.
					// Color 2 is 1/2 of the way between colors 0 and 1.
					DirectX::PackedVector::XMStoreUByteN4(&c2, DirectX::XMVectorLerp(vecColor0, vecColor1, 0.5f));
					// Color 3 is transparent black. 
					c3 = DirectX::PackedVector::XMUBYTEN4(0U);
				}

				// Precalculate the current row (in pixel rows) so we can properly offset into our result array.
				int currentRow = (i / ((desc.Width / pixelsPerBlockDimension))) / bytesPerBlock * pixelsPerBlockDimension;
				// Precalculate the current column offset (in pixels) so we can properly offset into our result array.
				int currentColumnOffset = (i % ((desc.Width / pixelsPerBlockDimension) * bytesPerBlock)) / bytesPerBlock * pixelsPerBlockDimension;

				// Loop through the current block's 4x4 grid of pixels.
				for (int y = 0; y < pixelsPerBlockDimension; y++)
				{
					for (int x = 0; x < pixelsPerBlockDimension; x++)
					{
						// For BC3, which alpha to use for each pixel is stored in a bitmap, with 3 bits for each pixel. For
						// more info, see: http://msdn.microsoft.com/en-us/library/windows/desktop/bb206238(v=vs.85).aspx .
						if (isBC3)
						{
							switch (alphamap & 7) // 7 in binary is 111
							{
							case 0:
								alphaValue = alpha0;
								break;
							case 1:
								alphaValue = alpha1;
								break;
							case 2:
								alphaValue = alpha2;
								break;
							case 3:
								alphaValue = alpha3;
								break;
							case 4:
								alphaValue = alpha4;
								break;
							case 5:
								alphaValue = alpha5;
								break;
							case 6:
								alphaValue = alpha6;
								break;
							case 7:
								alphaValue = alpha7;
								break;
							default:
								throw Platform::Exception::CreateException(E_UNEXPECTED);
							}
						}
						else
						{
							// For BC1, a pixel is either opaque or transparent. For the first 3 pixel colors, it's always opaque. For the
							// fourth color it's opaque if the uint16 value of color0 is greater than color 1, otherwise it's transparent. For
							// more info, see: http://msdn.microsoft.com/en-us/library/windows/desktop/bb147243(v=vs.85).aspx .
							switch (colormap & 3) // 3 in binary is 11
							{
							case 0:
								// Intentional fall-through.
							case 1:
								// Intentional fall-through.
							case 2:
								alphaValue = 0xFF;
								break;
							case 3:
								alphaValue = (color0 > color1) ? 0xFF : 0x0;
								break;
							default:
								throw Platform::Exception::CreateException(E_UNEXPECTED);
							}
						}

						// Calculate the result array index offset (in bytes) for the current pixel so that we write the data to the proper place.
						int index = ((currentRow * desc.Width * bytesPerBGRAPixel) + (y * desc.Width * bytesPerBGRAPixel)) + (currentColumnOffset * bytesPerBGRAPixel) + (x * bytesPerBGRAPixel);

						// For BC1 and BC3, which color to use for each pixel is stored in a bitmap, with 2 bits for each pixel. For more
						// info, see: http://msdn.microsoft.com/en-us/library/bb694531(v=vs.85).aspx .
						switch (colormap & 3)
						{
						case 0:
							resultDataPtr[index + 0] = c0.x;
							resultDataPtr[index + 1] = c0.y;
							resultDataPtr[index + 2] = c0.z;
							resultDataPtr[index + 3] = alphaValue;
							break;
						case 1:
							resultDataPtr[index + 0] = c1.x;
							resultDataPtr[index + 1] = c1.y;
							resultDataPtr[index + 2] = c1.z;
							resultDataPtr[index + 3] = alphaValue;
							break;
						case 2:
							resultDataPtr[index + 0] = c2.x;
							resultDataPtr[index + 1] = c2.y;
							resultDataPtr[index + 2] = c2.z;
							resultDataPtr[index + 3] = alphaValue;
							break;
						case 3:
							resultDataPtr[index + 0] = c3.x;
							resultDataPtr[index + 1] = c3.y;
							resultDataPtr[index + 2] = c3.z;
							resultDataPtr[index + 3] = alphaValue;
							break;
						default:
							throw Platform::Exception::CreateException(E_UNEXPECTED);
						}
						// Shift the color bitmap two bits to the right so we're ready for the next pixel.
						colormap >>= 2;
						// Shift the alpha bitmap three bits to the right so we're ready for the next pixel.
						alphamap >>= 3;
					}
				}
			}

			// Return the result.
			return std::move(result);
		}

	default:
#if defined(_DEBUG)
		assert("GetTexture2DCollisionDataNoRender called with an unhandled DXGI_FORMAT.");
#endif
		// Use the COM HRESULT error ERROR_GRAPHICS_INVALID_PIXELFORMAT as it's the closest HRESULT to our problem.
		DX::ThrowIfFailed(ERROR_GRAPHICS_INVALID_PIXELFORMAT, filename, lineNumber);
		return nullptr;
	}
}

std::unique_ptr<uint8> DX::GetTexture2DCollisionData(
	_In_ ID3D11Device* device,
	_In_ ID3D11DeviceContext* context,
	_In_ DirectX::SpriteBatch* spriteBatch,
	_In_ DirectX::CommonStates* commonStates,
	_In_ ID3D11ShaderResourceView* textureSRV,
	_In_opt_z_ const wchar_t* filename,
	_In_ unsigned long lineNumber
	)
{
	// If null filename was passed in, use an empty string.
	if (filename == nullptr)
	{
		filename = L"";
	}

	// Extract the ID3D11Texture2D from the SRV so that we can get its desc (and thus its width and height).
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	textureSRV->GetResource(&resource);
	DX::ThrowIfFailed(
		resource.As(&texture), filename, lineNumber
		);

	// Get the texture desc so we know the dimensions.
	D3D11_TEXTURE2D_DESC desc = {};
	texture->GetDesc(&desc);

	// Create a render target that we can render the texture to. We create the render target as a DXGI_FORMAT_B8G8R8A8_UNORM
	// target both because it is a supported format in Feature Level 9.1 and because we can render a texture of any format (e.g. BC3)
	// to this render target and thus effectively convert the format of any supported texture to a uniform data format that we can use for
	// per-pixel collision detection.
	RenderTarget2D rt;
	rt.CreateRenderTarget(device, desc.Width, desc.Height, DXGI_FORMAT_B8G8R8A8_UNORM);

	// Save the original render target and depth stencil view so we can restore them. If more than one render target was bound then
	// this will fail to preserve anything but the first target, so you will need to store and restore state on your own if you have
	// multiple render targets bound before calling this function and want them restored when this function completes.
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> originalRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> originalDSV;
	context->OMGetRenderTargets(1, &originalRTV, &originalDSV);

	// Save the original view port. If more than one viewport is bound then this will fail to preserve anything but the first view
	// port, so you will need to store and restore state on your own if you have multiple viewports bound.
	D3D11_VIEWPORT originalViewPort;
	UINT originalNumViewPorts = 1L;
	context->RSGetViewports(&originalNumViewPorts, &originalViewPort);

	// Create a viewport that matches the texture's dimensions.
	CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height));

	// Set the viewport and render target.
	context->RSSetViewports(1, &viewPort);
	context->OMSetRenderTargets(1, rt.GetRTV(), rt.GetDSV());
	context->ClearRenderTargetView(*rt.GetRTV(), DirectX::Colors::Transparent);

	// Draw the texture to our render target.
	spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr, commonStates->PointClamp());
	spriteBatch->Draw(textureSRV, DirectX::XMVectorZero());
	spriteBatch->End();

	// Restore viewport and render target.
	context->RSSetViewports(originalNumViewPorts, &originalViewPort);
	context->OMSetRenderTargets(1, originalRTV.GetAddressOf(), originalDSV.Get());

	// Create a texture that matches the render target's format but which is configured for CPU read access. This requires that
	// the texture have a D3D11_USAGE_STAGING usage and the only thing you can do with staging usage textures is use them with
	// ID3D11DeviceContext::CopyResource, CopySubresource, Map, and Unmap. That's why the render target wasn't created as a
	// staging texture since it wouldn't have been able to be used as a render target then. Think of staging textures as being
	// bridges between the CPU and GPU. Note that reading data from the GPU is slow so this should be avoided in time critical
	// areas of your code.
	Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture;
	desc = *rt.GetDesc();
	desc.Usage = D3D11_USAGE_STAGING; // Make it a staging texture.
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // Give the CPU read access (you could also give it write access but we don't need that.
	desc.BindFlags = 0; // Zero out the bind flags since a staging texture can never be bound to the render pipeline in any way.

	// Create the staging texture.
	DX::ThrowIfFailed(
		device->CreateTexture2D(
		&desc,
		nullptr,
		&stagingTexture
		), filename, lineNumber
		);

	// Copy the render target's data to the staging texture.
	context->CopyResource(stagingTexture.Get(), rt.GetTexture2D());

	// Calculate the byte size of the data once and reuse the value. Since the texture is B8G8R8A8 (a 32-bit format) we must multiply
	// by 4 to account for the memory size of each pixel.
	auto resultSizeInBytes = desc.Width * desc.Height * 4;
	const uint32 componentsPerUnit = 4;
	auto unitSizeInBytes = sizeof(uint8);
	auto rowSizeInBytes = desc.Width * componentsPerUnit * unitSizeInBytes;

	// Create and zero out the result smart pointer byte array.
	std::unique_ptr<uint8> result(new uint8[resultSizeInBytes]);
	ZeroMemory(result.get(), resultSizeInBytes);

	// Map the staging texture so that we can read out its data.
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	DX::ThrowIfFailed(
		context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource), filename, lineNumber
		);

	// We have to copy row by row since the RowPitch could be greater than desc.Width * 4 (e.g. if the runtime needed to place padding in between rows for some reason).
	for (unsigned int i = 0; i < desc.Height; i++)
	{
		// We know that the mapped data can be viewed as byte data so we can use reinterpret_cast to cast it to byte data such that we can offset correctly. We then cast back to void*.
		memcpy_s(result.get() + (i * rowSizeInBytes), resultSizeInBytes - (i * rowSizeInBytes), (reinterpret_cast<void *>(reinterpret_cast<uint8 *>(mappedResource.pData) + (i * mappedResource.RowPitch))), desc.Width * 4); 
	}

	// Unmap the staging texture now that we are done with it.
	context->Unmap(stagingTexture.Get(), 0);

	// Return the result.
	return std::move(result);
}

bool DX::IsPixelPerfectCollision(
	_In_ const std::unique_ptr<uint8>& spriteOneTextureData,
	_In_ Windows::Foundation::Rect& spriteOnePosition,
	_In_ const std::unique_ptr<uint8>& spriteTwoTextureData,
	_In_ Windows::Foundation::Rect& spriteTwoPosition
	)
{
	// Start by performing a simple intersection test between the two rectangles since looping the pixels would be a waste if no pixels could possibly overlap.
	if (spriteOnePosition.IntersectsWith(spriteTwoPosition))
	{
		// Find the bounds of the rectangle intersection
		int top = static_cast<int>(max(spriteOnePosition.Top, spriteTwoPosition.Top));
		int bottom = static_cast<int>(min(spriteOnePosition.Bottom, spriteTwoPosition.Bottom));
		int left = static_cast<int>(max(spriteOnePosition.Left, spriteTwoPosition.Left));
		int right = static_cast<int>(min(spriteOnePosition.Right, spriteTwoPosition.Right));

		// Get pointers for the texture data to avoid paying the cost of calling get() repeatedly within the loop.
		uint8* spriteOneData = spriteOneTextureData.get();
		uint8* spriteTwoData = spriteTwoTextureData.get();

		// Get the needed parameters as ints.
		auto spriteOneX = static_cast<int>(spriteOnePosition.X);
		auto spriteOneY = static_cast<int>(spriteOnePosition.Y);
		auto spriteOneWidth = static_cast<int>(spriteOnePosition.Width);
		auto spriteTwoX = static_cast<int>(spriteTwoPosition.X);
		auto spriteTwoY = static_cast<int>(spriteTwoPosition.Y);
		auto spriteTwoWidth = static_cast<int>(spriteTwoPosition.Width);

		// Check every point within the intersection bounds.
		for (int y = top; y < bottom; y++)
		{
			for (int x = left; x < right; x++)
			{
				// Calculate the ((y * width) + x) value to get the pixel position then multiply by 4 because each pixel is four bytes then
				// add 3 to get the alpha value for the pixel. (Note: BGRA ordering means B = + 0, G = + 1, R = + 2, A = + 3).
				auto alphaOne = spriteOneData[(((x - spriteOneX) +
					((y - spriteOneY) * spriteOneWidth)) * 4) + 3];

				// Same formula as alphaOne just with sprite two.
				auto alphaTwo = spriteTwoData[(((x - spriteTwoX) +
					((y - spriteTwoY) * spriteTwoWidth)) * 4) + 3];

				// Check to see if both pixels have any alpha (i.e. aren't transparent).
				if ((alphaOne != 0) && (alphaTwo != 0))
				{
					// If so, an intersection has been found so we return true.
					return true;
				}
			}
		}
	}

	// If there was no rectangle intersection or if none of the pixels in the intersection rectangle were opaque or translucent for both sprites, then there was no pixel collision so return false. 
	return false;
}

Windows::Foundation::Rect DX::GetTransformedBoundingRectangle(
	_In_ Windows::Foundation::Rect boundingRectangle,
	_In_ DirectX::CXMMATRIX transformationMatrix
	)
{
	//auto transformationMatrix = XMMatrixInverse(nullptr, matrix);

	// Get all four corners in local space.
	DirectX::XMVECTOR leftTop = DirectX::XMVectorSet(boundingRectangle.Left, boundingRectangle.Top, 0.0f, 1.0f);
	DirectX::XMVECTOR rightTop = DirectX::XMVectorSet(boundingRectangle.Right, boundingRectangle.Top, 0.0f, 1.0f);
	DirectX::XMVECTOR leftBottom = DirectX::XMVectorSet(boundingRectangle.Left, boundingRectangle.Bottom, 0.0f, 1.0f);
	DirectX::XMVECTOR rightBottom = DirectX::XMVectorSet(boundingRectangle.Right, boundingRectangle.Bottom, 0.0f, 1.0f);

	// Transform all four corners into world space.
	leftTop = DirectX::XMVector3Transform(leftTop, transformationMatrix);
	rightTop = DirectX::XMVector3Transform(rightTop, transformationMatrix);
	leftBottom = DirectX::XMVector3Transform(leftBottom, transformationMatrix);
	rightBottom = DirectX::XMVector2Transform(rightBottom, transformationMatrix);

	// Find the minimum and maximum extents of the rectangle in world space.
	DirectX::XMVECTOR min = DirectX::XMVectorMin(DirectX::XMVectorMin(leftTop, rightTop),
		DirectX::XMVectorMin(leftBottom, rightBottom));
	DirectX::XMVECTOR max = DirectX::XMVectorMax(DirectX::XMVectorMax(leftTop, rightTop),
		DirectX::XMVectorMax(leftBottom, rightBottom));

	// Truncate the min for maximum bounding extents.
	min = DirectX::XMVectorTruncate(min);
	// Round the max after adding 0.5f to the X and Y components for maximum bounding extents.
	max = DirectX::XMVectorRound(DirectX::XMVectorAdd(max, DirectX::XMVectorSet(0.5f, 0.5f, 0.0f, 0.0f)));

	// Store the results back into float2s so we can return them.
	DirectX::XMFLOAT2 fMin;
	DirectX::XMStoreFloat2(&fMin, min);

	DirectX::XMFLOAT2 fMax;
	DirectX::XMStoreFloat2(&fMax, max);

	// Return the result as a rectangle
	return Windows::Foundation::Rect(fMin.x, fMin.y,
		fMax.x - fMin.x, fMax.y - fMin.y);
}

bool DX::IsTransformedPixelPerfectCollision(
	_In_ const std::unique_ptr<uint8>& spriteOneTextureData,
	_In_ float spriteOneTextureWidth,
	_In_ float spriteOneTextureHeight,
	_In_ DirectX::CXMMATRIX spriteOneWorldTransform,
	_In_ const std::unique_ptr<uint8>& spriteTwoTextureData,
	_In_ float spriteTwoTextureWidth,
	_In_ float spriteTwoTextureHeight,
	_In_ DirectX::CXMMATRIX spriteTwoWorldTransform
	)
{
	// Create a matrix to transform coordinates from sprite one local to sprite two local.
	DirectX::XMMATRIX transformOneToTwo = spriteOneWorldTransform * DirectX::XMMatrixInverse(nullptr, spriteTwoWorldTransform);

	// Create an X unit vector.
	const DirectX::XMFLOAT2 fUnitX(1.0f, 0.0f);

	// Calculate the value we need to add to a sprite two coordinate when we move one pixel column right in sprite one's local space.
	DirectX::XMFLOAT2 stepX;
	DirectX::XMStoreFloat2(&stepX, DirectX::XMVector2TransformNormal(DirectX::XMLoadFloat2(&fUnitX), transformOneToTwo));

	// Create a Y unit vector.
	const DirectX::XMFLOAT2 fUnitY(0.0f, 1.0f);

	// Calculate the value we need to add to a sprite two coordinate when we move one pixel row down in sprite one's local space.
	DirectX::XMFLOAT2 stepY;
	DirectX::XMStoreFloat2(&stepY, DirectX::XMVector2TransformNormal(DirectX::XMLoadFloat2(&fUnitY), transformOneToTwo));

	// We'll be starting our loop at sprite one's upper left (0,0) and going right and down. Calculate what the corresponding
	// coordinate in B's local space is using our transformation matrix.
	DirectX::XMFLOAT2 yPositionInSpriteTwo;
	DirectX::XMStoreFloat2(&yPositionInSpriteTwo, DirectX::XMVector2Transform(DirectX::XMVectorZero(), transformOneToTwo));

	// Get pointers for the texture data to avoid paying the cost of calling get() repeatedly within the loop.
	auto dataOne = spriteOneTextureData.get();
	auto dataTwo = spriteTwoTextureData.get();

	// Get sprite two's width and height as int values to avoid unnecessary casting within the loop.
	auto texOneWidth = static_cast<unsigned int>(spriteOneTextureWidth);
	auto texTwoWidth = static_cast<int>(spriteTwoTextureWidth);
	auto texTwoHeight = static_cast<int>(spriteTwoTextureHeight);

	// Loop through sprite one by row.
	for (unsigned int spriteOneY = 0; spriteOneY < spriteOneTextureHeight; spriteOneY++)
	{
		// Start at the beginning of the row for sprite two.
		DirectX::XMFLOAT2 positionInSpriteTwo = yPositionInSpriteTwo;

		// Loop through sprite one by column.
		for (unsigned int spriteOneX = 0; spriteOneX < spriteOneTextureWidth; spriteOneX++)
		{
			// Round to the nearest pixel. Note that since sprite two values might be negative and we're using integer truncation, we need 
			// to add 0.5f to get to the nearest pixel if it's positive and -0.5f if it's negative. We don't care what we add if it's zero since
			// it will always truncate to zero.
			int spriteTwoX = static_cast<int>(positionInSpriteTwo.x + (positionInSpriteTwo.x > 0.0f ? 0.5f : -0.5f));
			int spriteTwoY = static_cast<int>(positionInSpriteTwo.y + (positionInSpriteTwo.y > 0.0f ? 0.5f : -0.5f));

			// Check to make sure the transformed coordinates are within the bounds of sprite two; if not there can't be a collision.
			if ((spriteTwoX >= 0) && (spriteTwoX < texTwoWidth) &&
				(spriteTwoY >= 0) && (spriteTwoY < texTwoHeight))
			{
				// Get the alphas of the overlapping pixels.
				auto alphaOne = dataOne[(spriteOneX * 4 + spriteOneY * texOneWidth * 4) + 3];
				auto alphaTwo = dataTwo[(spriteTwoX * 4 + spriteTwoY * texTwoWidth * 4) + 3];

				// Check to see if both pixels have any alpha (i.e. aren't transparent).
				if (alphaOne != 0 && alphaTwo != 0)
				{
					// If so, an intersection has been found so we return true.
					return true;
				}
			}

			// Move to the next sprite two pixel in the row.
			positionInSpriteTwo.x += stepX.x;
			positionInSpriteTwo.y += stepX.y;
		}

		// Move to the next sprite two row
		yPositionInSpriteTwo.x += stepY.x;
		yPositionInSpriteTwo.y += stepY.y;
	}

	// If none of the overlapping pixels were opaque in both sprites, there's no collision so return false.
	return false;
}
