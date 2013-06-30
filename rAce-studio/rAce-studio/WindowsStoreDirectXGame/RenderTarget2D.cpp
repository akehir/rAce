#include "pch.h"
#include "RenderTarget2D.h"


RenderTarget2D::RenderTarget2D(void) :
	Texture2D(),
	m_rtv(),
	m_dsv()
{
}


RenderTarget2D::~RenderTarget2D(void)
{
}

void RenderTarget2D::CreateRenderTarget(
	_In_ ID3D11Device* device,
	_In_ UINT width,
	_In_ UINT height,
	_In_ DXGI_FORMAT format,
	_In_ bool createDepthStencilBuffer,
	_In_ DXGI_FORMAT depthStencilFormat,
	_In_ UINT preferredMultisamplingCount,
	_In_ UINT preferredMultisamplingQuality,
	_In_ bool onlyNeedsPointSampling
	)
{
	CreateRenderTargetEx(
		device,
		width,
		height,
		D3D11_USAGE_DEFAULT,
		0L,
		format,
		createDepthStencilBuffer,
		depthStencilFormat,
		preferredMultisamplingCount,
		preferredMultisamplingQuality,
		onlyNeedsPointSampling
		);
}

void RenderTarget2D::CreateRenderTargetEx(
	_In_ ID3D11Device* device,
	_In_ UINT width,
	_In_ UINT height,
	_In_ D3D11_USAGE usage,
	_In_ LONG cpuAccessFlags,
	_In_ DXGI_FORMAT format,
	_In_ bool createDepthStencilBuffer,
	_In_ DXGI_FORMAT depthStencilFormat,
	_In_ UINT preferredMultisamplingCount,
	_In_ UINT preferredMultisamplingQuality,
	_In_ bool onlyNeedsPointSampling
	)
{
	// Validate multisample values and ensure that they are supported for the format we're looking to use. Also validate depth stencil if it will be used.
	DX::ValidateMultisampleValues(device, format, &preferredMultisamplingCount, &preferredMultisamplingQuality);

	// Validate the multisample values for the depth stencil buffer if we are creating one (since a multisampled render target and depth stencil buffer must
	// match in their multisampling values).
	if (createDepthStencilBuffer)
	{
		DX::ValidateMultisampleValues(device, depthStencilFormat, &preferredMultisamplingCount, &preferredMultisamplingQuality);
	}

	// Get the format support bit flags for the specified render target format.
	UINT d3d11FormatSupport;
	DX::ThrowIfFailed(
		device->CheckFormatSupport(format, &d3d11FormatSupport), __FILEW__, __LINE__
		);

	// Check the format support flags to ensure that the adapter supports multisampling with that format if multisampling is specified.
	if (preferredMultisamplingCount > 1)
	{
		if (!(d3d11FormatSupport & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET))
		{
#if defined(_DEBUG)
			std::wstringstream debugMessage;
			debugMessage << L"The graphics card does not support format '" << DX::DXGIFormatString(format) << "' when requesting a multisampled render target usage. Defaulting to non-multisampled.\n";
			OutputDebugStringW(debugMessage.str().c_str());
#endif
			preferredMultisamplingCount = 1;
			preferredMultisamplingQuality = 0;
		}
	}

	if (createDepthStencilBuffer)
	{
		// Get the format support bit flags for the specified depth stencil buffer format.
		UINT depthStencilFormatSupport;
		DX::ThrowIfFailed(
			device->CheckFormatSupport(depthStencilFormat, &depthStencilFormatSupport), __FILEW__, __LINE__
			);

		// Check to make sure the depth stencil format is supported by the adapter.
		if (!(depthStencilFormatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))
		{
			std::wstringstream msg;
			msg << L"The graphics card does not support format '" << DX::DXGIFormatString(format) << "' when requesting a D3D11_FORMAT_SUPPORT_DEPTH_STENCIL usage.";
			auto exceptmsg = ref new Platform::String(msg.str().c_str());
			throw ref new Platform::InvalidArgumentException(exceptmsg);
		}
	}


	if (preferredMultisamplingCount <= 1)
	{
		// Check to make sure the adapter support the specified format as a render target format.
		if (!(d3d11FormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET))
		{
			std::wstringstream msg;
			msg << L"The graphics card does not support format '" << DX::DXGIFormatString(format) << "' when requesting a D3D11_FORMAT_SUPPORT_RENDER_TARGET usage.";
			auto exceptmsg = ref new Platform::String(msg.str().c_str());
			throw ref new Platform::InvalidArgumentException(exceptmsg);
		}
	}

	// Check to make sure the adapter supports this format for use with an ID3D11Texture2D.
	if (!(d3d11FormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D))
	{
		std::wstringstream msg;
		msg << L"The graphics card does not support format '" << DX::DXGIFormatString(format) << "' when requesting a D3D11_FORMAT_SUPPORT_TEXTURE2D usage.";
		auto exceptmsg = ref new Platform::String(msg.str().c_str());
		throw ref new Platform::InvalidArgumentException(exceptmsg);
	}

	if (preferredMultisamplingCount <= 1)
	{
		// Check to make sure the adapter supports this format for non-point sampling. If not then point sampling would still be supported so it's an error if
		// the program requires sampling other than point sampling for this render target.
		if (!(d3d11FormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) && !onlyNeedsPointSampling)
		{
			std::wstringstream msg;
			msg << L"The graphics card does not support format '" << DX::DXGIFormatString(format) << "' when requesting a D3D11_FORMAT_SUPPORT_SHADER_SAMPLE usage. As such it only supports point sampling and the program requires other sampling types (e.g. linear or anisotropic).";
			auto exceptmsg = ref new Platform::String(msg.str().c_str());
			throw ref new Platform::InvalidArgumentException(exceptmsg);
		}
	}

	// Populate the texture description struct with the appropriate values.
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = preferredMultisamplingCount;
	texDesc.SampleDesc.Quality = preferredMultisamplingQuality;
	texDesc.Usage = usage;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = cpuAccessFlags;
	texDesc.MiscFlags = 0;

	// Store the texture description so we can easily access it later.
	m_desc = texDesc;
	m_width = static_cast<float>(texDesc.Width);
	m_height = static_cast<float>(texDesc.Height);

	// Create the ID3D11Texture2D resource.
	DX::ThrowIfFailed(
		device->CreateTexture2D(&texDesc, nullptr, &m_texture), __FILEW__, __LINE__
		);

	// Populate the render target view description with the appropriate values. Note that the '= {};' will zero out the struct.
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = format;
	if (preferredMultisamplingCount > 1)
	{
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
	}

	// Create an ID3D11RenderTargetView of the texture resource so that we can bind it for use as a render target.
	DX::ThrowIfFailed(
		device->CreateRenderTargetView(m_texture.Get(), &rtvDesc, &m_rtv), __FILEW__, __LINE__
		);

	// To draw the render target to another render target (including the back buffer), we need a shader resource view of it.
	// Populate the shader resource view description with the appropriate values.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	if (preferredMultisamplingCount > 1)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
	}

	// Create an ID3D11ShaderResourceView of the texture resource so that we can bind it for use in shaders.
	DX::ThrowIfFailed(
		device->CreateShaderResourceView(m_texture.Get(), &srvDesc, &m_srv), __FILEW__, __LINE__
		);

	if (createDepthStencilBuffer)
	{
		// We need a second texture resource for a depth stencil buffer.
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilTexture;

		// It will have almost entirely the same values as the render target so we only change the
		// texture description values that are different for the depth stencil buffer.
		texDesc.Format = depthStencilFormat;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		// Create the ID3D11Texture2D for the depth stencil buffer.
		DX::ThrowIfFailed(
			device->CreateTexture2D(&texDesc, nullptr, &depthStencilTexture), __FILEW__, __LINE__
			);

		// Populate a depth stencil view description.
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = depthStencilFormat;
		if (preferredMultisamplingCount > 1)
		{
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
		}

		// Create the ID3D11DepthStencilView of the depth stencil texture resource so that we can bind it for use as a depth stencil buffer.
		DX::ThrowIfFailed(
			device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvDesc, &m_dsv), __FILEW__, __LINE__
			);
	}
}
