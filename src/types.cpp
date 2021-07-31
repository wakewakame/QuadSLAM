#include "types.h"
#include <iostream>

using namespace qs;

// Camera
Camera Camera::clone() const {
	return Camera{
		timestamp,
		color.clone(),
		depth.clone(),
		confidence.clone(),
		intrinsicsMatrix.clone(),
		projectionMatrix.clone(),
		viewMatrix.clone()
	};
}

// GPS
cv::Vec3d Gps::cvGps() const { return cv::Vec3d(latitude, longitude, altitude); }

// IMU
cv::Vec3d Imu::cvGravity() const {
	return cv::Vec3d(gravityX, gravityY, gravityZ);
}
cv::Vec3d Imu::cvUserAccleration() const {
	return cv::Vec3d(userAcclerationX, userAcclerationY, userAcclerationZ);
}
cv::Vec3d Imu::cvAttitude() const {
	return cv::Vec3d(attitudeX, attitudeY, attitudeZ);
}

// QuadFrame
QuadFrame QuadFrame::clone() const {
	return QuadFrame{ timestamp, camera.clone(), imu, gps };
}
