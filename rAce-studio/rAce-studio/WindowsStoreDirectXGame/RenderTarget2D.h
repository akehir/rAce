#pragma once
#include "texture2d.h"

// A class for creating and managing a 2D render target. Derives from Texture2D.
class RenderTarget2D : public Texture2D
{
public:
	// Constructor.
	RenderTarget2D(void);

	// Destructor.
	virtual ~RenderTarget2D(void);

	// Does the actual work of creating the render target.
	// device - The ID3D11Device used by the game.
	// width - The width you want the render target to be. Does not need to match the back buffer.
	// height - The height you want the render target to be. Does not need to match the back buffer.
	// format - The DXGI_FORMAT of the render target. To support Feature Level 9.1 you would typically want either DXGI_FORMAT_B8G8R8A8_UNORM or DXGI_FORMAT_R8G8B8A8_UNORM. See: http://msdn.microsoft.com/en-us/library/ff471324(v=vs.85).aspx (note that there is a documentation error claiming that B8G8R8A8 requires FL 9.3 which I have just recently reported).
	// createDepthStencilBuffer - Set to true if you want a depth stencil buffer. If you do not require one, set this to false. If in doubt, use true.
	// depthStencilFormat - The DXGI_FORMAT of the depth stencil buffer. Typically either DXGI_FORMAT_D24_UNORM_S8_UINT or DXGI_FORMAT_D16_UNORM for FL 9.1 support. Only applies if createDepthStencilBuffer is true.
	// preferredMultisamplingCount - The multisampling count (MSAA) you would prefer. A value of 1 means no multisampling. You can use DX::ValidateMultisampleValues from Utility.h to check values before passing them in. Invalid values will be corrected automatically by this function. Hence the "requested" prefix.
	// preferredMultisamplingQuality - The multisampling quality (MSAA) you would prefer. A value of 0 (with a 1 for count) means no multisampling. Typically, NVIDIA cards use multiple quality settings while AMD cards use just a single quality setting. DX::ValidateMultisampleValues will correct a quality setting to the highest supported value assuming that a correct count value is provided.
	// onlyNeedsPointSampling - Set to true if you will only need to do point sampling when sampling from this render target in a shader. Some formats that are supported for a certain feature level (e.g. DXGI_FORMAT_R16G16B16A16_FLOAT with FL 9.2 and up) for a render target only support point sampling (the previous example optionally supports linear and aniso filtering only in some FL 9.3 cards). If you're sticking to FL 9.1 supported formats this should not be an issue.
	void CreateRenderTarget(
		_In_ ID3D11Device* device,
		_In_ UINT width,
		_In_ UINT height,
		_In_ DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM,
		_In_ bool createDepthStencilBuffer = true,
		_In_ DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		_In_ UINT preferredMultisamplingCount = 1,
		_In_ UINT preferredMultisamplingQuality = 0,
		_In_ bool onlyNeedsPointSampling = false
		);

	// Does the actual work of creating the render target.
	// device - The ID3D11Device used by the game.
	// width - The width you want the render target to be. Does not need to match the back buffer.
	// height - The height you want the render target to be. Does not need to match the back buffer.
	// format - The DXGI_FORMAT of the render target. To support Feature Level 9.1 you would typically want either DXGI_FORMAT_B8G8R8A8_UNORM or DXGI_FORMAT_R8G8B8A8_UNORM. See: http://msdn.microsoft.com/en-us/library/ff471324(v=vs.85).aspx (note that there is a documentation error claiming that B8G8R8A8 requires FL 9.3 which I have just recently reported).
	// createDepthStencilBuffer - Set to true if you want a depth stencil buffer. If you do not require one, set this to false. If in doubt, use true.
	// depthStencilFormat - The DXGI_FORMAT of the depth stencil buffer. Typically either DXGI_FORMAT_D24_UNORM_S8_UINT or DXGI_FORMAT_D16_UNORM for FL 9.1 support. Only applies if createDepthStencilBuffer is true.
	// preferredMultisamplingCount - The multisampling count (MSAA) you would prefer. A value of 1 means no multisampling. You can use DX::ValidateMultisampleValues from Utility.h to check values before passing them in. Invalid values will be corrected automatically by this function. Hence the "requested" prefix.
	// preferredMultisamplingQuality - The multisampling quality (MSAA) you would prefer. A value of 0 (with a 1 for count) means no multisampling. Typically, NVIDIA cards use multiple quality settings while AMD cards use just a single quality setting. DX::ValidateMultisampleValues will correct a quality setting to the highest supported value assuming that a correct count value is provided.
	// onlyNeedsPointSampling - Set to true if you will only need to do point sampling when sampling from this render target in a shader. Some formats that are supported for a certain feature level (e.g. DXGI_FORMAT_R16G16B16A16_FLOAT with FL 9.2 and up) for a render target only support point sampling (the previous example optionally supports linear and aniso filtering only in some FL 9.3 cards). If you're sticking to FL 9.1 supported formats this should not be an issue.
	void CreateRenderTargetEx(
		_In_ ID3D11Device* device,
		_In_ UINT width,
		_In_ UINT height,
		_In_ D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		_In_ LONG cpuAccessFlags = 0,
		_In_ DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM,
		_In_ bool createDepthStencilBuffer = true,
		_In_ DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		_In_ UINT preferredMultisamplingCount = 1,
		_In_ UINT preferredMultisamplingQuality = 0,
		_In_ bool onlyNeedsPointSampling = false
		);

	// Returns the render target view in the proper format for a call to ID3D11DeviceContext::OMSetRenderTargets. If using MRT (not supported in FL 9.1) then you will need to dereference this once and include it in your array of render targets, e.g. ID3D11RenderTargetView* rtvs[] = { *m_renderTarget1.GetRTV(), *m_renderTarget1.GetRTV() };
	ID3D11RenderTargetView* const* GetRTV() const { return m_rtv.GetAddressOf(); }

	// Gets the depth stencil buffer (assuming you specified that one be created).
	ID3D11DepthStencilView* GetDSV() const { return m_dsv.Get(); }

	// Resets the RenderTarget2D. This resets the internal ComPtr objects for all of the COM member variables the RenderTarget2D has (including those from Texture2D). Assuming no other references exist to these objects, they will be destroyed.
	virtual void Reset() override { Texture2D::Reset(); m_rtv.Reset(); m_dsv.Reset(); }

protected:
	// The render target view.
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_rtv;

	// The depth stencil buffer.
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      m_dsv;
};
