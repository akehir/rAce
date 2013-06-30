#pragma once

// An implementation of IMFMediaEngineNotify for tracking IMFMediaEngine events such as when the engine is ready to seek and when it encounters an error.
// For simplicity we use the Microsoft::WRL::RuntimeClass template class to implement all the COM bits for us since IMFMediaEngineNotify implementations
// must be COM objects.
class MediaEngineNotify : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IMFMediaEngineNotify>
{
public:
	// This is called by the Media Foundation runtime to notify us of an event. The meEvent parameter is an MF_MEDIA_ENGINE_EVENT enum member. param1
	// and param2 have different meanings (or no meaning) depending on the value of meEvent.
	STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2);

	// Since we're using Microsoft::WRL::RuntimeClass we can't write our own constructor. As such we have this class member variable initialization
	// member function which should always be called immediately after creating an instance of this class to ensure that the class member 
	// variables have proper initial values.
	void InitializeVariables();

	// If true an error occurred.
	bool					m_errorOccurred;
	// The type of error encountered.
	MF_MEDIA_ENGINE_ERR		m_errorType;
	// The HRESULT of the error.
	HRESULT					m_hresult;
	// If true music is currently playing.
	bool					m_musicIsPlaying;
	// If true we can seek to a specific location in the current music.
	bool					m_readyToSeek;
	// If true, there was music playing and it has finished.
	bool					m_previousMusicFinished;
};

// This is a RAII class for the MFStartup(...) and MFShutdown functions to ensure that they are properly called/not called.
class MediaFoundationStartupShutdown
{
public:
	// Constructor. This does not call MFStartup(...); you must call the Startup member function to do that.
	MediaFoundationStartupShutdown();
	// Destructor. Ensures that MFShutdown is called if necessary.
	~MediaFoundationStartupShutdown();
	// Calls MFStartup(...) iff it has not been successfully called already. Returns true if Media Foundation startup was successful, false if startup failed.
	bool Startup();
	// Explicitly calls MFShutdown iff MFStartup(...) was successfully called via Startup. Provided for error recovery that allows an instance of
	// this class to be a non-pointer member variable of another class.
	void Shutdown();

private:
	// Tracking variable to determine if Media Foundation was successfully started such that it can/should be shutdown.
	bool									m_wasStarted;
	// Slim Reader/Writer lock to avoid any possibility of race conditions or reordering problems.
	std::shared_ptr<SRWLOCK>				m_srwLock;
};

// A template class to add RAII to IXAudio2Voice objects in order to ensure that voices are properly destroyed before going out of scope.
template<class T>
class xaudio2_voice_ptr
{
public:
	// Constructor.
	xaudio2_voice_ptr(T* voice = nullptr) :
		m_voice(voice)
	{
	}

	// Move constructor.
	xaudio2_voice_ptr(xaudio2_voice_ptr&& other) :
		m_voice()
	{
		// Invoke the move assignment operator via std::move rather than replicating the same code twice.
		*this = std::move(other);
	}

	// Move assignment operator.
	xaudio2_voice_ptr& operator=(xaudio2_voice_ptr&& other)
	{
		if (m_voice != nullptr)
		{
			m_voice->DestroyVoice();
			m_voice = nullptr;
		}
		m_voice = other.m_voice;
		other.m_voice = nullptr;
	}

	// Destructor.
	~xaudio2_voice_ptr()
	{
		if (m_voice != nullptr)
		{
			// Note that DestroyVoice waits for the audio processing thread to be idle such that it can take
			// some time to complete (possibly several milliseconds). If any other voice is sending data to
			// this voice, destroy voice will fail silently and this will thus leak memory. Also, DestroyVoice
			// cannot be called from any XAudio2 callback. Voices should thus either be destroyed on a separate
			// thread (so that they will not block the game) by calling Reset or else should be destroyed only
			// when the game can afford the time (e.g. in loading screens or between levels).
			m_voice->DestroyVoice();
			m_voice = nullptr;
		}
	}

	// Give it similar characteristics to ComPtr<T> in terms of operator behavior and member functions.
	T& operator*() { return *m_voice; }
	T* operator->() { return m_voice; }
	T** operator&() { return &m_voice; }
	T* Get() const { return m_voice; };
	T* const* GetAddressOf() const { return &m_voice; }

	// Destroys the voice (if it exists) and then nulls out the pointer to it.
	void Reset()
	{
		if (m_voice != nullptr)
		{
			m_voice->DestroyVoice();
			m_voice = nullptr;
		}
	}

	bool operator==(xaudio2_voice_ptr<T> other) { return m_voice == other.m_voice; }
	bool operator!=(xaudio2_voice_ptr<T> other) { return m_voice != other.m_voice; }

protected:
	// Pointer to the IXAudio2Voice-derived object.
	T*				m_voice;
};

// SourceVoice serves as a self-contained IXAudio2SourceVoice complete with its own IXAudio2VoiceCallback implementation.
struct SourceVoice : public IXAudio2VoiceCallback
{
	// The source voice.
	xaudio2_voice_ptr<IXAudio2SourceVoice>		m_soundEffectSourceVoice;
	// Tracking variable for SourceVoice instance reuse.
	bool										m_soundEffectStarted;
	// Tracking variable for the loop count. Decrements each loop unless set to XAUDIO2_LOOP_INFINITE.
	uint32										m_soundEffectLoopCount;
	// Set to true via the IXAudio2VoiceCallback::OnVoiceError implementation when a critical error was encountered.
	bool										m_criticalError;
	// If m_criticalError is true, contains the HRESULT of the error that was encountered. Default value should be S_OK.
	HRESULT										m_hresult;

	// Called just before this voice's processing pass begins.
	virtual void __declspec(nothrow) __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) override { UNREFERENCED_PARAMETER(BytesRequired); }

	// Called just after this voice's processing pass ends.
	virtual void __declspec(nothrow) __stdcall OnVoiceProcessingPassEnd() override { }

	// Called when this voice has just finished playing a buffer stream
	// (as marked with the XAUDIO2_END_OF_STREAM flag on the last buffer).
	virtual void __declspec(nothrow) __stdcall OnStreamEnd() override
	{
		m_soundEffectStarted = false;
	}

	// Called when this voice is about to start processing a new buffer.
	virtual void __declspec(nothrow) __stdcall OnBufferStart(void* pBufferContext) override { UNREFERENCED_PARAMETER(pBufferContext); }

	// Called when this voice has just finished processing a buffer.
	// The buffer can now be reused or destroyed.
	virtual void __declspec(nothrow) __stdcall OnBufferEnd(void* pBufferContext) override { UNREFERENCED_PARAMETER(pBufferContext); }

	// Called when this voice has just reached the end position of a loop.
	virtual void __declspec(nothrow) __stdcall OnLoopEnd(void* pBufferContext) override
	{
		UNREFERENCED_PARAMETER(pBufferContext);

		if (m_soundEffectLoopCount != XAUDIO2_LOOP_INFINITE)
		{
			--m_soundEffectLoopCount;
		}
	}

	// Called in the event of a critical error during voice processing,
	// such as a failing xAPO or an error from the hardware XMA decoder.
	// The voice may have to be destroyed and re-created to recover from
	// the error.  The callback arguments report which buffer was being
	// processed when the error occurred, and its HRESULT code.
	virtual void __declspec(nothrow) __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) override
	{
		UNREFERENCED_PARAMETER(pBufferContext);

		m_criticalError = true;
		m_hresult = Error;
	}
};

// Holds the data loaded from a sound effect file and the SourceVoice instance(s) created from it.
struct SoundEffect
{
	SoundEffect() :
		m_audioBuffer(),
		m_waveFormatEx(),
		m_sourceVoices(),
		m_soundEffectBufferData(),
		m_soundEffectBufferLength()
	{
	}

	// Stores the data used in calling IXAudio2SourceVoice::SubmitSourceBuffer
	XAUDIO2_BUFFER								m_audioBuffer;
	// Stores the wave format data we need to create a source voice for this sound effect.
	WAVEFORMATEX								m_waveFormatEx;
	// The collection of SourceVoice instances that serves as a pool.
	std::vector<std::unique_ptr<SourceVoice>>	m_sourceVoices;
	// The sound effect data itself.
	std::unique_ptr<uint8>						m_soundEffectBufferData;
	// The length of the sound effect data.
	uint32										m_soundEffectBufferLength;
	// The sample rate of the sound effect data.
	uint32										m_soundEffectSampleRate;

private:
	// Disable copy constructor.
	SoundEffect(const SoundEffect&);
	// Disable copy assignment.
	SoundEffect& operator=(const SoundEffect&);
};

// Implements IXAudio2EngineCallback. This implementation records the HRESULT of any critical error. It
// does nothing else.
class SoundEffectsEngineCallbacks : public IXAudio2EngineCallback
{
public:
	// Constructor.
	SoundEffectsEngineCallbacks() :
		m_error(S_OK)
	{
	}

	// Called by XAudio2 just before an audio processing pass begins.
	virtual void __declspec(nothrow) __stdcall OnProcessingPassStart() override { }

	// Called just after an audio processing pass ends.
	virtual void __declspec(nothrow) __stdcall OnProcessingPassEnd() override { }

	// Called in the event of a critical system error which requires XAudio2
	// to be closed down and restarted.  The error code is given in Error.
	virtual void __declspec(nothrow) __stdcall OnCriticalError(HRESULT Error) override
	{
		m_error = Error;
	}

	// Default value should be S_OK. Reset it to that after processing any error so that
	// you can check for FAILED(...->m_error) to determine if an error occurred.
	HRESULT					m_error;
};

namespace WindowsStoreDirectXGame
{
	[Platform::Metadata::FlagsAttribute()]
	public enum class UpdateErrorCodes : uint32
	{
		None =					0x0,
		MusicEngine =			0x1,
		SoundEffectsEngine =	0x2,
	};

	public value struct MusicQueueEntry sealed
	{
		Platform::String^		Filename;
		uint32					LoopCount;
		bool					AutoPlayAfterPreviousMusic;
	};

	public ref class AudioEngine sealed
	{
	public:
		// Constructor.
		AudioEngine();
		// Destructor.
		virtual ~AudioEngine();

		// Attempts to initialize the music engine (Media Foundation's IMFMediaEngine).
		void InitializeMusicEngine();

		// Shuts down and destroys the music engine.
		void ShutdownMusicEngine();

		// Attempts to initialize the sound effects engine (XAudio2's IXAudio2).
		void InitializeSoundEffectsEngine();

		// Shuts down and destroys the sound effects engine.
		void ShutdownSoundEffectsEngine();

		// Updates the music engine and sound effects engine, checking to see if errors occurred and handling them
		// as possible. Also handles looping of music and proper processing of the music queue.
		UpdateErrorCodes Update();

		// Adds a song to the music queue and sets it to automatically play after the song (if any) that proceeds it
		// in the queue.
		// filename - The relative path and full file name of the song, e.g. "Menu Music.wma" or "somedir\\Some Song.mp3"
		// loopCount - The number of times to loop the song. If infinite looping is desired, use -1 or some other negative number. 0 means play once (i.e. loop zero times).
		void AddMusicToQueue(Platform::String^ filename, int loopCount);
		// Adds a song to the music queue.
		// filename - The relative path and full file name of the song, e.g. "Menu Music.wma" or "somedir\\Some Song.mp3"
		// loopCount - The number of times to loop the song. If infinite looping is desired, use -1 or some other negative number. 0 means play once (i.e. loop zero times).
		// autoPlayAfterPreviousMusic - If true, the music will automatically begin playing after the previous music (if any) completes playing (including any looping so if the previous song is set to loop infinite then you will need to call MoveToNextMusicInQueue to play this song). If false then music will stop when the previous song finishes and this song will be the song that would be played in any subsequent call to PlayMusic.
		void AddMusicToQueue(Platform::String^ filename, int loopCount, bool autoPlayAfterPreviousMusic);

		// Clears all songs from the music queue. This includes the currently playing song (if any).
		void ClearMusicQueue();

		// Moves to the next song in the music queue, ignoring any loop count settings for the currently playing song, and plays the new current song.
		void MoveToNextMusicInQueue();

		// Plays the first song in the music queue, if any.
		void PlayMusic();

		// Plays the volume test sound through the music engine. Used in volume settings.
		void PlayMusicVolumeTestSound();

		// Plays the volume test sound through the sound effects engine. Used in volume settings.
		void PlaySoundEffectsVolumeTestSound();

		// Loads a sound effect file. If the file is already loaded, this function call will be disregarded.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		void LoadSoundEffect(Platform::String^ filename);

		// Loads a sound effect file.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		// forceReload - If true then the file will be reloaded even if it has already been loaded. If false then if the file is already loaded, this function call will be disregarded.
		void LoadSoundEffect(Platform::String^ filename, bool forceReload);

		// USE CAREFULLY. Unloads a sound effect file (if loaded), freeing all resources associated with it. To avoid potentially blocking the game, this should
		// be called from a separate thread that can safely be blocked from continuing to execute for up to several milliseconds (potentially longer). This should
		// also never be called when you might use this sound effect again within the next minute or so since you could wind up colliding the creation and
		// destruction and to unknown (but assuredly bad) results.
		void UnloadSoundEffect(Platform::String^ filename);

		// Plays the specified sound effect with no looping and no limit on maximum concurrent instances of the effect. The sound effect must have
		// been loaded with LoadSoundEffect prior to this call.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		void PlaySoundEffect(Platform::String^ filename);

		// Plays the specified sound effect with no limit on maximum concurrent instances of the effect. The sound effect must have been loaded 
		// with LoadSoundEffect prior to this call.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		// loopCount - The number of times to loop the sound effect. If infinite looping is desired, use XAUDIO2_LOOP_INFINITE. 0 means play once (i.e. loop zero times).
		void PlaySoundEffect(Platform::String^ filename, uint32 loopCount);

		// Plays the specified sound effect. The sound effect must have been loaded with LoadSoundEffect prior to this call.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		// loopCount - The number of times to loop the sound effect. If infinite looping is desired, use XAUDIO2_LOOP_INFINITE. 0 means play once (i.e. loop zero times).
		// maxInstances - The maximum concurrent instances of the effect. If this effect is playing less than maxInstances times this call will play the effect, otherwise this call will be ignored. A value of 0 means no limit on maximum concurrent instances.
		void PlaySoundEffect(Platform::String^ filename, uint32 loopCount, uint32 maxInstances);

		// Stops all instances of the specified sound effect.
		// filename - The relative path and full file name of the sound effect, e.g. "laser.wav" or "somedir\\ball drop.wav"
		// playTails - If true, any tailing effects (such as reverb) will be allowed to play. If false, the effect instances will be stopped instantly. Will not cause further buffering either way.
		void StopSoundEffect(Platform::String^ filename, bool playTails);

		// USE CAREFULLY. Removes unused source voices for the specified sound effect file. To avoid potentially blocking the game, this should be called
		// from a separate thread that can safely be blocked from continuing to execute for up to several milliseconds (potentially longer).
		// This should also never be called when you might use this sound effect again within the next minute or so since you could wind up attempting to 
		// use a voice that is about to be destroyed giving unknown (but assuredly bad) results.
		// filename - The loaded sound effect file for which you want to clear unused source voices.
		void ClearUnusedSourceVoices(Platform::String^ filename);

		// Returns true when media foundation could not be started.
		bool GetNoMediaFoundation();

		// Returns true when music is off, false when it is on. Note that music could be off because the user shut off music in settings or it could be 
		// because the audio engine encountered an error it was not designed to recover from such that it shut off music.
		bool GetMusicOff();

		// Returns false if music is on as a result of the call, true if music is off as a result. Note that if you tried to enable music and it could not
		// be enabled, this function will return true (indicating that music is off). If music is turned on and there is a song that should be playing, it 
		// will begin playing.
		// turnOff - If true, music will be turned off. If false, and attempt will be made to turn music on.
		bool SetMusicOnOff(bool turnOff);

		// Returns true when sound effects are off, false when they are on. Note that sound effects could be off because the user shut off sound effects
		// in settings or it could be because the audio engine encountered an error it was not designed to recover from such that it shut off sound effects.
		bool GetSoundEffectsOff();

		// Returns false if sound effects are on as a result of the call, true if sound effects are off as a result. Note that if you tried to enable sound
		// effects and they could not be enabled, this function will return true (indicating that sound effects are off). Unlike with its music counterpart,
		// this will not start up any sound effects if sound effects are successfully turned on.
		// turnOff - If true, sound effects will be turned off. If false, and attempt will be made to turn sound effects on.
		bool SetSoundEffectsOnOff(bool turnOff); // Returns the result; if music couldn't be enabled, returns true.

		// Returns music volume from a range of 0.0 to 100.0.
		double GetMusicVolume();

		// Sets music volume.
		// volume - Should be a value between 0.0 and 100.0, inclusive. Values will be clamped internally.
		void SetMusicVolume(double volume);

		// Returns sound effects volume from a range of 0.0 to 100.0.
		double GetSoundEffectsVolume();

		// Sets sound effects volume.
		// volume - Should be a value between 0.0 and 100.0, inclusive. Values will be clamped internally.
		void SetSoundEffectsVolume(double volume);

		// Pauses the currently playing music. The return value is the position, in seconds, of the currently playing music. If music is already paused,
		// the return value will be the previously cached position in seconds. If music is off or media foundation is not present, the return value will
		// be -1.0.
		double PauseMusic();

		// Resumes the currently playing music at the previously cached position (i.e. the position from the last call to PauseMusic).
		void ResumeMusic();

		// Resumes the currently playing music at the specified position.
		// seconds - The position in seconds at which to start the music. Pass a negative value to avoid having any seeking to a position occur.
		void ResumeMusicAtTime(double seconds);

		// Returns the position, in seconds, of the currently playing music. If the music engine is off or disabled, this will always return 0.0.
		double GetMusicCurrentTime();

		// Seeks to the specified time if music is currently playing or stores the specified time as the time to seek to when music begins.
		// seconds - The position in seconds to seek to. Pass a negative value to clear out any previously stored seek time.
		void SetMusicCurrentTime(double seconds); // Seeks to the specified time (which is a time in seconds).

		// Pauses all currently playing sound effects.
		void PauseSoundEffects();

		// Resumes all paused sound effects.
		void ResumeSoundEffects();

		// Attempts to restart any sound effects which were playing and which experienced either an XAUDIO2_E_INVALID_CALL error or an
		// XAUDIO2_E_DEVICE_INVALIDATED error.
		void RestartFailedSoundEffects();

	internal:
		// Returns a pointer to the IMFMediaEngineEx object that serves as the music engine. Return value will be null if music is off or
		// Media Foundation is not present on the system.
		IMFMediaEngineEx* MusicEngine() const { return m_musicEngine.Get(); }

		// Returns a pointer to the IXAudio2MasteringVoice that serves as the matering voice for the IXAudio2 object that is the sound
		// effects instance.
		IXAudio2MasteringVoice* SoundEffectsMasteringVoice() const { return m_masteringVoice.Get(); }

		// Returns a pointer to the IXAudio2 object that serves as the sound effects engine. Return value will be null if sound effects
		// are off.
		IXAudio2* SoundEffectsEngine() const { return m_soundEffectsEngine.Get(); }

	private:
		// Disable copying.
		AudioEngine(const AudioEngine%); // % is the ref class reference token (the equivalent of &).
		AudioEngine% operator=(const AudioEngine%);

		// Creates a new source voice for a sound effect.
		void CreateSourceVoice(std::unique_ptr<SoundEffect>& soundEffect, SourceVoice* sv);

		// Starts a source voice for a sound effect.
		void StartSourceVoice(std::unique_ptr<SoundEffect>& soundEffect, SourceVoice* sv, uint32 loopCount);

		// This function does the following:
		// if (m_musicOff || m_musicDisabledNoMediaFoundation || !m_musicIsPlaying)
		//     return true;
		// return false;
		bool GetSkipMusicFunction();

		// An instance of the class that handles callbacks for the sound effects engine. Basically an error-recording implementation of IXAudio2EngineCallback.
		SoundEffectsEngineCallbacks												m_soundEffectsEngineCallbacks;

		// The music engine, which is an IMFMediaEngineEx object.
		Microsoft::WRL::ComPtr<IMFMediaEngineEx>								m_musicEngine;

		// The sound effects engine, which is an IXAudio2 object.
		Microsoft::WRL::ComPtr<IXAudio2>										m_soundEffectsEngine;

		// An instance of the class that handles callbacks for the music engine. Handles things ranging from seeking to errors and more.
		Microsoft::WRL::ComPtr<MediaEngineNotify>								m_mediaEngineNotify;

		// The mastering voice for the sound effects engine. This is what all other voices feed into.
		xaudio2_voice_ptr<IXAudio2MasteringVoice>								m_masteringVoice;

		// The filename-indexed dictionary of sound effects.
		std::map<Platform::String^, std::unique_ptr<SoundEffect>>				m_soundEffectsMap;

		// The song queue for music.
		std::queue<MusicQueueEntry>												m_musicQueue;

		// Ensures that Media Foundation startup and shutdown is properly handled.
		MediaFoundationStartupShutdown											m_mediaFoundationStartupShutdown;

		// Set to true if Media Foundation could not be started (which likely means an N or KN edition of Windows that does not have the media pack installed).
		bool																	m_musicDisabledNoMediaFoundation;

		// Set to true if the user has turned off music in the settings or if the audio engine encountered an unrecoverable error such that it needed to shutdown music.
		bool																	m_musicOff;

		// Set to true if the user has turned off sound effects in the settings or if the audio engine encountered an unrecoverable error such that it needed to shutdown sound effects.
		bool																	m_soundEffectsOff;

		// The music volume, from 0.0 to 100.0 inclusive.
		double																	m_musicVolume;

		// The sound effects volume, from 0.0 to 100.0 inclusive.
		double																	m_soundEffectsVolume;

		// Used in tracking when music is supposed to be playing.
		bool																	m_musicIsPlaying;
		
		// Tracks whether music is currently paused.
		bool																	m_musicIsPaused;

		// Tracks where music should seek to and where music was when it was paused. A negative value means do not seek.
		double																	m_musicPosition;
	};
}
