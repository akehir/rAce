#include "pch.h"
#include "Texture2D.h"

using namespace DirectX;

Texture2D::Texture2D() :
	m_texture(),
	m_srv(),
	m_desc(),
	m_width(),
	m_height()
{
}

Texture2D::~Texture2D()
{
}

concurrency::task<void> Texture2D::LoadAsync(
	_In_ ID3D11Device* device,
	_In_opt_ ID3D11DeviceContext* context,
	_In_ LPCWSTR filename,
	_In_ concurrency::cancellation_token token,
	_In_ bool isDDS
	)
{
	// Turn the filename into a platform string to keep it alive during async (and because we need it in that
	// format anyway).
	auto fn = ref new Platform::String(filename);

	// Begin the continuation chain.
	return concurrency::create_task([this, device, context, isDDS, fn]()
	{
#if defined (_DEBUG)
		assert(IsBackgroundThread());
#endif
		// Check for cancellation.
		if (concurrency::is_task_cancellation_requested())
		{
			// This cancels this task and all subsequent tasks without worry about 
			concurrency::cancel_current_task();
		}

		// Return the async operation that gets the file.
		return Windows::ApplicationModel::Package::Current->InstalledLocation->GetFileAsync(fn);
	}, token).then([this, device, context, isDDS, fn](Windows::Storage::StorageFile^ file)
	{
		// Notice that this lambda takes as a parameter the file that was loaded by the async operation
		// that the previous task in the chain returned.

		if (concurrency::is_task_cancellation_requested())
		{
			concurrency::cancel_current_task();
		}

		if (file == nullptr)
		{
			throw ref new Platform::InvalidArgumentException("The file failed to load.");
		}

		// Return the async operation that loads the file's data into a WinRT IBuffer.
		return Windows::Storage::FileIO::ReadBufferAsync(file);
	}, token).then([this, device, context, isDDS, fn](Windows::Storage::Streams::IBuffer^ buffer)
	{
		if (concurrency::is_task_cancellation_requested())
		{
			concurrency::cancel_current_task();
		}

		// Create a new data array for the data from the IBuffer.
		auto data = ref new Platform::Array<byte>(buffer->Length);
		// Read the data from the IBuffer into the array.
		Windows::Storage::Streams::DataReader::FromBuffer(buffer)->ReadBytes(data);

		if (concurrency::is_task_cancellation_requested())
		{
			concurrency::cancel_current_task();
		}

		if (isDDS)
		{
			// CreateDDSTextureFromMemory expects and ID3D11Resource** to store the underlying
			// texture that it loads. This is because it can load more than just an ID3D11Texture2D.
			// So we create a variable we can load it in to. If we didn't want the underlying texture 
			// at all, we could just pass nullptr there instead.
			Microsoft::WRL::ComPtr<ID3D11Resource> resource;
			DX::ThrowIfFailed(
				CreateDDSTextureFromMemory(
				device,
				data->Data,
				static_cast<size_t>(data->Length),
				&resource,
				&m_srv
				), __FILEW__, __LINE__
				);

			// Since we intended to load an ID3D11Texture2D, we use ComPtr::As to QI the resource as an
			// ID3D11Texture2D (the type stored in m_texture).
			DX::ThrowIfFailed(
				resource.As(&m_texture), __FILEW__, __LINE__
				);
		}
		else
		{
			// See the description above for why we need this.
			Microsoft::WRL::ComPtr<ID3D11Resource> resource;
			DX::ThrowIfFailed(
				CreateWICTextureFromMemory(
				device,
				context,
				data->Data,
				static_cast<size_t>(data->Length),
				&resource,
				&m_srv
				), __FILEW__, __LINE__
				);

			DX::ThrowIfFailed(
				resource.As(&m_texture), __FILEW__, __LINE__
				);
		}

		if (concurrency::is_task_cancellation_requested())
		{
			concurrency::cancel_current_task();
		}

		m_texture->GetDesc(&m_desc);
		m_width = static_cast<float>(m_desc.Width);
		m_height = static_cast<float>(m_desc.Height);
	}, token);
}

void Texture2D::Load(
	_In_ ID3D11Device* device,
	_In_opt_ ID3D11DeviceContext* context,
	_In_reads_bytes_(dataSize) const void* data,
	_In_ unsigned int dataSize,
	_In_ bool isDDS
	)
{
	if (dataSize <= 0)
	{
		// dataSize cannot be 0 if loading from memory; we need the value and can't guess at it.
		DX::ThrowIfFailed(
			E_INVALIDARG, __FILEW__, __LINE__
			);
	}

	if (isDDS)
	{
		// See the description in LoadAsync for why we create this temporary ComPtr<ID3D11Resource>.
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;

		// Create the texture from memory.
		DX::ThrowIfFailed(
			CreateDDSTextureFromMemory(device, static_cast<const uint8_t*>(data), dataSize, &resource, &m_srv), __FILEW__, __LINE__
			);

		DX::ThrowIfFailed(
			resource.As(&m_texture), __FILEW__, __LINE__
			);
	}
	else
	{
		// See the description in LoadAsync for why we create this temporary ComPtr<ID3D11Resource>.
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;

		// Create the texture from memory.
		DX::ThrowIfFailed(
			CreateWICTextureFromMemory(device, context, static_cast<const uint8_t*>(data), dataSize, &resource, &m_srv), __FILEW__, __LINE__
			);

		DX::ThrowIfFailed(
			resource.As(&m_texture), __FILEW__, __LINE__
			);
	}

	// Get the texture description and store it.
	m_texture->GetDesc(&m_desc);
	m_width = static_cast<float>(m_desc.Width);
	m_height = static_cast<float>(m_desc.Height);
}
