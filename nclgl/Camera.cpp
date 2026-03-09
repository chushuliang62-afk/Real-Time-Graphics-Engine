#include "Camera.h"
#include "Mouse.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void Camera::UpdateCamera(float msec)	{
	static bool firstCall = true;
	if (firstCall) {
		std::cout << "Camera::UpdateCamera first call - Mode: " << (autoMode ? "AUTO" : "MANUAL") << std::endl;
		std::cout << "  Waypoints count: " << waypoints.size() << std::endl;
		std::cout << "  Current waypoint index: " << currentWaypoint << std::endl;
		std::cout << "  Input dt (msec): " << msec << std::endl;
		firstCall = false;
	}

	if (autoMode) {
		// IMPORTANT: Despite the parameter name "msec", main.cpp actually passes SECONDS
		// (from w.GetTimer()->GetTimeDeltaSeconds())
		// So we pass it directly to UpdateAutoCamera which expects seconds
		UpdateAutoCamera(msec);  // msec is actually seconds!
	} else {
		UpdateManualCamera(msec);
	}
}

void Camera::SetFOV(float f) {
	// Clamp FOV to reasonable range: 10° (telephoto) to 120° (wide angle)
	fov = std::max(10.0f, std::min(120.0f, f));
}

void Camera::AdjustFOV(float delta) {
	SetFOV(fov + delta);
}

void Camera::UpdateManualCamera(float msec) {
	// Handle mouse scroll wheel for zoom (FOV adjustment)
	float scrollDelta = Window::GetMouse()->GetWheelMovement();
	if (scrollDelta != 0.0f) {
		float oldFOV = fov;
		// Negative scroll = zoom in (decrease FOV), positive = zoom out (increase FOV)
		AdjustFOV(-scrollDelta * 5.0f);  // 5.0 = zoom sensitivity
		std::cout << "FOV adjusted: " << oldFOV << "° -> " << fov << "° (scroll: " << scrollDelta << ")" << std::endl;
	}

	//Update the mouse by how much
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	//Bounds check the pitch, to be between straight up and straight down ;)
	pitch = std::min(pitch,90.0f);
	pitch = std::max(pitch,-90.0f);

	if(yaw <0) {
		yaw+= 360.0f;
	}
	if(yaw > 360.0f) {
		yaw -= 360.0f;
	}

	// IMPORTANT: Despite parameter name "msec", we actually receive SECONDS from main.cpp
	// Convert to milliseconds for original movement speed expectations
	// Then multiply by speed factor for comfortable movement
	float movementSpeed = msec * 1000.0f;  // Convert seconds to milliseconds
	movementSpeed *= 5.0f;  // Base speed multiplier

	if (Window::GetMouse()->ButtonDown(MOUSE_FOUR)) {
		movementSpeed *= 5.0f;  // Turbo speed when holding mouse button 4
	}

	if(Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
		position += Matrix4::Rotation(yaw, Vector3(0,1,0)) * Vector3(0,0,-1) * movementSpeed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
		position -= Matrix4::Rotation(yaw, Vector3(0,1,0)) * Vector3(0,0,-1) * movementSpeed;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
		position += Matrix4::Rotation(yaw, Vector3(0,1,0)) * Vector3(-1,0,0) * movementSpeed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) {
		position -= Matrix4::Rotation(yaw, Vector3(0,1,0)) * Vector3(-1,0,0) * movementSpeed;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) {
		position.y += movementSpeed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
		position.y -= movementSpeed;
	}
}

void Camera::UpdateAutoCamera(float dt) {
	// No waypoints = do nothing
	if (waypoints.empty()) {
		static bool warnedOnce = false;
		if (!warnedOnce) {
			std::cout << "WARNING: UpdateAutoCamera called but waypoints vector is empty!" << std::endl;
			warnedOnce = true;
		}
		return;
	}

	// Ensure currentWaypoint is valid
	if (currentWaypoint >= waypoints.size()) {
		currentWaypoint = 0;
	}

	// Debug output every 60 frames (~1 second)
	static int debugFrameCount = 0;
	debugFrameCount++;
	if (debugFrameCount % 60 == 0) {
		std::cout << "AUTO Camera - WP:" << currentWaypoint << "/" << waypoints.size()
		          << " Timer:" << waypointTimer << "/" << waypoints[currentWaypoint].duration
		          << " Pos:(" << position.x << "," << position.y << "," << position.z << ")"
		          << " dt:" << dt << std::endl;
	}

	// Update timer
	waypointTimer += dt;

	// Get current waypoint
	CameraWaypoint& current = waypoints[currentWaypoint];

	// Check if we should move to next waypoint
	if (waypointTimer >= current.duration) {
		waypointTimer = 0.0f;
		currentWaypoint = (currentWaypoint + 1) % waypoints.size();  // Loop back to start
		std::cout << "Switching to waypoint " << currentWaypoint << std::endl;
	}

	// Calculate interpolation factor (0 to 1)
	float t = waypointTimer / current.duration;

	// Smooth interpolation using smoothstep (ease in/out)
	t = t * t * (3.0f - 2.0f * t);

	// Get next waypoint index (wraps around)
	int nextIndex = (currentWaypoint + 1) % waypoints.size();
	CameraWaypoint& next = waypoints[nextIndex];

	// Interpolate position
	position = InterpolatePosition(t, currentWaypoint);

	// Interpolate angles
	pitch = InterpolateAngle(current.pitch, next.pitch, t);
	yaw = InterpolateAngle(current.yaw, next.yaw, t);
}

/*
Generates a view matrix for the camera's viewpoint. This matrix can be sent
straight to the skeletonShader...it's already an 'inverse camera' matrix.
*/
Matrix4 Camera::BuildViewMatrix()	{
	//Why do a complicated matrix inversion, when we can just generate the matrix
	//using the negative values ;). The matrix multiplication order is important!
	return	Matrix4::Rotation(-pitch, Vector3(1,0,0)) *
			Matrix4::Rotation(-yaw, Vector3(0,1,0)) *
			Matrix4::Translation(-position);
};

// ========================================
// Waypoint System Implementation
// ========================================

void Camera::AddWaypoint(const CameraWaypoint& wp) {
	waypoints.push_back(wp);
	std::cout << "Waypoint " << (waypoints.size() - 1) << " added: Pos("
	          << wp.position.x << ", " << wp.position.y << ", " << wp.position.z
	          << ") Pitch=" << wp.pitch << " Yaw=" << wp.yaw
	          << " Duration=" << wp.duration << "s" << std::endl;
}

void Camera::ClearWaypoints() {
	waypoints.clear();
	currentWaypoint = 0;
	waypointTimer = 0.0f;
}

void Camera::ResetTrack() {
	currentWaypoint = 0;
	waypointTimer = 0.0f;

	// Set camera to first waypoint position
	if (!waypoints.empty()) {
		position = waypoints[0].position;
		pitch = waypoints[0].pitch;
		yaw = waypoints[0].yaw;

		// Debug output
		std::cout << "Camera reset to waypoint 0:" << std::endl;
		std::cout << "  Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
		std::cout << "  Pitch: " << pitch << ", Yaw: " << yaw << std::endl;
		std::cout << "  Total waypoints: " << waypoints.size() << std::endl;
	}
}

void Camera::ToggleMode() {
	autoMode = !autoMode;
	std::cout << "Camera mode toggled to: " << (autoMode ? "AUTO" : "MANUAL") << std::endl;
	std::cout << "  Waypoints loaded: " << waypoints.size() << std::endl;
	std::cout << "  Current position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

// Interpolate position between current and next waypoint
Vector3 Camera::InterpolatePosition(float t, int waypointIndex) {
	if (waypoints.size() < 2) {
		return waypoints.empty() ? Vector3(0, 0, 0) : waypoints[0].position;
	}

	// Get current and next waypoint
	int nextIndex = (waypointIndex + 1) % waypoints.size();
	Vector3 current = waypoints[waypointIndex].position;
	Vector3 next = waypoints[nextIndex].position;

	// Simple linear interpolation (Lerp)
	// For smoother motion, could use Catmull-Rom spline with 4 points
	return current + (next - current) * t;
}

// Interpolate angle with proper wrap-around handling
float Camera::InterpolateAngle(float start, float end, float t) {
	// Normalize angles to 0-360 range
	while (start < 0.0f) start += 360.0f;
	while (start >= 360.0f) start -= 360.0f;
	while (end < 0.0f) end += 360.0f;
	while (end >= 360.0f) end -= 360.0f;

	// Calculate the shortest rotation direction
	float diff = end - start;

	// Wrap diff to -180 to 180 range (shortest path)
	if (diff > 180.0f) {
		diff -= 360.0f;
	} else if (diff < -180.0f) {
		diff += 360.0f;
	}

	// Interpolate
	float result = start + diff * t;

	// Normalize result to 0-360
	while (result < 0.0f) result += 360.0f;
	while (result >= 360.0f) result -= 360.0f;

	return result;
}
