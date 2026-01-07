#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Matrix.h"
#include "Timer.h"

namespace dae
{
	struct Camera final
	{
		Camera() = default;
		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{}

		Vector3 origin{};
		float fovAngle{45.f};
		float aspect{1.0f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float zNear{.1f};
		float zFar{100.f};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix(bool useSoftwareRasterizer)
		{
			//forward = {0, 0, 1};
			//pitchYawToVec(-totalYaw, -totalPitch, forward.x, forward.y, forward.z);
			//Matrix rotation{ Matrix::CreateLookAtLH({0, 0, 0}, forward, {0, 1, 0}) };
			Matrix rotation = Matrix::CreateRotation((useSoftwareRasterizer ? 1.0f : -1.0f) * totalPitch, totalYaw, 0.0f);
			forward = rotation.TransformVector({ 0, 0, 1 });
			right = Vector3::Cross(forward, { 0, 1, 0 }).Normalized();

			Vector3 flipY = origin;
			if (useSoftwareRasterizer)
				flipY.y *= -1.0f;

			viewMatrix = (rotation * Matrix::CreateTranslation(flipY)).Inverse(); //*Matrix::CreateScale(1.0f, -1.0f, 1.0f);
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fovAngle, aspect, zNear, zFar);
		}

		void pitchYawToVec(float pitch, float yaw, float &x, float &y, float &z) {
			const float hpi = M_PI / 2.0f;

			x = cosf(pitch + hpi) * cosf(yaw);
			y = sinf(yaw);
			z = sinf(pitch + hpi) * cosf(yaw);
		}

		void Update(const Timer* pTimer, bool isLeftMouseButtonDown, bool isRightMouseButtonDown, bool useSoftwareRasterizer)
		{
			const float deltaTime = pTimer->GetElapsed();

			// Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			const float BOOST{ pKeyboardState[SDL_SCANCODE_LSHIFT] ? 2.0f : 1.0f };

			 //Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const float sensitivity{1.f / 200.0f};

			if (isLeftMouseButtonDown && isRightMouseButtonDown) {
				const float input_Y = float(mouseY);

				// Move (world) Up/Down (LMB + RMB + Mouse Move Y)
				origin += (up * input_Y) * pTimer->GetElapsed() * 50.0f * BOOST;
			}
			else if (isLeftMouseButtonDown) {
				const float input_Y = float(mouseY);

				// Rotate Yaw (LMB + Mouse Move X)
				totalYaw += float(mouseX) * sensitivity;

				// Move (local) Forward/Backward (LMB + Mouse Move Y)
				origin += (forward * -input_Y) * pTimer->GetElapsed() * 50.0f * BOOST;
			}
			else if (isRightMouseButtonDown) {
				// Rotate Yaw (RMB + Mouse Move X)
				// Rotate Pitch (RMB + Mouse Move Y)
				totalYaw += float(mouseX) * sensitivity;
				totalPitch += float(mouseY) * sensitivity;

				totalYaw = fmodf(totalYaw, 360.f);
				totalPitch = fminf(fmaxf(totalPitch, -80.0f), 80.0f);

				forward = {0, 0, 1};
				pitchYawToVec(-totalYaw, -totalPitch, forward.x, forward.y, forward.z);
				Matrix rotation {Matrix::CreateLookAtLH({0, 0, 0}, forward, {0, 1, 0})};
				right = Vector3::Cross({ 0, 1, 0 }, forward);

				/**
				- Move (local) Forward (Arrow Up) and (‘W’)
				- (Camera) Move (local) Backward (Arrow Down) and (‘S’)
				- (Camera) Move (local) Right (Arrow Right) and (‘D’)
				- (Camera) Move (local) Left (Arrow Left) and (‘A’)
				 */
				const float VERTICAL{(pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP] ? 1.0f : 0.0f) + (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN] ? -1.0f : 0.0f)};
				const float HORIZONTAL{(pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT] ? 1.0f : 0.0f) + (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT] ? -1.0f : 0.0f)};
				const float UP_DOWN{(pKeyboardState[SDL_SCANCODE_SPACE] ? 1.0f : 0.0f) + (pKeyboardState[SDL_SCANCODE_C] ? -1.0f : 0.0f)};

				origin += (forward * VERTICAL + up * UP_DOWN + right * HORIZONTAL) * pTimer->GetElapsed() * 50.0f * BOOST;
			}

			CalculateViewMatrix(useSoftwareRasterizer);
			CalculateProjectionMatrix();
		}
	};
}
