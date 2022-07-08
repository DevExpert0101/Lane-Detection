#ifndef __LPR_TRACKER_H
#define __LPR_TRACKER_H

#pragma once

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>

#include <ctime>

class __declspec(dllexport) Tracker {
	cv::Mat x_state;

	cv::Mat F;
	cv::Mat H;
	cv::Mat P;
	cv::Mat Q;
	cv::Mat R;

	float L;
	float R_ratio;

	__int64 firstTime;
	__int64 lastTime;

	int countLessThan(std::vector<int> arr, int cre);
	int countGreaterThan(std::vector<int> arr, int cre);
public:
	char id;
	cv::Rect curBox;
	std::string className;
	double confidence;
	cv::Mat frame;

	int hits;
	int no_losses;
	int hit_time;
	int dt;

	int duration;

	std::vector<cv::Rect> history;


	Tracker(char);

	~Tracker();

	void update_R();
	void Kalman_filter(cv::Rect z, __int64 atime);
	void predict_only();
	void set_xState(cv::Rect z, __int64 atime);

	cv::Point2f estimatedSpeed();
	cv::Rect getCurPos();

	int getDuration();
};

#endif