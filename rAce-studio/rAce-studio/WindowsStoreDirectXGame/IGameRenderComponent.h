#pragma once
#include <sal.h>

ref class Game;

// An interface representing a game component that should be rendered. Render components are updated in FIFO order (unless you add member functions to explicitly control the insertion 
// point of a component).
// Note: 
// An IGameRenderComponent will typically also be an IGameResourcesComponent, but it could make use of resources already existing within the game such that it did not need any of its own
// resources. A simple example would be a component that existed merely to clear a render target to one of several colors depending on the value of some state variable within the game.
class IGameRenderComponent
{
public:
	// Empty constructor.
	IGameRenderComponent() { }

	// Empty virtual destructor.
	virtual ~IGameRenderComponent() { }

	// Render the game component.
	// timeTotal - The duration in seconds between the last time the game timer was reset and the last time it was updated. In practice this should represent the total amount of time the game has been running.
	// timeDelta - The duration in seconds between the last two times the timer was updated. In practice this should represent the elapsed time between frames.
	virtual void Render(
		_In_ Game^ game,
		_In_ float timeTotal,
		_In_ float timeDelta
		) = 0;
};
