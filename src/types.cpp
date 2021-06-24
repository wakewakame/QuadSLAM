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

// Frame
Frame::Frame() {}
Frame::~Frame() {}
Frame::Frame(const Frame& frame) :
	frameNumber(frame.frameNumber), timestamp(frame.timestamp),
	camera(frame.camera.clone()), depth(frame.depth.clone()), confidence(frame.confidence.clone()),
	imu(frame.imu), gps(frame.gps), ar(frame.ar) {}
Frame::Frame(Frame&& frame) :
	frameNumber(frame.frameNumber), timestamp(frame.timestamp),
	camera(std::move(frame.camera)), depth(std::move(frame.depth)), confidence(std::move(frame.confidence)),
	imu(frame.imu), gps(frame.gps), ar(frame.ar) {}
Frame& Frame::operator=(const Frame& frame) {
	frameNumber = frame.frameNumber; timestamp = frame.timestamp;
	camera = frame.camera.clone(); depth = frame.depth.clone(); confidence = frame.confidence.clone();
	imu = frame.imu; gps = frame.gps; ar = frame.ar;
	return *this;
}
Frame& Frame::operator=(Frame&& frame) {
	frameNumber = frame.frameNumber; timestamp = frame.timestamp;
	camera = std::move(frame.camera); depth = std::move(frame.depth); confidence = std::move(frame.confidence);
	imu = frame.imu; gps = frame.gps; ar = frame.ar;
	return *this;
}
