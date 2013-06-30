#pragma once
#include <sal.h>

ref class Game;

// An interface representing a game component that contains game resources. These could be non-graphics related (e.g. audio resources), graphics related but not dependent on the
// size of the game window (e.g. a fixed size render target, a shader, a texture, a 3D model), or graphics related and dependent on the size of the game window (typically
// window-sized render targets or render targets that derive their size based on window size). There is a member function for each type and while each must be implemented, it
// does not need to do anything.
// Example:
// Windows::Foundation::IAsyncActionWithProgress<int>^ SomeClass::CreateDeviceIndependentResources(
//     _In_ Game^ game
//     )
// {
//     return concurrency::create_async([](concurrency::progress_reporter<int> progressReporter, concurrency::cancellation_token cancellationToken)
//     {
//         // Do nothing.
//     });
// }
// Note:
// The Windows Runtime IAsyncActionWithProgress<int>^ task type was chosen both to simplify task cancellation and to simplify progress reporting. The BloomComponent class in this
// sample illustrates how to report progress but does not do so in an intelligent way. Since a common use of progress reporting would be to allow you to update a loading
// screen, a good way to do this would be to create an enum that represents the various progress states you wish to report, cast it to an int so you can return it, then in the
// function you specify as a handler via the Windows::Foundation::IAsyncActionWithProgress<T>::Progress property, you would cast the int back to the appropriate enum type and
// use its value to update the loading screen appropriately. Or you could simply use an incrementing int value as an indefinite progress updater or (if you were extremely careful)
// a definite progress updater. (You would need to make sure you knew exactly what the final return value was for a definite update and that the value was always the same regardless 
// of which code path you followed in loading if there were any branching statements).
class IGameResourcesComponent
{
public:
	// Empty constructor.
	IGameResourcesComponent() { }

	// Empty virtual destructor.
	virtual ~IGameResourcesComponent() { }

	// Creates resources that do not depend on the D3D device.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateDeviceIndependentResources(
		_In_ Game^ game
		) = 0;

	// Creates D3D device resources that do not depend on window size.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateDeviceResources(
		_In_ Game^ game
		) = 0;

	// Creates D3D device resources that depend on the window size.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateWindowSizeDependentResources(
		_In_ Game^ game
		) = 0;
};
