#include "pch.h"
#include "Camera.h"

using namespace DirectX;

Camera::Camera() :
	m_projectionLH(),
	m_projectionRH(),
	m_viewLH(),
	m_viewRH()
{
}

void Camera::SetViewParameters(
	_In_ DirectX::XMFLOAT3 cameraPosition,
	_In_ DirectX::XMFLOAT3 lookAtPosition,
	_In_ DirectX::XMFLOAT3 upDirection
	)
{
	XMStoreFloat4x4(&m_viewLH,
		XMMatrixLookAtLH(
		XMLoadFloat3(&cameraPosition),
		XMLoadFloat3(&lookAtPosition),
		XMLoadFloat3(&upDirection)
		)
		);

	XMStoreFloat4x4(&m_viewRH,
		XMMatrixLookAtRH(
		XMLoadFloat3(&cameraPosition),
		XMLoadFloat3(&lookAtPosition),
		XMLoadFloat3(&upDirection)
		)
		);
}

void Camera::SetProjectionParameters(
	_In_ float fieldOfView, // In radians
	_In_ float aspectRatio,
	_In_ float nearClip,
	_In_ float farClip
	)
{
	XMStoreFloat4x4(&m_projectionLH,
		XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearClip, farClip)
		);

	XMStoreFloat4x4(&m_projectionRH,
		XMMatrixPerspectiveFovRH(fieldOfView, aspectRatio, nearClip, farClip)
		);
}
