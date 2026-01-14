//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Linux/Android gyroscope implementation for inputsystem
//
//===========================================================================//

#include "inputsystem.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"

#if defined(USE_SDL)
#include "SDL.h"

#if SDL_VERSION_ATLEAST(2, 0, 9)
#include "SDL_sensor.h"
#endif

#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#if defined(USE_SDL) && SDL_VERSION_ATLEAST(2, 0, 9)

//-----------------------------------------------------------------------------
// Handle the events coming from the Gyro SDL subsystem.
//-----------------------------------------------------------------------------
int GyroSDLWatcher( void *userInfo, SDL_Event *event )
{
	CInputSystem *pInputSystem = (CInputSystem *)userInfo;

	if( !event || !pInputSystem ) return 1;

	switch ( event->type ) {
	case SDL_SENSORUPDATE:
		// Check if this is a gyroscope sensor
		// SDL_SensorGetType would be needed here, but we'll assume it's gyro for now
		// event->sensor.data[0] = X axis angular velocity (rad/s)
		// event->sensor.data[1] = Y axis angular velocity (rad/s)
		// event->sensor.data[2] = Z axis angular velocity (rad/s)
		
		pInputSystem->GyroEvent( 
			event->sensor.data[0],  // pitch rate
			event->sensor.data[1],  // yaw rate
			event->sensor.data[2]   // roll rate
		);
		break;
	}

	return 1;
}

#endif // USE_SDL && SDL_VERSION_ATLEAST

//-----------------------------------------------------------------------------
// Initialize gyroscope
//-----------------------------------------------------------------------------
void CInputSystem::InitializeGyro( void )
{
	if ( m_bGyroInitialized )
		ShutdownGyro();

	// abort startup if user requests no gyro
	if ( CommandLine()->FindParm("-nogyro") ) return;

	m_gyroAccumPitch = 0.0f;
	m_gyroAccumYaw = 0.0f;
	m_gyroAccumRoll = 0.0f;

#if defined(USE_SDL) && SDL_VERSION_ATLEAST(2, 0, 9)
	
	// Find and open gyroscope sensor
	int numSensors = SDL_NumSensors();
	
	for (int i = 0; i < numSensors; i++)
	{
		SDL_SensorType type = SDL_SensorGetDeviceType(i);
		
		if (type == SDL_SENSOR_GYRO)
		{
			m_pGyroSensor = SDL_SensorOpen(i);
			
			if (m_pGyroSensor)
			{
				const char* name = SDL_SensorGetDeviceName(i);
				Msg("Gyroscope initialized: %s\n", name ? name : "Unknown");
				
				m_bGyroInitialized = true;
				
				// Register event watcher
				SDL_AddEventWatch(GyroSDLWatcher, this);
				
				return;
			}
			else
			{
				Warning("Failed to open gyroscope: %s\n", SDL_GetError());
			}
		}
	}
	
	if (!m_bGyroInitialized)
	{
		Msg("No gyroscope sensor found\n");
	}
	
#else
	Msg("Gyroscope not supported (SDL 2.0.9+ required)\n");
#endif
}

void CInputSystem::ShutdownGyro()
{
	if ( !m_bGyroInitialized )
		return;

#if defined(USE_SDL) && SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_DelEventWatch( GyroSDLWatcher, this );
	
	if (m_pGyroSensor)
	{
		SDL_SensorClose((SDL_Sensor*)m_pGyroSensor);
		m_pGyroSensor = NULL;
	}
#endif

	m_bGyroInitialized = false;
}

bool CInputSystem::GetGyroAccumulators( float &pitch, float &yaw, float &roll )
{
	pitch = m_gyroAccumPitch;
	yaw = m_gyroAccumYaw;
	roll = m_gyroAccumRoll;

	m_gyroAccumPitch = m_gyroAccumYaw = m_gyroAccumRoll = 0.0f;

	return m_bGyroInitialized;
}

void CInputSystem::GyroEvent(float pitchRate, float yawRate, float rollRate)
{
	if( !m_bGyroInitialized )
		return;

	// Convert from radians/sec to degrees and integrate over time
	// Assuming ~60 FPS, dt = 1/60 seconds
	const float dt = 1.0f / 60.0f;
	const float radToDeg = 57.2957795f; // 180 / PI
	
	float pitchDelta = pitchRate * dt * radToDeg;
	float yawDelta = yawRate * dt * radToDeg;
	float rollDelta = rollRate * dt * radToDeg;
	
	// Accumulate the deltas
	m_gyroAccumPitch += pitchDelta;
	m_gyroAccumYaw += yawDelta;
	m_gyroAccumRoll += rollDelta;

	// Optional: Post events for other systems to use
	// int _pitch, _yaw, _roll;
	// memcpy(&_pitch, &pitchDelta, sizeof(float));
	// memcpy(&_yaw, &yawDelta, sizeof(float));
	// memcpy(&_roll, &rollDelta, sizeof(float));
	// PostEvent(IE_GyroMotion, m_nLastSampleTick, _pitch, _yaw, _roll);
}
