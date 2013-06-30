#pragma once

#include <DirectXMath.h>

// A simple 3D camera. Allows you to use both left-handed and right-handed coordinates so that you can use whichever is appropriate for your data.
class Camera
{
public:
	// Constructor.
	Camera();

	// Creates a view matrix using DirectX::XMMatrixLookAtLH and DirectX::XMMatrixLookAtRH.
	// cameraPosition - The position, in world coordinates, of the camera.
	// lookAtPosition - The target position, in world coordinates, that the camera is looking at.
	// upDirection - The unit vector which denotes the "up" direction in world coordinates. Typically (0,1,0) but it should properly be at a 90 degree angle from the line formed by cameraPosition and lookAtPosition.
	void SetViewParameters(
		_In_ DirectX::XMFLOAT3 cameraPosition,
		_In_ DirectX::XMFLOAT3 lookAtPosition,
		_In_ DirectX::XMFLOAT3 upDirection
		);

	// Creates a projection matrix using DirectX::XMMatrixPerspectiveFovLH and DirectX::XMMatrixPerspectiveFovRH.
	// fieldOfView - The field of view for the camera, in radians. Typically DirectX::XM_PIDIV4.
	// aspectRatio - The aspect ratio. Typically the width / height of the render target you will be drawing to.
	// nearClip - The near clipping plane distance from the camera. Typically something between 0.01f and 1.0f, though it will depend on the units you are working in.
	// farClip - The far clipping plane distance from the camera. Typically something between 1000f and 10000f, though it will depend on the units you are working in.
	void SetProjectionParameters(
		_In_ float fieldOfView,
		_In_ float aspectRatio,
		_In_ float nearClip,
		_In_ float farClip
		);

	// Returns a const reference to the XMFLOAT4X4 representation of the right-handed projection matrix. Typically you would do something like XMLoadFloat4x4(&camera.GetProjectionMatrix()) to get an XMMATRIX you can pass for drawing.
	const DirectX::XMFLOAT4X4& GetProjectionMatrixLH() { return m_projectionLH; }
	// Returns a const reference to the XMFLOAT4X4 representation of the right-handed view matrix. Typically you would do something like XMLoadFloat4x4(&camera.GetViewMatrix()) to get an XMMATRIX you can pass for drawing.
	const DirectX::XMFLOAT4X4& GetViewMatrixLH() { return m_viewLH; }

	// Returns a const reference to the XMFLOAT4X4 representation of the right-handed projection matrix. Typically you would do something like XMLoadFloat4x4(&camera.GetProjectionMatrix()) to get an XMMATRIX you can pass for drawing.
	const DirectX::XMFLOAT4X4& GetProjectionMatrixRH() { return m_projectionRH; }
	// Returns a const reference to the XMFLOAT4X4 representation of the right-handed view matrix. Typically you would do something like XMLoadFloat4x4(&camera.GetViewMatrix()) to get an XMMATRIX you can pass for drawing.
	const DirectX::XMFLOAT4X4& GetViewMatrixRH() { return m_viewRH; }

private:
	// The left-handed projection matrix, stored as an XMFLOAT4X4 since storing XMMATRIX instances requires 16 byte alignment which can make the code difficult to understand and use properly.
	DirectX::XMFLOAT4X4			m_projectionLH;

	// The right-handed projection matrix, stored as an XMFLOAT4X4 since storing XMMATRIX instances requires 16 byte alignment which can make the code difficult to understand and use properly.
	DirectX::XMFLOAT4X4			m_projectionRH;

	// The left-handed view matrix, stored as an XMFLOAT4X4 since storing XMMATRIX instances requires 16 byte alignment which can make the code difficult to understand and use properly.
	DirectX::XMFLOAT4X4			m_viewLH;

	// The right-handed view matrix, stored as an XMFLOAT4X4 since storing XMMATRIX instances requires 16 byte alignment which can make the code difficult to understand and use properly.
	DirectX::XMFLOAT4X4			m_viewRH;
};

