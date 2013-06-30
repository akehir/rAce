#pragma once
#include <sal.h>

ref class Game;

// An interface representing a game component that should be updated (excluding any updating that would take place during a render operation). Would commonly be used for something such as
// a physics engine, an audio processing component, UI processing or something else that needs to perform calculations at regular periods (once a frame or less) which are not directly related 
// to rendering graphics (e.g. not updating a cbuffer). Update components are updated in FIFO order (unless you add member functions to explicitly control the insertion point of a component).
// Note:
// There is no bright line test for what should fall under update vs. what should fall under render. One person might think that updating to the correct frame in a sprite animation should be 
// in update and another might think it should be in render. Both could be right (though you should decide one way or the other and stick with it) since that question does involve updating 
// graphics (thus giving weight to deciding that it fits under render) but does not update any GPU resources (thus giving weight to deciding that it fits under update). Either way, anything 
// definitely not graphics (e.g. audio and input processing) should definitely fall under update.
class IGameUpdateComponent
{
public:
	// Empty constructor.
	IGameUpdateComponent() { }

	// Empty virtual destructor.
	virtual ~IGameUpdateComponent() { }

	// Update the game component.
	// timeTotal - The duration in seconds between the last time the game timer was reset and the last time it was updated. In practice this should represent the total amount of time the game has been running.
	// timeDelta - The duration in seconds between the last two times the timer was updated. In practice this should represent the elapsed time between frames.
	virtual void Update(
		_In_ Game^ game,
		_In_ float timeTotal,
		_In_ float timeDelta
		) = 0;
};
