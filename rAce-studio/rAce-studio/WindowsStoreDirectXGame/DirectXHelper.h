#pragma once

// Helper utilities to make Win32 APIs work with exceptions.
namespace DX
{
	// Throws a Platform::Exception if the HRESULT failed. If you pass __FILEW__ for the filename and __LINE__ for the lineNumber
	// then you will know exactly where the failure occurred. You could also modify this function to output that information to a 
	// custom logger that you might create for analytics and real world bug reports. You can pass nullptr for filename and whatever
	// valid lineNumber value you want (e.g. the MAXULONG32 macro) if you don't want this information.
	inline void ThrowIfFailed(HRESULT hr, const wchar_t* filename, unsigned long lineNumber)
	{
		if (FAILED(hr))
		{
			std::wstringstream msg;
			msg << L"Failed HRESULT 0x" <<
				std::hex << std::uppercase << static_cast<unsigned long>(hr) << L" in file " <<
				(filename == nullptr ? L"(No filename passed)" : filename) << L" at line " << std::dec << lineNumber << L".\n";

#if defined (_DEBUG)
			OutputDebugStringW(msg.str().c_str());

			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr, ref new Platform::String(msg.str().c_str()));
#else
			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr, ref new Platform::String(msg.str().c_str()));
#endif
		}
	}
}
