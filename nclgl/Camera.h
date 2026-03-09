/******************************************************************************
Class:Camera
Implements:
Author:Rich Davison	<richard.davison4@newcastle.ac.uk>
Description:FPS-Style camera. Uses the mouse and keyboard from the Window
class to get movement values!

-_-_-_-_-_-_-_,------,   
_-_-_-_-_-_-_-|   /\_/\   NYANYANYAN
-_-_-_-_-_-_-~|__( ^ .^) /
_-_-_-_-_-_-_-""  ""   

*//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Window.h"
#include "Matrix4.h"
#include "Vector3.h"
#include <vector>

// Waypoint structure for camera track system
struct CameraWaypoint {
	Vector3 position;    // World position
	float pitch;         // Camera pitch (degrees)
	float yaw;           // Camera yaw (degrees)
	float duration;      // Time to stay/transition to next waypoint (seconds)

	CameraWaypoint() : position(0, 0, 0), pitch(0), yaw(0), duration(5.0f) {}

	CameraWaypoint(Vector3 pos, float p, float y, float dur)
		: position(pos), pitch(p), yaw(y), duration(dur) {}
};

class Camera	{
public:
	Camera(void){
		yaw		= 0.0f;
		pitch	= 0.0f;
		fov		= 45.0f;        // Default FOV
		autoMode = true;        // Start in auto mode
		currentWaypoint = 0;
		waypointTimer = 0.0f;
	};

	Camera(float pitch, float yaw, Vector3 position){
		this->pitch		= pitch;
		this->yaw		= yaw;
		this->position	= position;
		this->fov		= 45.0f;  // Default FOV
		autoMode = true;
		currentWaypoint = 0;
		waypointTimer = 0.0f;
	}

	~Camera(void){};

	void UpdateCamera(float msec = 10.0f);

	//Builds a view matrix for the current camera variables, suitable for sending straight
	//to a vertex skeletonShader (i.e it's already an 'inverse camera matrix').
	Matrix4 BuildViewMatrix();

	//Gets position in world space
	Vector3 GetPosition() const { return position;}
	//Sets position in world space
	void	SetPosition(Vector3 val) { position = val;}

	//Gets yaw, in degrees
	float	GetYaw()   const { return yaw;}
	//Sets yaw, in degrees
	void	SetYaw(float y) {yaw = y;}

	//Gets pitch, in degrees
	float	GetPitch() const { return pitch;}
	//Sets pitch, in degrees
	void	SetPitch(float p) {pitch = p;}

	// Field of View (FOV) control for zoom functionality
	float	GetFOV() const { return fov; }
	void	SetFOV(float f);  // Sets FOV with clamping
	void	AdjustFOV(float delta);  // Adjusts FOV by delta (for scroll wheel)

	// ========================================
	// Waypoint System (for automatic camera track)
	// ========================================
	void AddWaypoint(const CameraWaypoint& wp);
	void ClearWaypoints();
	void ResetTrack();  // Reset to first waypoint
	void ToggleMode();  // Toggle between auto and manual mode
	bool IsAutoMode() const { return autoMode; }

protected:
	// Manual FPS camera controls
	void UpdateManualCamera(float msec);

	// Automatic waypoint track
	void UpdateAutoCamera(float msec);

	// Smooth interpolation between waypoints
	Vector3 InterpolatePosition(float t, int waypointIndex);
	float InterpolateAngle(float start, float end, float t);

	float	yaw;
	float	pitch;
	Vector3 position;
	float	fov;  // Field of view in degrees (default: 45.0)

	// Waypoint track system
	std::vector<CameraWaypoint> waypoints;
	int currentWaypoint;      // Current waypoint index
	float waypointTimer;      // Time spent at/transitioning to current waypoint
	bool autoMode;            // true = auto track, false = manual FPS
};