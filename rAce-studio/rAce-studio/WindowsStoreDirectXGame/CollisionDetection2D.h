#pragma once

#include <Windows.h>
#include <wrl\client.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <memory>

#include "SpriteBatch.h"
#include "Texture2D.h"
#include "RenderTarget2D.h"
#include "Utility.h"

namespace DX
{
	// Returns the pixel data in B8G8R8A8 format of a texture with one of a limited number of formats. Unlike GetTexture2DCollisionData, there is no render target step in this method so no context
	// settings will be lost. The data is copied into a second texture (which has CPU read access) which is then mapped that to read the texture data. This data will be reinterpreted from its 
	// native format into B8G8R8A8 data. This operation will take a non-trivial amount of time so it should be avoided during gameplay. This function requires the immediate context and so it will 
	// block the UI thread until it completes.
	// Supported texture formats:
	// DXGI_FORMAT_B8G8R8A8_UNORM
	// DXGI_FORMAT_R8G8B8A8_UNORM
	// DXGI_FORMAT_R32G32B32A32_FLOAT
	// DXGI_FORMAT_BC1_UNORM
	// DXGI_FORMAT_BC3_UNORM
	// Parameters:
	// device - The game's ID3D11Device.
	// context - The ID3D11DeviceContext being used with the SpriteBatch instance (typically the immediate context).
	// spriteBatch - A SpriteBatch instance we can use for drawing. Its context should match the context parameter.
	// textureSRV - An SRV of the ID3D11Texture2D resource whose data you want.
	// filename - Pass __FILEW__ for this parameter (or nullptr if you don't want the file name thrown used in any exception messages).
	// lineNumber - Pass __LINE__ for this parameter to know which line of your game's code an exception came from. Helps with debugging.
	// Return value - The pixel data of the textureSRV in BGRA format. So the first pixel's blue value would be a byte at offset 0, green at offset 1, red at offset 2, and alpha at offset 3. 
	// Throws a Platform::COMException with an HRESULT of ERROR_GRAPHICS_INVALID_PIXELFORMAT (defined in winerror.h) if a texture with an unsupported DXGI_FORMAT is passed in.
	std::unique_ptr<uint8> GetTexture2DCollisionDataNoRender(
		_In_ ID3D11Device* device,
		_In_ ID3D11DeviceContext* context,
		_In_ ID3D11ShaderResourceView* textureSRV,
		_In_opt_z_ const wchar_t* filename,
		_In_ unsigned long lineNumber
		);

	// Returns the pixel data in B8G8R8A8 format of any texture (regardless of format). Note that the context's state (e.g. render targets, view ports, shaders, etc.) will be modified since this 
	// data method involves drawing the texture to a temporary render target, copying the data to a second texture (which has CPU read access), and then maps that to read the texture data. This
	// operation will take a non-trivial amount of time so it should be avoided during gameplay. This function requires the immediate context and so it will block the UI thread until it completes.
	// device - The game's ID3D11Device.
	// context - The ID3D11DeviceContext being used with the SpriteBatch instance (typically the immediate context).
	// spriteBatch - A SpriteBatch instance we can use for drawing. Its context should match the context parameter.
	// textureSRV - An SRV of the ID3D11Texture2D resource whose data you want.
	// filename - Pass __FILEW__ for this parameter (or nullptr if you don't want the file name thrown used in any exception messages).
	// lineNumber - Pass __LINE__ for this parameter to know which line of your game's code an exception came from. Helps with debugging.
	// Return value - The pixel data of the textureSRV in BGRA format. So the first pixel's blue value would be a byte at offset 0, green at offset 1, red at offset 2, and alpha at offset 3. 
	std::unique_ptr<uint8> GetTexture2DCollisionData(
		_In_ ID3D11Device* device, 
		_In_ ID3D11DeviceContext* context, 
		_In_ DirectX::SpriteBatch* spriteBatch,
		_In_ DirectX::CommonStates* commonStates,
		_In_ ID3D11ShaderResourceView* textureSRV,
		_In_opt_z_ const wchar_t* filename,
		_In_ unsigned long lineNumber
		);

	// Performs a rectangle collision check and returns true if there is a collision, false if not..
	// spriteOnePosition - The position for the first sprite.
	// spriteTwoPosition - The position for the second sprite.
	inline bool IsRectangleCollision(
		_In_ Windows::Foundation::Rect& spriteOnePosition,
		_In_ Windows::Foundation::Rect& spriteTwoPosition
		)
	{
		// Yes, it is this easy and you can just use this directly in your code if desired.
		return spriteOnePosition.IntersectsWith(spriteTwoPosition);
	}

	// Performs a rectangle collision check and returns the corresponding collision rectangle if there is collision or a rectangle of all zeros if no collision exists.
	// spriteOnePosition - The position for the first sprite.
	// spriteTwoPosition - The position for the second sprite.
	// Returns the rectangle corresponding with the collision area if there is a collision. If not, returns a rectangle of all zeros. (Note that this is not the same as Rect::Empty.)
	inline Windows::Foundation::Rect GetRectangleCollisionIntersection(
		_In_ Windows::Foundation::Rect& spriteOnePosition,
		_In_ Windows::Foundation::Rect& spriteTwoPosition
		)
	{
		// Check for intersection.
		if (spriteOnePosition.IntersectsWith(spriteTwoPosition))
		{
			// Calculate the intersection rectangle.
			auto x = max(spriteOnePosition.X, spriteTwoPosition.X);
			auto width = min(spriteOnePosition.Right, spriteTwoPosition.Right) - x;
			auto y = max(spriteOnePosition.Y, spriteTwoPosition.Y);
			auto height = min(spriteOnePosition.Bottom, spriteTwoPosition.Bottom) - y;

			return Windows::Foundation::Rect(x, y, width, height);
		}

		// There was no intersection so return a rect of all zeros.
		return Windows::Foundation::Rect(0.0f, 0.0f, 0.0f, 0.0f);
	}

	// Performs non-transformed pixel perfect collision detection.
	// spriteOneTextureData - The texture data for the first sprite.
	// spriteOnePosition - The position for the first sprite. Note that the width and height must match the first texture's width and height. Use IsTransformedPixelPerfectCollision if you need scaling.
	// spriteTwoTextureData - The texture data for the second sprite.
	// spriteTwoPosition - The position for the second sprite. Note that the width and height must match the second texture's width and height. Use IsTransformedPixelPerfectCollision if you need scaling.
	// Returns true if collision was detected, false if not.
	bool IsPixelPerfectCollision(
		_In_ const std::unique_ptr<uint8>& spriteOneTextureData,
		_In_ Windows::Foundation::Rect& spriteOnePosition,
		_In_ const std::unique_ptr<uint8>& spriteTwoTextureData,
		_In_ Windows::Foundation::Rect& spriteTwoPosition
		);

	// Gets the bounding rectangle for a sprite which has been transformed (by scaling, rotating, translating, or some combination thereof).
	// boundingRectangle - The untransformed bounding rectangle of the sprite. Should be (0, 0, width, height).
	// transformationMatrix - The matrix which combines all the transforms for the rectangle. Right hand coordinate system so the transform order should be translate for origin offset, scale, rotate (Z axis), translate for position.
	Windows::Foundation::Rect GetTransformedBoundingRectangle(
		_In_ Windows::Foundation::Rect boundingRectangle,
		_In_ DirectX::CXMMATRIX transformationMatrix
		);

	// Performs transformed (scaling, rotation, translation (including non (0,0) origin), or any combination thereof) pixel perfect collision detection.
	// spriteOneTextureData - The texture data for the first sprite.
	// spriteOneTextureWidth - The unscaled pixel width of the texture for the first sprite.
	// spriteOneTextureHeight - The unscaled pixel height of the texture for the first sprite.
	// spriteOneWorldTransform - The transform matrix for the first sprite. Right hand coordinate system so the transform order should be translate for origin offset, scale, rotate (Z axis), translate for position.
	// spriteTwoTextureWidth - The unscaled pixel width of the texture for the second sprite.
	// spriteTwoTextureHeight - The unscaled pixel height of the texture for the second sprite.
	// spriteTwoWorldTransform - The transform matrix for the second sprite. Right hand coordinate system so the transform order should be translate for origin offset, scale, rotate (Z axis), translate for position.
	// spriteTwoTextureData - The texture data for the second sprite.
	// Returns true if collision was detected, false if not.
	bool IsTransformedPixelPerfectCollision(
		_In_ const std::unique_ptr<uint8>& spriteOneTextureData,
		_In_ float spriteOneTextureWidth,
		_In_ float spriteOneTextureHeight,
		_In_ DirectX::CXMMATRIX spriteOneWorldTransform,
		_In_ const std::unique_ptr<uint8>& spriteTwoTextureData,
		_In_ float spriteTwoTextureWidth,
		_In_ float spriteTwoTextureHeight,
		_In_ DirectX::CXMMATRIX spriteTwoWorldTransform
		);
}
