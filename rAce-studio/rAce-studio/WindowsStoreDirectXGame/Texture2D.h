#pragma once
#include <d3d11_1.h>
#include <wrl\client.h>

class Texture2D
{
public:
	// Constructor. Does not actually create a texture. Use Load/LoadAsync or else manually create the Texture2D with SettableTexture2D, SettableSRV, and SetDesc.
	Texture2D();

	// Destructor.
	virtual ~Texture2D();

	// Move constructor.
	Texture2D(Texture2D&& value) :
		m_texture(nullptr),
		m_srv(nullptr),
		m_desc(),
		m_width(),
		m_height()
	{
		// Invoke the move assignment operator.
		*this = std::move(value);
	}

	// Move assignment operator.
	Texture2D& operator=(Texture2D&& value)
	{
		if (this != &value)
		{
			m_texture.Swap(value.m_texture);
			m_srv.Swap(value.m_srv);
			m_desc = value.m_desc;
			m_width = value.m_width;
			m_height = value.m_height;
		}

		return *this;
	}

	// Loads a texture from a file.
	// device - The ID3D11Device to use to create the texture and SRV.
	// context - Does nothing if isDDS == true, otherwise if it's not null then mipmaps will be autogenerated (if possible) for the texture.
	// filename - The file to load the texture data from.
	// token - A cancellation_token from a cancellation_token_source, which can be used to cancel this task if needed.
	// isDDS - Set this to true if you are loading a DDS file, false if loading another format (e.g. PNG, JPG, BMP).
	concurrency::task<void> LoadAsync(
		_In_ ID3D11Device* device,
		_In_opt_ ID3D11DeviceContext* context,
		_In_ LPCWSTR filename,
		_In_ concurrency::cancellation_token token,
		_In_ bool isDDS
		);

	// Loads a texture from memory.
	// device - The ID3D11Device to use to create the texture and SRV.
	// context - Does nothing if isDDS == true, otherwise if it's not null then mipmaps will be autogenerated (if possible) for the texture.
	// data - The texture data to create the texture from.
	// dataSize - The size, in bytes, of the texture data.
	// isDDS - Set this to true if you are loading a DDS file, false if loading another format (e.g. PNG, JPG, BMP).
	void Load(
		_In_ ID3D11Device* device,
		_In_opt_ ID3D11DeviceContext* context,
		_In_reads_bytes_(dataSize) const void* data,
		_In_ unsigned int dataSize,
		_In_ bool isDDS
		);

	//
	// Member functions for accessing the various member variables of a Texture2D.
	//

	// Returns a pointer to the texture.
	ID3D11Texture2D* GetTexture2D() const { return m_texture.Get(); }

	// Returns a pointer to the shader resource view of the texture.
	ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

	// Returns a pointer to the D3D11_TEXTURE2D_DESC of the texture.
	const D3D11_TEXTURE2D_DESC* GetDesc() const { return &m_desc; }

	// Returns the width of the texture.
	float GetWidth() const { return m_width; }

	// Returns the height of the texture.
	float GetHeight() const { return m_height; }

	//
	// Member functions for manually creating a Texture2D.
	//

	// Returns a reference in the internal ComPtr to the texture. Use this only when you want to manually create a Texture2D.
	// Must be used in conjunction with SettableSRV and SetDesc.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>& SettableTexture2D() { return m_texture; }

	// Returns a reference to the internal ComPtr to the SRV. Use this only when you want to manually create a Texture2D.
	// Must be used in conjunction with SettableTexture2D and SetDesc.
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& SettableSRV() { return m_srv; }

	// Sets the internal D3D11_TEXTURE2D_DESC to the provided value. Use this only when you want to manually create a Texture2D. Also sets the cached width and height.
	// Must be used in conjunction with SettableTexture2D and SettableSRV.
	void SetDesc(_In_ const D3D11_TEXTURE2D_DESC& desc) { m_desc = desc; m_width = static_cast<float>(desc.Width); m_height = static_cast<float>(desc.Height); }

	//
	// Housekeeping member functions.
	//

	// Release the texture and SRV, and reset the D3D11_TEXTURE2D_DESC to default values.
	virtual void Reset() { m_texture.Reset(); m_srv.Reset(); m_desc = D3D11_TEXTURE2D_DESC(); }

protected:
	// The underlying texture resource.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_texture;

	// The SRV that allows the texture to be bound as input for a shader.
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_srv;

	// The texture desc, which stores a variety of helpful info about the texture (e.g. format, dimensions).
	D3D11_TEXTURE2D_DESC								m_desc;

	// The texture's width as a float.
	float												m_width;

	// The texture's height as a float.
	float												m_height;
};
