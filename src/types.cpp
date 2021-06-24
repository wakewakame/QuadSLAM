#include "types.h"
#include <iostream>

using namespace qs;

// IMU
cv::Vec3d IMU::cvGravity()         { return cv::Vec3d(gravity);         }
cv::Vec3d IMU::cvUserAccleration() { return cv::Vec3d(userAccleration); }
cv::Vec3d IMU::cvAttitude()        { return cv::Vec3d(attitude);        }

// GPS
cv::Vec3d GPS::cvGPS() { return cv::Vec3d(latitude, longitude, altitude); }

// AR
cv::Mat AR::cvIntrinsics()       { return cv::Mat(3, 3, CV_32F, intrinsics      ).clone(); }
cv::Mat AR::cvProjectionMatrix() { return cv::Mat(4, 4, CV_32F, projectionMatrix).clone(); }
cv::Mat AR::cvViewMatrix()       { return cv::Mat(4, 4, CV_32F, viewMatrix      ).clone(); }

// Camera
Camera::Camera() {}
Camera::~Camera() {}
Camera::Camera(const Camera& frame) :
	timestamp(frame.timestamp),
	color(frame.color.clone()),
	depth(frame.depth.clone()),
	confidence(frame.confidence.clone()),
	ar(frame.ar) {}
Camera::Camera(Camera&& frame) :
	timestamp(frame.timestamp),
	color(std::move(frame.color)),
	depth(std::move(frame.depth)),
	confidence(std::move(frame.confidence)),
	ar(frame.ar) {}
Camera& Camera::operator=(const Camera& frame) {
	timestamp = frame.timestamp;
	color = frame.color.clone();
	depth = frame.depth.clone();
	confidence = frame.confidence.clone();
	ar = frame.ar;
	return *this;
}
Camera& Camera::operator=(Camera&& frame) {
	timestamp = frame.timestamp;
	color = std::move(frame.color);
	depth = std::move(frame.depth);
	confidence = std::move(frame.confidence);
	ar = frame.ar;
	return *this;
}
