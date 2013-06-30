#pragma once

#include "DirectXBase.h"
#include "SpriteBatch.h"
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "BasicLoader.h"
#include "AudioEngine.h"
#include "IGameResourcesComponent.h"
#include "IGameUpdateComponent.h"
#include "IGameRenderComponent.h"

// Feel free to change this to suit your game's needs. This is for example purposes only.
enum class GameState
{
	Startup = 0,
	Menus,
	Playing,
	CutScene,
	Credits,
};

/// <summary>
/// The Game class, like the default Game1 class in XNA, is the main class for our game. It derives from DirectXBase, which handles much of the graphics, 
/// and makes use of AudioEngine, which handles playing music and sound effects.
/// The general flow of a game is that execution begins with the Game constructor. All this should do is initialize your class member variables. It shouldn't 
/// perform any long running operations nor run any async code.
/// 
/// From there CreateDeviceIndependentResources, CreateDeviceResources, and then SetDpi (from DirectXBase) which calls CreateWindowSizeDependentResources will 
/// execute in succession. Think of these Create*Resources member function as being like XNA's LoadContent method. In this case responsibility is divided up. 
/// On very rare occasions the graphics device (ID3D11Device) may need to be recreated. If that happens, it doesn't make sense to reload things like music 
/// and sound effect files that don't depend on the graphics device. So you put loading of those sorts of resources into CreateDeviceIndependentResources in
/// order to avoid wasting time needlessly reloading them. Instead, just the resources that depend on the graphics device (those from the other two 
/// Create*Resources member functions) will be recreated. Somewhat more frequently the window size will change. This might be because the user decides to 
/// snap another window next to your game. Or maybe they rotate the screen and you support both a landscape and a portrait method of playing your game. Or 
/// maybe the user docks a laptop, activating a monitor with different dimensions. Lots of things can happen that will trigger a window size change. But
/// it doesn't make sense to reload any resource other than those that are tied directly to the window size (e.g. resizing the swap chain and recreating its 
/// render target view and depth stencil buffer). All other graphics content would still be valid so you can handle a window size change quickly without much 
/// disruption to the player (though pausing is still a good idea).
/// 
/// From there, the main game loop proceeds. First the internal game timer is updated. Then Update is called, followed by Render and DirectXBase's Present.
/// If you are curious, this all happens in DirectXPage::OnRendering. Pointer (which includes both mouse and touch) and keyboard input are event driven. So 
/// we have event handlers for the most common events that would be used. What these handlers should do is to simply record any state change in the 
/// appropriate class member variables so that those changes can be processed the next time that Update runs. XInput is based on a polling model and so we 
/// just poll the XInput device(s) each update (if desired).
/// </summary>
ref class Game sealed : public DirectXBase
{
public:
	// Constructor.
	Game();

	// Creates game resources that are not dependent on the graphics device.
	virtual void CreateDeviceIndependentResources() override;
	// Creates game resources that depend on the graphics device but not on the window size.
	virtual void CreateDeviceResources() override;
	// Creates game resources that depend on both the graphics device and the window size (e.g. render targets, (possibly) fonts).
	virtual void CreateWindowSizeDependentResources() override;
	// Executed when the window becomes activated or deactivated. Deactivation can be a good time to do a quick save if that makes sense for your game.
	virtual void OnWindowActivationChanged() override;

	// Update game objects based on time, input events that have occurred, and game state (e.g. playing the game will be different than being in menus).
	void Update(float timeTotal, float timeDelta);

	// Draw the game scene.
	virtual void Render(float timeTotal, float timeDelta) override;

	// Handler for key down events.
	void KeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	// Handler for key up events.
	void KeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	// Handler for pointer pressed events. Pointer encompassed both mouse and touch.
	void PointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	// Handler for pointer moved events. Pointer encompassed both mouse and touch.
	void PointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	// Handler for pointer released events. Pointer encompassed both mouse and touch.
	void PointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);

	// Save game state. Called in DirectXPage::SaveInternalState which is call by App::OnSuspending in response to the app being suspended.
	void SaveInternalState();
	// Load game state. Called in DirectXPage::LoadInternalState which is called by App::OnLaunched when the game starts up.
	void LoadInternalState();

	// Retrieves the audio engine. Primarily used for by the game settings to update various audio states such as whether a particular subsystem is on/off and its volume.
	WindowsStoreDirectXGame::AudioEngine^ GetAudioEngine() { return m_audioEngine; }

private:
	// A simplistic example game state tracking mechanism. Does not do anything of note in this sample.
	GameState												m_gameState;

	// The audio engine used for playing music and sound effects.
	WindowsStoreDirectXGame::AudioEngine^						m_audioEngine;

	// A loader useful for loading shader and (if you don't want to use Texture2D) textures.
	BasicLoader^											m_basicLoader;

	// Our background color. It is a naked pointer to const float because we want to use colors from the DirectX::Colors namespace (from DirectXColors.h). If you want
	// to use a custom color, create an array of 4 floats (RGBA) and set this to point to that, e.g.:
	// static float someColor[4] = {{0.1f}, {0.2f}, {0.3f}, {1.0f}};
	// m_backgroundColor = someColor;
	const float*											m_backgroundColor;

	// A concurrency runtime (ConcRT) cancellation token source. Used to cancel async ConcRT PPL tasks.
	concurrency::cancellation_token_source					m_cancellationTokenSource;

	// Tracks whether or not the data in m_lastPoint comes from a currently active touch or left mouse button down click/drag.
	bool													m_lastPointIsValid;

	// Records the id of a pointer when contact (touch or LMB down) occurs so we can track just the one touch continuously without being distracted by other touches.
	unsigned int											m_lastPointPointerId;

	// The position of the last recorded pointer position. See m_lastPointIsValid to know whether this is was from an active pointer contact event or not.
	Windows::Foundation::Point								m_lastPoint;

	// The change between the last pointer position and the current pointer position.
	Windows::Foundation::Point								m_pointerDelta;

	// A vector of IGameResourcesComponent pointers. Used to load resources in game components that have resources.
	std::vector<IGameResourcesComponent*>					m_gameResourcesComponents;

	// A vector of IGameUpdateComponent pointers. Used to update updateable game components.
	std::vector<IGameUpdateComponent*>						m_gameUpdateComponents;

	// A vector of IGameRenderComponent pointers. Used to render renderable game components.
	std::vector<IGameRenderComponent*>						m_gameRenderComponents;
};
