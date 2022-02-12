#include "types.h"

using namespace qs;

// Camera
Camera Camera::clone() const {
	return Camera{
		frameNumber,
		timestamp,
		color.clone(),
		depth.clone(),
		confidence.clone(),
		intrinsicsMatrix.clone(),
		projectionMatrix.clone(),
		viewMatrix.clone()
	};
}

// IMU
cv::Vec3d Imu::cvGravity() const {
	return cv::Vec3d(gravityX, gravityY, gravityZ);
}
cv::Vec3d Imu::cvUserAccleration() const {
	return cv::Vec3d(userAcclerationX, userAcclerationY, userAcclerationZ);
}
cv::Vec3d Imu::cvRotationRate() const {
	return cv::Vec3d(rotationRateX, rotationRateY, rotationRateZ);
}
cv::Vec3d Imu::cvAttitude() const {
	return cv::Vec3d(attitudeX, attitudeY, attitudeZ);
}

// GPS
cv::Vec3d Gps::cvGps() const { return cv::Vec3d(latitude, longitude, altitude); }

// QuadFrame
QuadFrame QuadFrame::clone() const {
	return QuadFrame{ camera.clone(), imu, gps };
}

// CameraForOrm
Camera CameraForOrm::toCamera(Description description, cv::Mat color) const {
	// フレーム番号
	uint64_t frameNumber = 0;
	if (colorFrame.has_value()) { frameNumber = colorFrame.value(); }

	// デプスの取得
	cv::Mat depth;
	if (
		depthZlib.has_value() &&
		description.depthWidth.has_value() &&
		description.depthHeight.has_value()
	) do {
		cv::Mat depth_;
		const std::vector<char>& depthZlib_ = depthZlib.value();
		const uint64_t& depthWidth_  = description.depthWidth.value();
		const uint64_t& depthHeight_ = description.depthHeight.value();

		// cv::Matは行間にアライメントが混入する場合があるが、
		// cv::Mat::createによって作成された配列は常に連続となる
		depth_.create(depthHeight_, depthWidth_, CV_32FC1);
		assert(depth_.isContinuous());

		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)depthZlib_.data();
		stream.avail_in = depthZlib_.size();
		stream.next_out = (Bytef*)depth_.ptr<char>(0);
		stream.avail_out = depth_.total() * depth_.elemSize();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { break; }
		inflateEnd(&stream);
		depth = std::move(depth_);
	} while(false);

	// 信頼度の取得
	cv::Mat confidence;
	if (
		confidenceZlib.has_value() &&
		description.confidenceWidth.has_value() &&
		description.confidenceHeight.has_value()
	) do {
		cv::Mat confidence_;
		const std::vector<char>& confidenceZlib_ = confidenceZlib.value();
		const uint64_t& confidenceWidth_  = description.confidenceWidth.value();
		const uint64_t& confidenceHeight_ = description.confidenceHeight.value();

		// cv::Matは行間にアライメントが混入する場合があるが、
		// cv::Mat::createによって作成された配列は常に連続となる
		confidence_.create(confidenceHeight_, confidenceWidth_, CV_8UC1);
		assert(confidence_.isContinuous());

		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)confidenceZlib_.data();
		stream.avail_in = confidenceZlib_.size();
		stream.next_out = (Bytef*)confidence_.ptr<char>(0);
		stream.avail_out = confidence_.total() * confidence_.elemSize();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { break; }
		inflateEnd(&stream);
		confidence = std::move(confidence_);
	} while(false);

	// ARKitから取得した行列の取得
	cv::Mat intrinsics, projection, view;
	if (intrinsicsMatrix.has_value()) {
		intrinsics.create(3, 3, CV_32F);
		std::memcpy(intrinsics.ptr(0), intrinsicsMatrix->data(), intrinsicsMatrix->size());
	}
	if (projectionMatrix.has_value()) {
		projection.create(4, 4, CV_32F);
		std::memcpy(projection.ptr(0), projectionMatrix->data(), projectionMatrix->size());
	}
	if (viewMatrix.has_value()) {
		view.create(4, 4, CV_32F);
		std::memcpy(view.ptr(0), viewMatrix->data(), viewMatrix->size());
	}

	return Camera {
		frameNumber, timestamp,
		std::move(color), std::move(depth), std::move(confidence),
		std::move(intrinsics), std::move(projection), std::move(view)
	};
}
