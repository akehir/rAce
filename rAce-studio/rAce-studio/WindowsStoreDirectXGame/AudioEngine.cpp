#include "pch.h"
#include "AudioEngine.h"

#include "DirectXHelper.h"
#include "MediaStreamer.h"

#include <mfapi.h>
#include <mfmediaengine.h>
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
#include <Mferror.h>
#endif

#undef min
#undef max

using namespace Microsoft::WRL;
using namespace WindowsStoreDirectXGame;

MediaFoundationStartupShutdown::MediaFoundationStartupShutdown() :
	m_wasStarted(),
	m_srwLock(new SRWLOCK())
{
	InitializeSRWLock(m_srwLock.get());
}

MediaFoundationStartupShutdown::~MediaFoundationStartupShutdown()
{
	// Only call MFShutdown if we successfully called MFStartup.
	if (m_wasStarted)
	{
#if defined(_DEBUG)
		DX::ThrowIfFailed(
			MFShutdown(), __FILEW__, __LINE__
			);
#else
		MFShutdown();
#endif
		m_wasStarted = false;
	}
}

bool MediaFoundationStartupShutdown::Startup()
{
	// Acquire (and guarantee the release of) an exclusive lock on our SRWLOCK.
	auto lock = Microsoft::WRL::Wrappers::SRWLock::LockExclusive(m_srwLock.get());

	// Don't start it a second time.
	if (m_wasStarted)
	{
		return m_wasStarted;
	}

	// If it's an N or KN version of Windows then MFStartup will fail.
	HRESULT hr = MFStartup(MF_VERSION);
	if (FAILED(hr))
	{
		// If it failed, set m_wasStarted to false so that we know there was a failure.
		m_wasStarted = false;
	}
	else
	{
		m_wasStarted = true;
	}

	// Return the value of m_wasStarted so that AudioEngine (or whatever class is calling this) knows
	// whether Media Foundation is available.
	return m_wasStarted;
}

void MediaFoundationStartupShutdown::Shutdown()
{
	// Acquire (and guarantee the release of) an exclusive lock on our SRWLOCK.
	auto lock = Microsoft::WRL::Wrappers::SRWLock::LockExclusive(m_srwLock.get());

	// Don't shutdown if it's not started.
	if (!m_wasStarted)
	{
		return;
	}

#if defined(_DEBUG)
	DX::ThrowIfFailed(
		MFShutdown(), __FILEW__, __LINE__
		);
#else
	MFShutdown();
#endif

	m_wasStarted = false;
}

STDMETHODIMP MediaEngineNotify::EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2)
{
	switch (meEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_ERROR:
		// We ignore NOERROR and ABORTED (ABORTED happens when "The process of fetching the media resource was stopped at the user's request").
		if (param1 != MF_MEDIA_ENGINE_ERR_NOERROR && param1 != MF_MEDIA_ENGINE_ERR_ABORTED)
		{
			m_errorOccurred = true;
			m_errorType = static_cast<MF_MEDIA_ENGINE_ERR>(param1);
			m_hresult = static_cast<HRESULT>(param2);
		}
		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:
		// The music came to a normal end. This is not triggered when the music is supposed to loop; only when it's finally done playing.
		m_musicIsPlaying = false;
		m_previousMusicFinished = true;
		break;
	case MF_MEDIA_ENGINE_EVENT_PLAYING:
		// We cannot seek until the music is actually playing. So if we need to seek, m_readyToSeek and m_musicIsPlaying are set to true here so
		// that we know we can safely seek. We use seeking to resume at the correct point when we need to interrupt the music in order to play
		// the test sound for setting volume levels in the game settings.
		m_musicIsPlaying = true;
		m_readyToSeek = true;
		break;
	default:
		// Do nothing.
		break;
	}

	return S_OK;
}

void MediaEngineNotify::InitializeVariables()
{
	// Initialize the class variables to their default values. Normally we would do this in a constructor, but
	// since we are using Microsoft::WRL::RuntimeClass<T> to make implementing IMFMediaEngineNotify much
	// simpler, the constructor is defined for us, ergo this initializer member function.
	m_errorOccurred = false;
	m_errorType = MF_MEDIA_ENGINE_ERR_NOERROR;
	m_hresult = S_OK;

	m_musicIsPlaying = false;

	m_readyToSeek = false;

	m_previousMusicFinished = false;
}

AudioEngine::AudioEngine() :
	m_soundEffectsEngineCallbacks(),
	m_musicEngine(),
	m_soundEffectsEngine(),
	m_mediaEngineNotify(),
	m_masteringVoice(),
	m_soundEffectsMap(),
	m_musicQueue(),
	m_mediaFoundationStartupShutdown(),
	m_musicDisabledNoMediaFoundation(),
	m_musicOff(),
	m_soundEffectsOff(),
	m_musicVolume(100.0),
	m_soundEffectsVolume(100.0),
	m_musicIsPlaying(),
	m_musicIsPaused(),
	m_musicPosition(-1.0) // We use a negative number to denote that we do not need to seek to any particular point.
{
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::InitializeSoundEffectsEngine()
{
	// First shutdown any existing sound effects engine so that resources are properly released and we don't have any voices
	// leftover that came from an old instance of the sound effects engine.
	ShutdownSoundEffectsEngine();

#if !defined(_DEBUG)
	// In a debug build we want any failures to throw so we can know about them. In a release build you should use the various
	// catch statements to log the problem instead (and then re-throw or continue without sound effects, depending on your preference).
	try
	{
#endif
		// Create the IXAudio2 engine. Note that with XAudio2, the IXAudio2 object is a COM object but the voices are not COM
		// objects (and thus do not use ComPtr for their management). XAPO effects are also COM objects. This simple audio engine
		// does not make use of any XAPO effects. For examples of how to use those, see, e.g., the Marble Maze C++ sample in the
		// MSDN Code Gallery.
		DX::ThrowIfFailed(
			XAudio2Create(&m_soundEffectsEngine), __FILEW__, __LINE__
			);

		// Sign up for engine callbacks so we can receive notice of errors and other events.
		DX::ThrowIfFailed(
			m_soundEffectsEngine->RegisterForCallbacks(&m_soundEffectsEngineCallbacks), __FILEW__, __LINE__
			);

		HRESULT hr = m_soundEffectsEngine->CreateMasteringVoice(&m_masteringVoice);

		if (FAILED(hr))
		{
			// No default audio device was found. This can happen if the computer doesn't have an audio device or
			// possibly during error recovery when trying to recreate the sound effects engine. The latter issue
			// has been resolved in this code but could reappear if you modify the code in a way that fails to
			// completely destroy all of the old objects that could be causing the previous IXAudio2 engine to 
			// not be destroyed before the new engine is created.
			if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
			{
				m_soundEffectsOff = true;
				m_soundEffectsEngine->UnregisterForCallbacks(&m_soundEffectsEngineCallbacks);
				m_soundEffectsEngine.Reset();
				return;
			}
			else
			{
				DX::ThrowIfFailed(hr, __FILEW__, __LINE__);
			}
		}

		DX::ThrowIfFailed(
			m_soundEffectsEngine->StartEngine(), __FILEW__, __LINE__
			);

		m_soundEffectsOff = false;

		SetSoundEffectsVolume(m_soundEffectsVolume);

#if defined (NDEBUG)
	}
	catch(Platform::Exception^ e)
	{
		// Log the error and anything else you can do to recover. Maybe set a flag to show the user an error message?


		// Set sound effects to off; this way if it was just a temporary issue, the user will be able to re-enable sound effects and move forward.
		m_soundEffectsOff = true;
	}
	catch(...)
	{
		// Do your best to figure out what went wrong based on what ComPtr pointers are not null and log it. Maybe set a flag to show the user an error message?


		// Set sound effects to off; this way if it was just a temporary issue, the user will be able to re-enable sound effects and move forward.
		m_soundEffectsOff = true;
	}
#endif
}

void AudioEngine::ShutdownSoundEffectsEngine()
{
	// Clear all source voices (if any) in the sound effects std::map.
	for (auto& voice : m_soundEffectsMap)
	{
		voice.second->m_sourceVoices.clear();
	}

	// Reset the mastering voice (if any).
	m_masteringVoice.Reset();

	// Unregister the IXAudio2EngineCallback and stop the engine, if the engine exists. 
	if (m_soundEffectsEngine != nullptr)
	{
		m_soundEffectsEngine->UnregisterForCallbacks(&m_soundEffectsEngineCallbacks);
		m_soundEffectsEngine->StopEngine();
	}

	// Reset the IXaudio2 engine (if any).
	m_soundEffectsEngine.Reset();

	// Note that sound effects are off.
	m_soundEffectsOff = true;
}

void AudioEngine::InitializeMusicEngine()
{
	// First shutdown any existing music engine to ensure that resources are properly freed and we don't have anything leftover trying
	// to communicate with any old music engine.
	ShutdownMusicEngine();

	// Attempt to startup Media Foundation.
	m_musicDisabledNoMediaFoundation = !m_mediaFoundationStartupShutdown.Startup();

	// Only proceed if starting up Media Foundation was successful.
	if (!m_musicDisabledNoMediaFoundation)
	{
		ComPtr<IMFMediaEngineClassFactory> mediaEngineFactory;
		ComPtr<IMFAttributes> mediaEngineAttributes;
		ComPtr<IMFMediaEngine> mfMediaEngine;
		ComPtr<IUnknown> notifyAsIUnknown;
#if !defined(_DEBUG)
		// In a debug build we want any failures to throw so we can know about them. In a release build you should use the various
		// catch statements to log the problem instead (and then re-throw or continue without music, depending on your preference).
		try
		{
#endif
			// Create the class factory for the Media Engine, from which we will create a media engine.
			DX::ThrowIfFailed(
				CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mediaEngineFactory)), __FILEW__, __LINE__
				);

			// Define configuration attributes for the media engine.
			UINT32 meAttrInitialSize = 1;
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
			meAttrInitialSize = 2;
#endif
			DX::ThrowIfFailed(
				MFCreateAttributes(&mediaEngineAttributes, meAttrInitialSize), __FILEW__, __LINE__
				);

			// Create an instance of MediaEngineNotify. This call does all sorts of COM stuff automatically, which is why we 
			// use Microsoft::WRL::RuntimeClass<T> rather than writing our own COM class.
			m_mediaEngineNotify = Make<MediaEngineNotify>();

			// Since we're using the RuntimeClass<T>'s constructor, we must call our InitializeVariables member function before proceeding.
			m_mediaEngineNotify->InitializeVariables();

			// To set our IMFMediaEngineNotify callback, we need to cast it as an IUnknown. Since it's a COM object at it's core, this is fine.
			DX::ThrowIfFailed(
				m_mediaEngineNotify.As(&notifyAsIUnknown), __FILEW__, __LINE__
				);
			DX::ThrowIfFailed(
				mediaEngineAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, notifyAsIUnknown.Get()), __FILEW__, __LINE__
				);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
			// If you wish to use this with a Windows Phone 8 project, we need to create this as a Frame-server mode media engine
			// rather than as an Audio mode media engine. See http://msdn.microsoft.com/en-us/library/hh447921(v=vs.85).aspx for
			// more details about these. Frame-server mode requires that we set the MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT attribute.
			DX::ThrowIfFailed(
				mediaEngineAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, static_cast<UINT32>(DXGI_FORMAT_B8G8R8A8_UNORM))
				);
#endif

			DWORD mfFlags = 0;
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
			mfFlags |= MF_MEDIA_ENGINE_AUDIOONLY;
#endif
			// Create the media engine.
			DX::ThrowIfFailed(
				mediaEngineFactory->CreateInstance(mfFlags, mediaEngineAttributes.Get(), &mfMediaEngine), __FILEW__, __LINE__
				);

			// What we actually want is an IMFMediaEngineEx but there is no way to directly get one of those. Windows 8 returns an
			// object that implements that interface when we call IMFMediaEngineClassFactory::CreateInstance though, so we can use
			// ComPtr's As member function to QI (QueryInterface) the desired IMFMediaEngineEx and store the result in our class
			// member variable.
			mfMediaEngine.As(&m_musicEngine);

			// Turn off autoplay such that merely setting a source alone will not start music playing instantly.
			DX::ThrowIfFailed(
				m_musicEngine->SetAutoPlay(FALSE), __FILEW__, __LINE__
				);

			// Turn looping off. We handle it ourselves.
			DX::ThrowIfFailed(
				m_musicEngine->SetLoop(FALSE), __FILEW__, __LINE__
				);

			// Once the music engine has been successfully created, we note that music is no longer off.
			m_musicOff = false;

			// Set the volume to whatever volume level is desired.
			SetMusicVolume(m_musicVolume);
#if !defined (_DEBUG)
		}
		catch(Platform::Exception^ e)
		{
			// Log the error and anything else you can do to recover. Maybe set a flag to show the user an error message? The sample
			// leaves it up to you to decide what to do here.


			// Set music to off; this way if it was just a temporary issue, the user will be able to re-enable music and move forward.
			m_mediaEngineNotify.Reset();

			if (m_musicEngine != nullptr)
			{
				m_musicEngine->Shutdown(); // Ignore HRESULT since we can't do anything if it fails anyway.
				m_musicEngine.Reset();
			}

			m_mediaFoundationStartupShutdown.Shutdown();
			m_musicOff = true;
		}
		catch(...)
		{
			// Do your best to figure out what went wrong based on what ComPtr pointers are not null and log it. Maybe set a flag to show the user an error message?


			// Set music to off; this way if it was just a temporary issue, the user will be able to re-enable music and move forward.
			m_mediaEngineNotify.Reset();

			if (m_musicEngine != nullptr)
			{
				m_musicEngine->Shutdown(); // Ignore HRESULT since we can't do anything if it fails anyway.
				m_musicEngine.Reset();
			}

			m_mediaFoundationStartupShutdown.Shutdown();
			m_musicOff = true;
		}
#endif
	}
}

void AudioEngine::ShutdownMusicEngine()
{
	m_mediaEngineNotify.Reset();

	if (m_musicEngine != nullptr)
	{
		m_musicEngine->Shutdown(); // Ignore HRESULT since we can't do anything if it fails anyway.
		m_musicEngine.Reset();
	}

	m_mediaFoundationStartupShutdown.Shutdown();

	m_musicOff = true;
}

UpdateErrorCodes AudioEngine::Update()
{
	UpdateErrorCodes result = UpdateErrorCodes::None;

	// Process an error in the sound effects engine.
	if (m_soundEffectsEngineCallbacks.m_error != S_OK)
	{
		// Handle error. (The following is default code that you may want to change based on your own testing and circumstances.)
		// Reset value.
		m_soundEffectsEngineCallbacks.m_error = S_OK;

		// Reset engine.
		InitializeSoundEffectsEngine();

		// Restart source voices.
		ResumeSoundEffects();

		// Set flag.
		result = result | UpdateErrorCodes::SoundEffectsEngine;
	}

	// Restart any failed sound effect source voices. You might not want to run this every time AudioEngine::Update is called.
	// Since this scans all voices in all sound effects that have been created, if you wind up with a lot of sound effects it
	// could eventually become a time sink. Further, you might simply not want some sound effects to restart if they suffer a
	// critical error since it may desynchronize the sound from the gameplay. The implementation of RestartFailedSoundEffects
	// in this sample is a general purpose demonstration of how you might go about handling failed sound effects. You should
	// tailor it for your game's needs or even disable it if that's what is best for your game.
	RestartFailedSoundEffects();

	// Process notifications from the music engine.
	if (m_mediaEngineNotify != nullptr)
	{
		// Previous music has finished playing. Move on or loop, as appropriate. The volume test sound only plays when the game is
		// paused, which should pause the current music (thereby setting m_musicIsPaused == true) such that we don't need to worry
		// about that messing up the queue.
		if (m_mediaEngineNotify->m_previousMusicFinished && m_musicIsPlaying && !m_musicIsPaused)
		{
			// Make sure the queue wasn't emptied.
			if (!m_musicQueue.empty())
			{
				// Get a reference to the item at the front of the queue without removing it.
				auto& currentMusic = m_musicQueue.front();

				// Tracks whether or not we should call PlayMusic when we're done processing.
				bool shouldPlay = false;

				// If the music is not supposed to loop or is done looping, remove it from the queue and
				// set shouldPlay equal to the music queue not being empty AND the new music piece
				// being set to autoplay.
				if (currentMusic.LoopCount == 0)
				{
					m_musicQueue.pop();
					shouldPlay = !m_musicQueue.empty() && m_musicQueue.front().AutoPlayAfterPreviousMusic;
				}
				else
				{
					// A negative LoopCount means to loop infinitely. If it's positive, decrement the loop count
					// so that it will stop looping and proceed to the next music piece when it reaches zero.
					if (currentMusic.LoopCount > 0)
					{
						--currentMusic.LoopCount;
					}
					shouldPlay = true;
				}

				// Play the music if we're supposed to.
				if (shouldPlay)
				{
					PlayMusic();
				}
			}
		}

		// When MediaEngine is ready, then check to see if we should seek to a particular position in the playing
		// music and if so seek to it.
		if (m_mediaEngineNotify->m_readyToSeek && m_musicIsPlaying)
		{
			m_mediaEngineNotify->m_readyToSeek = false;

			if (!m_musicIsPaused)
			{
				// A negative value of m_musicPosition indicates that we do not need to seek.
				if (m_musicPosition >= 0.0)
				{
					HRESULT hr = m_musicEngine->SetCurrentTime(m_musicPosition);

					if (FAILED(hr))
					{
						// Log error
#if defined(_DEBUG)
						// Convert the HRESULT to an unsigned long so we can format it using C++ Standard Library mechanisms.
						unsigned long hrAsUL = static_cast<unsigned long>(hr);
						// wstringstream is a wide character string stream. A string stream can be written to and read from just like any 
						// other C++ Standard Library stream (using << to write out, >> to read in, and using <ios> header format manipulators
						// such as std::uppercase and std::hex to output an integer value as in hex with uppercase A-F). Note that we still
						// prefix the HRESULT with a 0x since the format manipulators do not do that.
						std::wstringstream errorMessage;
						errorMessage << L"Error seeking in music. HRESULT 0x" << std::uppercase << std::hex << hrAsUL << L".\n" <<
							L"Trying to seek in file '" << (m_musicQueue.empty() ? L"(queue is empty)" : m_musicQueue.front().Filename->Data()) <<
							L"' to time " << std::dec << m_musicPosition << L" seconds.\n";
						OutputDebugStringW(errorMessage.str().c_str());

						// Determine the seekable range and output that to the debugger.
						ComPtr<IMFMediaTimeRange> mediaTimeRange;
						DX::ThrowIfFailed(
							m_musicEngine->GetSeekable(&mediaTimeRange), __FILEW__, __LINE__
							);
						OutputDebugStringW(std::wstring(L"Seekable range ").append((mediaTimeRange->ContainsTime(m_musicPosition) != FALSE) ? L"contains the time.\n" : L"does not contain the time.\n").c_str());
						for (DWORD i = 0; i < mediaTimeRange->GetLength(); ++i)
						{
							double start, end;
							mediaTimeRange->GetStart(i, &start);
							mediaTimeRange->GetEnd(i, &end);
							OutputDebugStringW(std::wstring(L"Seekable range ").append(
								std::to_wstring(i)).append(
								L" starts at ").append(
								std::to_wstring(start)).append(
								L" and ends at ").append(
								std::to_wstring(end)).append(
								L".\n").c_str());
						}
#endif
					}

					// Set m_musicPosition to a negative value to indicate that we no longer need to seek.
					m_musicPosition = -1.0;
				}
			}
		}

		// If an error has occurred, we handle it as best we can. MF_MEDIA_ENGINE_ERR_NOERROR and MF_MEDIA_ENGINE_ERR_ABORTED
		// are ignored in MediaEngineNotify and this engine is only configured to play locally stored music such that
		// we ignore MF_MEDIA_ENGINE_ERR_NETWORK (you would want to handle that if you foresee using streaming music). That
		// just leaves MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED, which we don't handle in this sample (if the source isn't supported
		// then you would need to have the same music in a different format available to play in order to handler that error)
		// and MF_MEDIA_ENGINE_ERR_DECODE, which we handle by trying to restart the music.
		if (m_mediaEngineNotify->m_errorOccurred)
		{
			m_mediaEngineNotify->m_errorOccurred = false;

			// An MF_MEDIA_ENGINE_ERR_DECODE might occur if the music stream is interrupted, so we try to restart the music if 
			// that error happened.
			if (m_mediaEngineNotify->m_errorType == MF_MEDIA_ENGINE_ERR_DECODE)
			{
				InitializeMusicEngine();

				// Try playing again.
				if (m_musicIsPlaying && !m_musicIsPaused)
				{
					PlayMusic();
				}
			}

			result = result | UpdateErrorCodes::MusicEngine;
		}
	}

	return result;
}

void AudioEngine::AddMusicToQueue(Platform::String^ filename, int loopCount)
{
	AddMusicToQueue(filename, loopCount, true);
}

void AudioEngine::AddMusicToQueue(Platform::String^ filename, int loopCount, bool autoPlayAfterPreviousMusic)
{
	// We need the full path for our call to IMFMediaEngine::SetSource so we concatenate the installed location path for the game with a path
	// seperator character and the relative path filename.
	auto music = Windows::ApplicationModel::Package::Current->InstalledLocation->Path + "\\" + filename;

	if (filename->IsEmpty())
	{
		music = "";
	}

	auto entry = MusicQueueEntry();
	entry.Filename = music;
	entry.LoopCount = loopCount;
	entry.AutoPlayAfterPreviousMusic = autoPlayAfterPreviousMusic;
	m_musicQueue.push(entry);


#if defined(_DEBUG)
	if (!music->IsEmpty())
	{
		// The following verifies that the file exists.
		WIN32_FIND_DATAW findData;
		HANDLE handle = FindFirstFileExW(music->Data(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);

		// INVALID_HANDLE_VALUE is returned when no file could be found or some other error was encountered.
		if (handle == INVALID_HANDLE_VALUE)
		{
			// When FindFirstFileEx fails, you can get more information by calling GetLastError.
			auto errCode = GetLastError();

			// Throw an exception to alert you to a bad file name.
			DX::ThrowIfFailed(HRESULT_FROM_WIN32(errCode), __FILEW__, __LINE__);
		}
		else
		{
			// We found the file. Close the returned handle to avoid leaking the resource.
			FindClose(handle);
		}
	}
#endif
}

void AudioEngine::ClearMusicQueue()
{
	while (!m_musicQueue.empty())
	{
		m_musicQueue.pop();
	}
}

void AudioEngine::MoveToNextMusicInQueue()
{
	// Remove the front song if the queue is not empty.
	if (!m_musicQueue.empty())
	{
		m_musicQueue.pop();
	}

	// Do nothing if Media Foundation failed to startup or music is set to off.
	if (m_musicDisabledNoMediaFoundation || m_musicOff)
	{
		return;
	}

	if (m_musicQueue.empty())
	{
		// Stop the current music.
		DX::ThrowIfFailed(
			m_musicEngine->Pause(), __FILEW__, __LINE__
			);
	}
	else
	{
		// Play the new current song.
		PlayMusic();
	}
}

void AudioEngine::PlayMusic()
{
	// Do nothing if Media Foundation failed to startup or music is set to off.
	if (m_musicDisabledNoMediaFoundation || m_musicOff)
	{
		return;
	}

	if (m_musicQueue.empty())
	{
		// If the queue is empty, output a warning and return.
#if defined(_DEBUG)
		OutputDebugStringW(L"The music queue is empty on a call to PlayMusic.");
		__debugbreak();
#endif
		return;
	}

	// Get the first item from the queue.
	auto filename = m_musicQueue.front().Filename;

	if (!filename->IsEmpty())
	{
		// Mark m_previousMusicFinished false in MediaEngineNotify.
		m_mediaEngineNotify->m_previousMusicFinished = false;

		// SetSource takes a BSTR, which is a COM data type which Platform::String's data imitates for our purposes. (Both are
		// pointers to immutable wide character strings with their length stored as an integer value before the wide character 
		// data begins and which are allocated in the system heap.)
		DX::ThrowIfFailed(
			m_musicEngine->SetSource(const_cast<wchar_t*>(filename->Data())), __FILEW__, __LINE__
			);

		// Play the music.
		DX::ThrowIfFailed(
			m_musicEngine->Play(), __FILEW__, __LINE__
			);
	}
	else
	{
		// Stop any currently playing music.
		if (m_musicEngine->IsPaused() == FALSE)
		{
			DX::ThrowIfFailed(
				m_musicEngine->Pause(), __FILEW__, __LINE__
				);
		}

		// Mark m_previousMusicFinished true in MediaEngineNotify. This will cause the empty item to be processed by Update and
		// removed if/when necessary.
		m_mediaEngineNotify->m_previousMusicFinished = true;
	}

	m_musicIsPlaying = true;
	m_musicIsPaused = false;
}

void AudioEngine::PlayMusicVolumeTestSound()
{
	// See AudioEngine::SetMusic for the commented version of this code. 
	if (m_musicDisabledNoMediaFoundation || m_musicOff)
	{
		return;
	}

	auto backgroundMusic = Windows::ApplicationModel::Package::Current->InstalledLocation->Path + "\\volume_test.wav";

#if defined(_DEBUG)

	WIN32_FIND_DATAW findData;
	HANDLE handle = FindFirstFileExW(backgroundMusic->Data(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);

	if (handle == INVALID_HANDLE_VALUE)
	{
		// When FindFirstFileEx fails, you can get more information by calling GetLastError.
		auto errCode = GetLastError();

		// Log the error.
		DX::ThrowIfFailed(HRESULT_FROM_WIN32(errCode), __FILEW__, __LINE__);
	}
	else
	{
		FindClose(handle);
	}

#endif

	DX::ThrowIfFailed(
		m_musicEngine->Pause(), __FILEW__, __LINE__
		);

	DX::ThrowIfFailed(
		m_musicEngine->SetSource(const_cast<wchar_t*>(backgroundMusic->Data())), __FILEW__, __LINE__
		);

	DX::ThrowIfFailed(
		m_musicEngine->Play(), __FILEW__, __LINE__
		);
}

void AudioEngine::PlaySoundEffectsVolumeTestSound()
{
	// Ensure that the volume_test.wav file is loaded as a sound effect.
	LoadSoundEffect("volume_test.wav");
	// Play it.
	PlaySoundEffect("volume_test.wav");
}

void AudioEngine::LoadSoundEffect(Platform::String^ filename)
{
	// Loads the specified file as a sound effect.
	LoadSoundEffect(filename, false);
}

void AudioEngine::LoadSoundEffect(Platform::String^ filename, bool forceReload)
{
	if (!forceReload)
	{
		// Check to see if the filename exists as a key already. If so skip reloading the file.
		if (m_soundEffectsMap.find(filename) != m_soundEffectsMap.end())
		{
#if defined(_DEBUG)
			OutputDebugStringW(std::wstring(L"File '").append(filename->Data()).append(L"' is already loaded. Skipping...\n").c_str());
#endif
			return;
		}
	}

	// Create a new sound effect using the filename as the key. Since we're working with a unique_ptr, we don't need to worry
	// about destroying any old entry since a unique_ptr will destroy itself when it goes out of scope.
	m_soundEffectsMap[filename] = std::unique_ptr<SoundEffect>(new SoundEffect());

	// Get a reference to the sound effect.
	auto& soundEffect = m_soundEffectsMap[filename];

	// C++ does not zero everything out by default, so we are zeroing out the XAUDIO2_BUFFER in the sound effect we created to
	// ensure that its member variables have the values we want.
	ZeroMemory(&soundEffect->m_audioBuffer, sizeof(soundEffect->m_audioBuffer));

	// Use a MediaStreamer instance to load and store all the data we require to create source voices for the sound effect.
	MediaStreamer soundEffectStream;
	soundEffectStream.Initialize(filename->Data());

	uint32 bufferLength = soundEffectStream.GetMaxStreamLengthInBytes();
	soundEffect->m_soundEffectBufferData = std::unique_ptr<uint8>(new uint8[bufferLength]);
	soundEffectStream.ReadAll(soundEffect->m_soundEffectBufferData.get(), bufferLength, &soundEffect->m_soundEffectBufferLength);
	soundEffect->m_waveFormatEx = soundEffectStream.GetOutputWaveFormatEx();
	soundEffect->m_soundEffectSampleRate = soundEffectStream.GetOutputWaveFormatEx().nSamplesPerSec;
	soundEffect->m_audioBuffer.AudioBytes = soundEffect->m_soundEffectBufferLength;
	soundEffect->m_audioBuffer.Flags = XAUDIO2_END_OF_STREAM; // The XAUDIO2_END_OF_STREAM flag must be set or the source voices will never mark themselves as done playing.
	soundEffect->m_audioBuffer.pAudioData = soundEffect->m_soundEffectBufferData.get();
}

void AudioEngine::UnloadSoundEffect(Platform::String^ filename)
{
	// If the filename exists as a key in the sound effect map, erase its entry.
	if (m_soundEffectsMap.find(filename) != m_soundEffectsMap.end())
	{
		auto& soundEffect = m_soundEffectsMap[filename];

		// Stop all source voices immediately.
		for (auto& sv : soundEffect->m_sourceVoices)
		{
			if (sv->m_soundEffectStarted)
			{
				DX::ThrowIfFailed(
					sv->m_soundEffectSourceVoice->Stop(), __FILEW__, __LINE__
					);
			}
		}

		// Erase the sound effect. The use of the xaudio2_voice_ptr<T> template class will ensure that any voices are destroyed properly.
		// Voice destruction can take some time and will fail if some other voice is sending data to this voice (very unlikely for the
		// intended use of SoundEffect) such that resources could silently leak. See: http://msdn.microsoft.com/en-us/library/microsoft.directx_sdk.ixaudio2voice.ixaudio2voice.destroyvoice(v=vs.85).aspx
		m_soundEffectsMap.erase(filename);
	}
}

void AudioEngine::PlaySoundEffect(Platform::String^ filename)
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Play the sound effect once with no looping and no limit on the number of instances of it that can be playing concurrently.
	PlaySoundEffect(filename, 0U, 0U);
}

void AudioEngine::PlaySoundEffect(Platform::String^ filename, uint32 loopCount)
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Play the sound effect with the specified looping and no limit on the number of instances of it that can be playing concurrently.
	PlaySoundEffect(filename, loopCount, 0U);
}

void AudioEngine::PlaySoundEffect(Platform::String^ filename, uint32 loopCount, uint32 maxInstances)
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Make sure the sound effect exists.
	if (m_soundEffectsMap.find(filename) == m_soundEffectsMap.end())
	{
#if defined(_DEBUG)
		throw ref new Platform::InvalidArgumentException(L"filename");
#endif
		return;
	}

	// Get a reference to the sound effect.
	auto& soundEffect = m_soundEffectsMap[filename];

	// A maxInstances value greater than 0 means we should cap the number of concurrently playing instances of this sound effect. This
	// can be helpful since playing the same effect many times at once can create distortions and other bad sounding results.
	if (maxInstances > 0U)
	{
		// Count the number of instances that are currently playing and return if it meets or exceeds the specified max instances cap.
		if (static_cast<uint32>(
			std::count_if(soundEffect->m_sourceVoices.begin(), soundEffect->m_sourceVoices.end(), [] (std::unique_ptr<SourceVoice>& voice) { return voice->m_soundEffectStarted == true; })
			) >= maxInstances)
		{
			return;
		}
	}

	// See if we have a source voice that's already been created that is no longer in use.
	auto voice = std::find_if(soundEffect->m_sourceVoices.begin(), soundEffect->m_sourceVoices.end(), [] (std::unique_ptr<SourceVoice>& voice) { return voice->m_soundEffectStarted == false; });

	// If we don't, we need to create a new one.
	if (voice == soundEffect->m_sourceVoices.end())
	{
		// Create a unique_ptr to a SourceVoice.
		std::unique_ptr<SourceVoice> sourceVoice(new SourceVoice());

		// Get a pointer to the SourceVoice contained by the unique_ptr. This lets us continue to manipulate it after we have added it to 
		// the source voices collection since we will be using std::move to add it (which will leave sourceVoice as a dead item.
		auto sv = sourceVoice.get();

		// Move the newly created source voice into the source voices collection.
		// Note that we need std::move here since if it was missing we would be trying to create a copy of the newly created unique_ptr
		// and unique_ptr instances cannot have copies of themselves (hence the name).
		soundEffect->m_sourceVoices.push_back(std::move(sourceVoice));
		sourceVoice = nullptr;

		// Set the loop count.
		soundEffect->m_audioBuffer.LoopCount = loopCount;

		// Create the source voice. This creates the IXAudio2SourceVoice object.
		CreateSourceVoice(soundEffect, sv);

		// Start the source voice.
		StartSourceVoice(soundEffect, sv, loopCount);
	}
	else
	{
		// We already have a suitable SourceVoice, so take the iterator we got, dereference it with * (giving us the std::unique_ptr) and
		// use get to get the pointer to the source voice.
		auto sv = (*voice).get();

		// Start the source voice.
		StartSourceVoice(soundEffect, sv, loopCount);
	}
}

void AudioEngine::StopSoundEffect(Platform::String^ filename, bool playTails)
{
	// Ensure that the sound effect exists.
	if (m_soundEffectsMap.find(filename) == m_soundEffectsMap.end())
	{
#if defined(_DEBUG)
		throw ref new Platform::InvalidArgumentException(L"filename");
#endif
		return;
	}

	// Get a reference to the sound effect.
	auto& soundEffect = m_soundEffectsMap[filename];

	UINT32 flags = playTails ? XAUDIO2_PLAY_TAILS : 0U;

	// Stop all source voices.
	for (auto& sv : soundEffect->m_sourceVoices)
	{
		if (sv->m_soundEffectStarted)
		{
			DX::ThrowIfFailed(
				sv->m_soundEffectSourceVoice->Stop(flags), __FILEW__, __LINE__
				);
		}
	}


}

void AudioEngine::ClearUnusedSourceVoices(Platform::String^ filename)
{
	// Ensure that the sound effect exists.
	if (m_soundEffectsMap.find(filename) == m_soundEffectsMap.end())
	{
#if defined(_DEBUG)
		throw ref new Platform::InvalidArgumentException(L"filename");
#endif
		return;
	}

	// Get a reference to the sound effect.
	auto& soundEffect = m_soundEffectsMap[filename];

	// Use std::remove_if to clear out all of the source voices that are not listed as started.
	std::remove_if(soundEffect->m_sourceVoices.rbegin(), soundEffect->m_sourceVoices.rend(), [] (std::unique_ptr<SourceVoice>& voice) { return voice->m_soundEffectStarted == false; });
}

bool AudioEngine::GetNoMediaFoundation()
{
	return m_musicDisabledNoMediaFoundation;
}

bool AudioEngine::GetMusicOff()
{
	return m_musicOff;
}

bool AudioEngine::SetMusicOnOff(bool turnOff)
{
	if (turnOff) // Turn off the music engine.
	{
		ShutdownMusicEngine();
		return m_musicOff;
	}
	else
	{
		if (m_musicOff && !m_musicDisabledNoMediaFoundation)
		{
			// Try to turn on the music engine.
			InitializeMusicEngine();

			// Initialization might not actually turn the music on, so check to make sure it did before proceeding.
			if (!m_musicOff)
			{
				// If the music is supposed to be playing (m_musicIsPlaying) then play it.
				if (m_musicIsPlaying)
				{
					PlayMusic();
				}
			}
		}
		return m_musicOff;
	}
}

bool AudioEngine::GetSoundEffectsOff()
{
	return m_soundEffectsOff;
}

bool AudioEngine::SetSoundEffectsOnOff(bool turnOff)
{
	if (turnOff)
	{
		ShutdownSoundEffectsEngine();
		return m_soundEffectsOff;
	}
	else
	{
		InitializeSoundEffectsEngine();
		return m_soundEffectsOff;
	}
}

double AudioEngine::GetMusicVolume()
{
	return m_musicVolume;
}

void AudioEngine::SetMusicVolume(double volume)
{
	m_musicVolume = volume;

	if (m_musicDisabledNoMediaFoundation || m_musicOff)
	{
		return;
	}

	// We work with volumes from 0.0 to 100.0 externally, but internally those values need to be normalized between 0.0 and 1.0.
	// They could go higher but that would risk distortion.
	m_musicEngine->SetVolume(std::max(0.0, std::min(1.0, volume / 100.0)));
}

double AudioEngine::GetSoundEffectsVolume()
{
	return m_soundEffectsVolume;
}

void AudioEngine::SetSoundEffectsVolume(double volume)
{
	m_soundEffectsVolume = volume;

	if (m_soundEffectsOff)
	{
		return;
	}

	// We work with volumes from 0.0 to 100.0 externally, but internally those values need to be normalized between 0.0f and 1.0f.
	// They could go higher but that would risk distortion.
	DX::ThrowIfFailed(
		m_masteringVoice->SetVolume(static_cast<float>(std::max(0.0, std::min(1.0, volume / 100.0)))), __FILEW__, __LINE__
		);
}

double AudioEngine::PauseMusic()
{
	if (m_musicIsPaused)
	{
		return m_musicPosition;
	}

	m_musicIsPaused = true;

	if (m_musicOff || m_musicDisabledNoMediaFoundation)
	{
		// We're returning the current position of the music. If music is off, we return -1.0. to indicate that we do not need
		// to seek.
		m_musicPosition = -1.0;
		return m_musicPosition;
	}

	// Get the current position of the music.
	m_musicPosition = m_musicEngine->GetCurrentTime();

	// Pause the music.
	DX::ThrowIfFailed(
		m_musicEngine->Pause(), __FILEW__, __LINE__
		);

	// Return the position so that we can resume at that position if we want.
	return m_musicPosition;
}

void AudioEngine::ResumeMusic()
{
	// Resume at the last recorded music position.
	ResumeMusicAtTime(m_musicPosition);
}

void AudioEngine::ResumeMusicAtTime(double seconds)
{
	m_musicIsPaused = false;

	if (m_musicDisabledNoMediaFoundation || m_musicOff)
	{
		return;
	}

	// Make sure we don't accidentally clear out the last piece.
	m_mediaEngineNotify->m_previousMusicFinished = false;

#if defined(_DEBUG)
	OutputDebugStringW(std::wstring(L"Resuming music at time ").append(std::to_wstring(seconds)).append(L" seconds.\n").c_str());
#endif

	// Set the resume position for the music.
	m_musicPosition = seconds;

	// Play the music.
	PlayMusic();
}

double AudioEngine::GetMusicCurrentTime()
{
	if (GetSkipMusicFunction())
	{
		if (!m_musicOff && !m_musicDisabledNoMediaFoundation && m_musicEngine != nullptr)
		{
			// Try to return the time if music is not playing but music can be played.
			return m_musicEngine->GetCurrentTime();
		}
		return 0.0;
	}

	return m_musicEngine->GetCurrentTime();
}

void AudioEngine::SetMusicCurrentTime(double seconds)
{
	if (GetSkipMusicFunction())
	{
		return;
	}

	if (!m_musicOff && !m_musicDisabledNoMediaFoundation && m_musicIsPlaying && m_musicEngine != nullptr)
	{
		// Try to set the time if music is playing.
		m_musicEngine->SetCurrentTime(seconds);
	}

	// Store the specified time.
	m_musicPosition = seconds;
}

void AudioEngine::PauseSoundEffects()
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Loop through all of the sound effects.
	for (auto& item : m_soundEffectsMap)
	{
		// Get a reference to the current sound effect.
		auto& soundEffect = item.second;

		// Loop through all of the source voices and stop all of the started source voices.
		for (auto& voice : soundEffect->m_sourceVoices)
		{
			if (voice->m_soundEffectStarted)
			{
#if defined(_DEBUG)
				DX::ThrowIfFailed(
					voice->m_soundEffectSourceVoice->Stop(), __FILEW__, __LINE__
					);
#else
				HRESULT hr = voice->m_soundEffectSourceVoice->Stop();
				if (FAILED(hr))
				{
					// Log the error.
				}
#endif
			}
		}
	}
}

void AudioEngine::ResumeSoundEffects()
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Loop through all of the sound effects.
	for (auto& item : m_soundEffectsMap)
	{
		// Get a reference to the current sound effect.
		auto& soundEffect = item.second;

		// Loop through all of the source voices and restart any that are listed as started.
		for (auto& voice : soundEffect->m_sourceVoices)
		{
			if (voice->m_soundEffectStarted)
			{
#if defined(_DEBUG)
				DX::ThrowIfFailed(
					voice->m_soundEffectSourceVoice->Start(), __FILEW__, __LINE__
					);
#else
				HRESULT hr = voice->m_soundEffectSourceVoice->Start();
				if (FAILED(hr))
				{
					// Log the error.
				}
#endif
			}
		}
	}
}

void AudioEngine::CreateSourceVoice(std::unique_ptr<SoundEffect>& soundEffect, SourceVoice* sv)
{
	// Set the source voices error tracking variables to their default values.
	sv->m_criticalError = false;
	sv->m_hresult = S_OK;

	// Set started to false.
	sv->m_soundEffectStarted = false;

	// Create an IXAudio2SourceVoice using the sound effect's data. We use the default values for flags and maxFrequency so
	// that we can set our source voice to be the IXAudio2VoiceCallback. So 0U and 2.0f aren't magic numbers, they're just
	// the defaults that XAudio2 would give us anyway if we didn't need to set an IXAudio2VoiceCallback. You could change
	// them, though there are unlikely to be any flags you would want.
	DX::ThrowIfFailed(
		m_soundEffectsEngine->CreateSourceVoice(&sv->m_soundEffectSourceVoice, &soundEffect->m_waveFormatEx, 0U, 2.0f, sv), __FILEW__, __LINE__
		);
}

void AudioEngine::StartSourceVoice(std::unique_ptr<SoundEffect>& soundEffect, SourceVoice* sv, uint32 loopCount)
{
	// Reset the error values to their defaults.
	sv->m_criticalError = false;
	sv->m_hresult = S_OK;

	// Set the loop count that lets us restart a sound effect with the proper number of remaining loops in case it needs recreating.
	sv->m_soundEffectLoopCount = loopCount;

	// Set the loop count for the audio buffer.
	soundEffect->m_audioBuffer.LoopCount = sv->m_soundEffectLoopCount;

	// Submit the data buffer for the source voice so that there's something to play.
	DX::ThrowIfFailed(
		sv->m_soundEffectSourceVoice->SubmitSourceBuffer(&soundEffect->m_audioBuffer), __FILEW__, __LINE__
		);

	// Start the source voice.
	DX::ThrowIfFailed(
		sv->m_soundEffectSourceVoice->Start(), __FILEW__, __LINE__
		);

	// Mark the source voice as started. Note that this is not thread safe since we check this value to see if a source voice is
	// in use. As such, each sound effect should be used synchronously from the same thread.
	sv->m_soundEffectStarted = true;
}

void AudioEngine::RestartFailedSoundEffects()
{
	if (m_soundEffectsOff)
	{
		return;
	}

	// Loop through all the sound effects.
	for (auto& item : m_soundEffectsMap)
	{
		// Get a reference to the current sound effect.
		auto& soundEffect = item.second;

		// If none of the voices in this sound effect had a critical error, continue to the next sound effect.
		if (std::count_if(soundEffect->m_sourceVoices.cbegin(), soundEffect->m_sourceVoices.cend(), [](const std::unique_ptr<SourceVoice>& voice) { return voice->m_criticalError; }) <= 0)
		{
			continue;
		}

		// Loop through all of the source voices for the current sound effect.
		for (auto& sv : soundEffect->m_sourceVoices)
		{
			if (sv->m_criticalError)
			{
				// Store the HRESULT so we can clear it before we try to restart the voice.
				HRESULT hr = sv->m_hresult;

#if defined(_DEBUG)
				// Write out a debug message.
				std::wstringstream str;

				str << L"Failure with instance of sound effect '" <<
					item.first->Data() <<
					std::hex <<
					std::uppercase <<
					L"'. HRESULT = 0x" <<
					static_cast<unsigned long>(hr) <<
					L".\n";

				OutputDebugStringW(str.str().c_str());
#endif
				// Reset the error tracking variables to their default values.
				sv->m_criticalError = false;
				sv->m_hresult = S_OK;

				// Only try restarting for HRESULTs that we comprehend. Otherwise we might
				// just continue to create errors.
				if (hr == XAUDIO2_E_INVALID_CALL ||
					hr == XAUDIO2_E_DEVICE_INVALIDATED)
				{
					// Try to create a new voice based on the error voice.
					CreateSourceVoice(soundEffect, sv.get());
					StartSourceVoice(soundEffect, sv.get(), sv.get()->m_soundEffectLoopCount);
				}
				else
				{
					sv->m_soundEffectStarted = false;
				}
			}
		}
	}
}

// This function does the following:
// if (m_musicOff || m_musicDisabledNoMediaFoundation || !m_musicIsPlaying)
//     return true;
// return false;
bool AudioEngine::GetSkipMusicFunction()
{
	if (m_musicOff || m_musicDisabledNoMediaFoundation || !m_musicIsPlaying)
	{
		return true;
	}

	return false;
}
