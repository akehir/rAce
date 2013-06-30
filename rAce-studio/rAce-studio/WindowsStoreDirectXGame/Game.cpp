#include "pch.h"
#include "Game.h"

using namespace concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace WindowsStoreDirectXGame;

Game::Game() :
	m_gameState(),
	m_audioEngine(ref new AudioEngine()),
	m_basicLoader(),
	m_backgroundColor(DirectX::Colors::CornflowerBlue),
	m_cancellationTokenSource(),
	m_lastPointIsValid(),
	m_lastPointPointerId(),
	m_lastPoint(),
	m_pointerDelta(),
	m_gameResourcesComponents(),
	m_gameUpdateComponents(),
	m_gameRenderComponents()
{
}

void Game::CreateDeviceIndependentResources()
{
	// Indicate that we have not finished loading resources.
	m_deviceIndependentResourcesLoaded = false;

	create_task([this]()
	{
		// Initialize the music engine.
		m_audioEngine->InitializeMusicEngine();

		// Initialize the sound effects engine.
		m_audioEngine->InitializeSoundEffectsEngine();

		// Load the volume test sound that is used by the settings.
		m_audioEngine->LoadSoundEffect("volume_test.wav");

		// Enable retrieval of data from XInput-compatible devices (e.g. an Xbox 360 controller).
		XInputEnable(TRUE);
	}, m_cancellationTokenSource.get_token()).then([this]()
	{
		std::vector<IAsyncActionWithProgress<int>^> asyncActions;
		for (auto item : m_gameResourcesComponents)
		{
			asyncActions.push_back(item->CreateDeviceIndependentResources(this));
		}

		return asyncActions;
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_current()).then([this](std::vector<IAsyncActionWithProgress<int>^> asyncActions)
	{
		// Wait for the async resource loading by the game components to complete.
#if defined(_DEBUG)
		assert(IsBackgroundThread());
#endif
		bool finished;

		while (true)
		{
			// Wait 10 ms to give the tasks some time to complete.
			concurrency::wait(10);

			// Mark finished true. If we ever hit an item that's not finished, we set it to false. If it's still true at the end of looping through all the tasks,
			// then we know they are all complete. 
			finished = true;
			for (auto item : asyncActions)
			{
				// Check to see if the item's status indicates that it is still running, if so we aren't done.
				if (item->Status == Windows::Foundation::AsyncStatus::Started)
				{
					finished = false;
					break;
				}
			}

			if (finished)
			{
				break;
			}
		}

		// Make sure that none of the tasks errored out.
		for (auto item : asyncActions)
		{
			if (item->Status == Windows::Foundation::AsyncStatus::Error)
			{
				DX::ThrowIfFailed(E_FAIL, __FILEW__, __LINE__);
			}
		}

		// Indicate that loading is finished.
		m_deviceIndependentResourcesLoaded = true;

		// While we need to run the IGameResourcesComponent::CreateDeviceIndependentResources member functions for each of the game components on the main thread (in
		// case they need to run something that must run on the main thread), we don't need to sit around waiting for them to complete on the main thread; that can be
		// done just fine on a background thread, which is what we do with task_continuation_context::use_arbitrary.
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_arbitrary());
}


void Game::CreateDeviceResources()
{
	// Indicate that we have not finished loading resources.
	m_deviceResourcesLoaded = false;

	// Call the base class CreateDeviceResources in order to create the graphics device, device context, sprite batch, and common states.
	DirectXBase::CreateDeviceResources();

	// Determine which (if any) multisample settings are supported by the graphics adapter.
	DX::MultisampleCountQualityVector msSettings = DX::GetSupportedMultisampleSettings(m_device.Get(), DXGI_FORMAT_B8G8R8A8_UNORM);

	uint32 msCount = 1;
	uint32 msQuality = 0;

	// If any multisample settings are supported, set it up to use the best settings possible.
	if (msSettings.size() > 0)
	{
		msCount = msSettings.back().first;
		msQuality = msSettings.back().second;
	}

#if defined(_DEBUG)
	//// Uncomment the following line if you want to see what the quality settings are.
	//OutputDebugStringW(std::wstring(L"MS Count: ").append(std::to_wstring(msCount)).append(L". MS Quality: ").append(std::to_wstring(msQuality)).append(L".\n").c_str());
#endif

	// Set the fixed back buffer parameters. This lets us draw our game at a consistent size and have scaling and letterboxing be handled automatically for us.
	SetFixedBackBufferParameters(1366, 768, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, true, msCount, msQuality);

	// Create the fixed back buffer.
	CreateFixedBackBuffer();

	// Add code to create device dependent objects here.

	// Create a new BasicLoader instance. It's not used in this sample but could be useful to you.
	m_basicLoader = ref new BasicLoader(m_device.Get());

	// We do async loading to avoid hanging the UI thread. We use the concurrency Runtime (ConcRT) and Parallel Patterns Library's PPLTasks.h functionality. In particular,
	// we make use of the concurrency::task<T> class. The call to concurrency::when_all on the allTasks vector at the end of this function ensures that we do not need to
	// manually insert wait() calls on all the async tasks.
	std::vector<task<void>> allTasks;

#if defined(_DEBUG)
	//// This task can be used to test whether your code it behaving properly in terms of not blocking the UI thread, properly cancelling if the program is suspended, etc.
	//// This should be commented out or removed in your final game build. For safety, we're including it only in Debug configuration since the Windows Store will reject
	//// games and apps that are not built in Release configuration.
	//allTasks.push_back(create_task([this]()
	//{
	//	assert(IsBackgroundThread());
	//	// Wait 60 ms each loop.
	//	uint32 waitTimeInMilliseconds = 60;

	//	// Loop 100 times, so 100 * 60 = 6000 ms which should guarantee a failed WACK run if this was running in the main thread.
	//	for (int i = 0; i < 100; ++i)
	//	{
	//		// This will throw concurrency::invalid_operation if called on the UI thread.
	//		wait(waitTimeInMilliseconds);

	//		if (is_task_cancellation_requested())
	//		{
	//			// This exits from this task and would also cancel any continuations from this task (i.e. any .then(...) calls).
	//			cancel_current_task();
	//		}
	//	}
	//}, m_cancellationTokenSource.get_token()));
#endif

	// This when_all call lets our tasks run asynchronously and only runs its .then(...) lambda when all of the tasks have completed.
	concurrency::when_all(std::begin(allTasks), std::end(allTasks), m_cancellationTokenSource.get_token()).then([this]()
	{
#if defined(_DEBUG)
		assert(IsMainThread());
#endif

		// Cancel loading if requested.
		if (concurrency::is_task_cancellation_requested())
		{
			concurrency::cancel_current_task();
		}

		// The call to when_all started on the main game thread. By using task_continuation_context::use_current() we tell ConcRT to stay on the same thread,
		// which we want to do to avoid accessing the ID3D11DeviceContext from different threads and causing errors since the context is not free-threaded.
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_current()).then([this]()
	{
		std::vector<IAsyncActionWithProgress<int>^> asyncActions;

		for (auto item : m_gameResourcesComponents)
		{
			asyncActions.push_back(item->CreateDeviceResources(this));
		}

		return std::move(asyncActions);
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_current()).then([this](std::vector<IAsyncActionWithProgress<int>^> asyncActions)
	{
		// Wait for the async resource loading by the game components to complete.
#if defined(_DEBUG)
		assert(IsBackgroundThread());
#endif
		bool finished;

		while (true)
		{
			// Wait 10 ms to give the tasks some time to complete.
			concurrency::wait(10);

			// Mark finished true. If we ever hit an item that's not finished, we set it to false. If it's still true at the end of looping through all the tasks,
			// then we know they are all complete. 
			finished = true;
			for (auto item : asyncActions)
			{
				// Check to see if the item's status indicates that it is still running, if so we aren't done.
				if (item->Status == Windows::Foundation::AsyncStatus::Started)
				{
					finished = false;
					break;
				}
			}

			if (finished)
			{
				break;
			}
		}

		// Make sure that none of the tasks errored out.
		for (auto item : asyncActions)
		{
			if (item->Status == Windows::Foundation::AsyncStatus::Error)
			{
				DX::ThrowIfFailed(E_FAIL, __FILEW__, __LINE__);
			}
		}

		// Indicate that loading is finished.
		m_deviceResourcesLoaded = true;

		// While we need to run the IGameResourcesComponent::CreateDeviceResources member functions for each of the game components on the main thread (in case they need
		// access to the immediate context), we don't need to sit around waiting for them to complete on the main thread; that can be done just fine on a background thread,
		// which is what we do with task_continuation_context::use_arbitrary.
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_arbitrary());
}

void Game::CreateWindowSizeDependentResources()
{
	m_windowSizeResourcesLoaded = false;

	// A vector of task<void>'s. See CreateDeviceResources for a more thorough description.
	std::vector<task<void>> allTasks;

	// This waits for device resources to finish loading to avoid thread collision for the device context.
	allTasks.push_back(create_task([this]()
	{
#if defined(_DEBUG)
		assert(IsBackgroundThread());
#endif
		while (!m_deviceResourcesLoaded)
		{
			unsigned int milliseconds = 50;
			wait(milliseconds);

			if (is_task_cancellation_requested())
			{
				cancel_current_task();
			}
		}
	}, m_cancellationTokenSource.get_token()));

	when_all(std::begin(allTasks), std::end(allTasks), m_cancellationTokenSource.get_token()).then([this]()
	{
		// Call the base class CreateWindowSizeDependentResources to create the swap chain and its render target view and depth stencil view.
		DirectXBase::CreateWindowSizeDependentResources();

		// Add code to create window size dependent objects here.

	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_current()).then([this]()
	{
		std::vector<IAsyncActionWithProgress<int>^> asyncActions;

		for (auto item : m_gameResourcesComponents)
		{
			asyncActions.push_back(item->CreateWindowSizeDependentResources(this));
		}

		return std::move(asyncActions);
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_current()).then([this](std::vector<IAsyncActionWithProgress<int>^> asyncActions)
	{
		// Wait for the async resource loading by the game components to complete.
#if defined(_DEBUG)
		assert(IsBackgroundThread());
#endif
		bool finished;

		while (true)
		{
			// Wait 10 ms to give the tasks some time to complete.
			concurrency::wait(10);

			// Mark finished true. If we ever hit an item that's not finished, we set it to false. If it's still true at the end of looping through all the tasks,
			// then we know they are all complete. 
			finished = true;
			for (auto item : asyncActions)
			{
				// Check to see if the item's status indicates that it is still running, if so we aren't done.
				if (item->Status == Windows::Foundation::AsyncStatus::Started)
				{
					finished = false;
					break;
				}
			}

			if (finished)
			{
				break;
			}
		}

		// Make sure that none of the tasks errored out.
		for (auto item : asyncActions)
		{
			if (item->Status == Windows::Foundation::AsyncStatus::Error)
			{
				DX::ThrowIfFailed(E_FAIL, __FILEW__, __LINE__);
			}
		}

		// Mark window sized resources as loaded.
		m_windowSizeResourcesLoaded = true;

		// While we need to run the IGameResourcesComponent::CreateDeviceResources member functions for each of the game components on the main thread (in case they need
		// access to the immediate context), we don't need to sit around waiting for them to complete on the main thread; that can be done just fine on a background thread,
		// which is what we do with task_continuation_context::use_arbitrary.
	}, m_cancellationTokenSource.get_token(), concurrency::task_continuation_context::use_arbitrary());
}

void Game::OnWindowActivationChanged()
{
	// We implement this inherited pure virtual function because we have to but don't do anything with it in this sample.
	if (m_windowIsDeactivated)
	{
	}
	else
	{
	}
}

void Game::Update(float timeTotal, float timeDelta)
{
	UNREFERENCED_PARAMETER(timeTotal); // This parameter is unused in the sample. This code just acknowledges this and avoids a warning.
	UNREFERENCED_PARAMETER(timeDelta); // This parameter is unused in the sample. This code just acknowledges this and avoids a warning.

	// If everything is loaded, the window is deactivated, and the game is not paused, pause the game.
	if (m_windowIsDeactivated && !m_gamePaused && m_deviceResourcesLoaded && m_windowSizeResourcesLoaded)
	{
		SetGamePaused(true);
		m_audioEngine->PauseMusic();
		m_audioEngine->PauseSoundEffects();
	}

	// Update the audio engine.
	m_audioEngine->Update();

	// Add code to update time dependent objects here.

	switch (m_gameState)
	{
	case GameState::Startup:
		if (m_deviceIndependentResourcesLoaded && m_deviceResourcesLoaded && m_windowSizeResourcesLoaded)
		{
			m_gameState = GameState::Playing;
		}
		break;
	case GameState::Menus:
		break;
	case GameState::Playing:
		if (!m_gamePaused)
		{
			// The following is a simple example of using XInput.
			XINPUT_STATE xInputState = {};
			DWORD playerIndex = 0;

			// If a particular controller is connected (in this case index 0), the result of XInputGetState will be ERROR_SUCCESS. If not
			// then the result of would ERROR_DEVICE_NOT_CONNECTED and none of the data in XInputState would be meaningful.
			if (XInputGetState(playerIndex, &xInputState) == ERROR_SUCCESS)
			{
				// We're checking the left thumbstick as an example.
				float LX = xInputState.Gamepad.sThumbLX;
				float LY = xInputState.Gamepad.sThumbLY;

				// Determine how far the controller is pushed.
				float magnitude = sqrt(LX*LX + LY*LY);

				// Determine the direction the controller is pushed.
				float normalizedLX = LX / magnitude;
				float normalizedLY = LY / magnitude;

				float normalizedMagnitude = 0;

				// Check if the controller is outside a circular dead zone.
				if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
				{
					// Clip the magnitude at its expected maximum value.
					if (magnitude > 32767) magnitude = 32767;

					// Adjust magnitude relative to the end of the dead zone.
					magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

					// Optionally normalize the magnitude with respect to its expected range giving a magnitude value of 0.0f to 1.0f.
					normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				}
				else // If the controller is in the deadzone zero out the magnitude.
				{
					magnitude = 0.0f;
					normalizedMagnitude = 0.0f;
				}

				// Adjust m_lastPoint based on the XInput left thumbstick movement (if any). Note that the Y axis is inverted for our purposes.
				m_lastPoint.X += static_cast<float>(static_cast<int>(normalizedLX * normalizedMagnitude * 3.0f + (normalizedLX > 0.0f ? 0.5f : -0.5f)));
				m_lastPoint.Y -= static_cast<float>(static_cast<int>(normalizedLY * normalizedMagnitude * 3.0f + (normalizedLY > 0.0f ? 0.5f : -0.5f)));
			}
		}
		break;
	case GameState::CutScene:
		break;
	case GameState::Credits:
		break;
	default:
		break;
	}

	// Update each of the updateable game components.
	for (auto item : m_gameUpdateComponents)
	{
		item->Update(this, timeTotal, timeDelta);
	}
}

void Game::Render(float timeTotal, float timeDelta)
{
	// If we aren't done loading yet, there's no DirectX content to display.
	if (!m_deviceResourcesLoaded || !m_windowSizeResourcesLoaded)
	{
		return;
	}

	UNREFERENCED_PARAMETER(timeTotal); // This parameter is unused in the sample. This code just acknowledges this and avoids a warning.
	UNREFERENCED_PARAMETER(timeDelta); // This parameter is unused in the sample. This code just acknowledges this and avoids a warning.

	// Set the correct back buffer and depth stencil buffer based on whether we're using a fixed size and multisampling.
	SetBackBuffer();

	// Clear the back buffer.
	m_context->ClearRenderTargetView(m_currentRenderTargetView, m_backgroundColor);

	// Clear the depth stencil buffer.
	// Depth is cleared to 1.0f. Depth is an unsigned 24-bit normal (i.e. from 0.0f to 1.0f).
	// Stencil is cleared to 0. Stencil is an unsigned 8-bit integer (i.e. from 0 to 255 or 0x0 to 0xFF).
	m_context->ClearDepthStencilView(m_currentDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Draw stuff.

	// Render each of the renderable game components.
	for (auto item : m_gameRenderComponents)
	{
		item->Render(this, timeTotal, timeDelta);
	}
}


void Game::KeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	if (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		// Ignore input in snapped mode. Note: Your game doesn't have to do this, this is just how the sample is handling snapped mode.
		return;
	}

	//// For sample purposes we use the P key to pause/unpause the game. Note that we ignore an unpause if the window is still listed as deactivated.
	//// That lets the user potentially press the P key in a settings flyout without having it unintentionally unpause the game.
	//if (args->VirtualKey == VirtualKey::P && !args->KeyStatus.WasKeyDown)
	//{
	//	if (!m_gamePaused)
	//	{
	//		SetGamePaused(true);
	//		m_audioEngine->PauseMusic();
	//		m_audioEngine->PauseSoundEffects();
	//	}

	//	// Handle this last since we don't want a key press that unpauses the game to be handled as a game input as well.
	//	else if (!m_windowIsDeactivated && m_gamePaused)
	//	{
	//		SetGamePaused(false);
	//		m_audioEngine->ResumeMusic();
	//		m_audioEngine->ResumeSoundEffects();
	//	}
	//}
}

void Game::KeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	if (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		// Ignore input in snapped mode. Note: Your game doesn't have to do this, this is just how the sample is handling snapped mode.
		return;
	}

	if (!m_gamePaused)
	{
		// Set the appropriate game state based on what key has been released.
	}
}

void Game::PointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	if (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		// Ignore pointer input in snapped mode. Note: Your game doesn't have to do this, this is just how the sample is handling snapped mode.
		return;
	}

	auto currentPoint = args->CurrentPoint;

	//#if defined(_DEBUG)
	//OutputDebugStringW(std::wstring(L"Point id '").append(std::to_wstring(args->CurrentPoint->PointerId)).append(L"' pressed.\n").c_str());
	//#endif

	if (!m_gamePaused && !m_lastPointIsValid)
	{
		// Update pointer position. Remember that pointer represents both touch and (left button down) mouse input.
		m_lastPoint = PointerPositionToFixedPosition(currentPoint->Position);
		m_lastPointIsValid = true;
		m_lastPointPointerId = currentPoint->PointerId;
	}

	// Handle this last since we don't want a pointer press that unpauses the game to be handled as a game input as well.
	if (!m_windowIsDeactivated && m_gamePaused)
	{
		// Set m_lastPointIsValid to false in case it was still true from when the game became paused.
		m_lastPointIsValid = false;
		SetGamePaused(false);
		m_audioEngine->ResumeMusic();
		m_audioEngine->ResumeSoundEffects();
	}
}

void Game::PointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	if (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		// Ignore input in snapped mode. Note: Your game doesn't have to do this, this is just how the sample is handling snapped mode.
		return;
	}

	auto currentPoint = args->CurrentPoint;

	// Restrict movement to a single pointer input by checking the pointer id. On multi-touch displays, different touches will have different ids but
	// each touch will retain its id from the moment it is pressed until the moment it is released.
	if (!m_gamePaused && (currentPoint->PointerId == m_lastPointPointerId))
	{
		// We check both values since touch input can easily generated a slight movement even when you are just tapping to, e.g., unpause the game.
		if (m_lastPointIsValid && currentPoint->IsInContact)
		{
			// Find out how much the pointer moved since the last time PointerMoved executed. This value isn't used in the sample.
			m_pointerDelta = Windows::Foundation::Point(
				currentPoint->Position.X - m_lastPoint.X,
				currentPoint->Position.Y - m_lastPoint.Y
				);

			m_lastPoint = PointerPositionToFixedPosition(currentPoint->Position);
			m_lastPointIsValid = true;
		}
		else
		{
			m_lastPointIsValid = false;
		}
	}
}

void Game::PointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	if (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		// Ignore input in snapped mode. Note: Your game doesn't have to do this, this is just how the sample is handling snapped mode.
		return;
	}

	//#if defined(_DEBUG)
	//OutputDebugStringW(std::wstring(L"Point id '").append(std::to_wstring(args->CurrentPoint->PointerId)).append(L"' released.\n").c_str());
	//#endif

	if (m_lastPointPointerId == args->CurrentPoint->PointerId)
	{
		m_lastPointIsValid = false;
	}
}

// The keys for our save state data are stored in a separate namespace to group them together.
namespace SaveState
{
	static Platform::String^ MusicVolumeKey = "m_musicVolume";
	static Platform::String^ MusicOffKey = "m_musicOff";
	static Platform::String^ SoundEffectsVolumeKey = "m_soundEffectsVolume";
	static Platform::String^ SoundEffectsOffKey = "m_soundEffectsOff";
}

void Game::SaveInternalState()
{
	// For things like save game files, you probably want to use roaming app data. I strongly recommend you
	// read the Guidelines for roaming application data page - http://msdn.microsoft.com/en-us/library/windows/apps/hh465094.aspx -
	// before you do this since you need to be very careful to observe the quota for roaming data otherwise
	// the data will stop roaming (until you reduce the size below the quota). As such, using a binary save
	// format such as you'd get by using DataReader and DataWriter - http://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.streams.datareader.aspx
	// - would be much better than using XML since XML will have a lot of excess bytes. Binary saving should
	// also run faster than XML would, which is an important consideration given that the App Certification
	// Requirement have strict limits on how long your app can continue to run once it has received suspend
	// event.

	// For the audio settings, we use local settings (part of local app data). Roaming wouldn't make sense
	// here since the user would likely want different settings depending on the machine the game is running on.
	auto state = Windows::Storage::ApplicationData::Current->LocalSettings->Values;

	// Insert will insert or replace a key/value pair so there is no reason for us to remove any old keys.
	state->Insert(SaveState::MusicVolumeKey, PropertyValue::CreateDouble(m_audioEngine->GetMusicVolume()));
	state->Insert(SaveState::MusicOffKey, PropertyValue::CreateBoolean(m_audioEngine->GetMusicOff()));

	state->Insert(SaveState::SoundEffectsVolumeKey, PropertyValue::CreateDouble(m_audioEngine->GetSoundEffectsVolume()));
	state->Insert(SaveState::SoundEffectsOffKey, PropertyValue::CreateBoolean(m_audioEngine->GetSoundEffectsOff()));
}

void Game::LoadInternalState()
{
	// See the comments at the beginning of SaveInternalState for information about roaming and local app data.

	auto state = Windows::Storage::ApplicationData::Current->LocalSettings->Values;

	// For each state we want to retrieve, we check to see if it's in the local settings before trying to retrieve it.
	if (state->HasKey(SaveState::MusicVolumeKey))
	{
		// When we retrieve a value with Lookup, it returns a Platform::Object. We can safely cast this to an IPropertyValue
		// and then extract the value with the appropriate accessor member function (e.g. GetDouble).
		m_audioEngine->SetMusicVolume(safe_cast<IPropertyValue^>(state->Lookup(SaveState::MusicVolumeKey))->GetDouble());
	}
	if (state->HasKey(SaveState::MusicOffKey))
	{
		m_audioEngine->SetMusicOnOff(safe_cast<IPropertyValue^>(state->Lookup(SaveState::MusicOffKey))->GetBoolean());
	}

	if (state->HasKey(SaveState::SoundEffectsVolumeKey))
	{
		m_audioEngine->SetSoundEffectsVolume(safe_cast<IPropertyValue^>(state->Lookup(SaveState::SoundEffectsVolumeKey))->GetDouble());
	}
	if (state->HasKey(SaveState::SoundEffectsOffKey))
	{
		m_audioEngine->SetSoundEffectsOnOff(safe_cast<IPropertyValue^>(state->Lookup(SaveState::SoundEffectsOffKey))->GetBoolean());
	}
}

