#include "stdafx.h"

#include <fstream>
#include <sstream>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "object_detection.h"

#include "Tracker.h"
#include "HAlgorithm.h"
#include "queue"
#include <ctime>

using namespace cv;
using namespace dnn;

// detection parameters
float confThreshold, nmsThreshold;
int inpWidth = 224;
int inpHeight = 224;
float scale = 0.00392;
std::vector<std::string> road_sign_classes;
Net roadsing_net;

std::vector<std::string> level_classes;

std::string road_sign_result = "";
double duaration = 0;
clock_t tStart;


double road_signDuration = 0;
clock_t t_rStart;


Net level_net;
Mat back_frame;

int road_sign_count = 0;

void postprocess(Mat& frame, const std::vector<Mat>& outs);
void postprocess_level(Mat& frame, const std::vector<Mat>& outs, int origin_X, int origin_Y, int origin_widht, int origin_height, Mat& origin_frame);
std::vector<String> getOutputsNames(const Net& net);


std::vector<std::string> getClasses()
{
	return road_sign_classes;
}
std::string getResult()
{
	return road_sign_result;
}

std::vector<std::string> get_level_classes()
{
	return level_classes;
}
int get_count()
{
	return road_sign_count;
}
void set_count()
{
	road_sign_count = 0;;
}

int init_roadsign_dnn(std::string &names, std::string &config, std::string &model)
{
	confThreshold = 0.9;
	nmsThreshold = 0.8;
	//std::string file = "voc.names";
	std::ifstream ifs(names.c_str());
	if (!ifs.is_open())
		return -1;
		//CV_Error(Error::StsError, "File " + file + " not found");
	std::string line;
	while (std::getline(ifs, line))
	{
		road_sign_classes.push_back(line);
	}
	roadsing_net = readNet(model, config);
	roadsing_net.setPreferableBackend(0);
	roadsing_net.setPreferableTarget(0);
	return 0;
}

int init_level_dnn(std::string &names, std::string &config, std::string &model)
{
	confThreshold = 0.9;
	nmsThreshold = 0.8;
	//std::string file = "voc.names";
	std::ifstream ifs(names.c_str());
	if (!ifs.is_open())
		return -1;
	//CV_Error(Error::StsError, "File " + file + " not found");
	std::string line;
	while (std::getline(ifs, line))
	{
		level_classes.push_back(line);
	}
	level_net = readNet(model, config);
	level_net.setPreferableBackend(0);
	level_net.setPreferableTarget(0);
	return 0;
}

void drawPred_image(int classId, float conf, int left, int top, int right, int bottom, Mat frame)
{
	Rect rt(left, top, right - left, bottom - top);
	//Draw a rectangle displaying the bounding box
	rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0), 3);
	//Get the label for the class name and its confidence
	string label = road_sign_classes[classId];
	//Display the label at the top of the bounding box
	int baseLine;
	Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
	top = max(top, labelSize.height);
	putText(frame, label, Point(left - 10, top - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 0), 2);
	
}

void drawPred_image_level(int classId, float conf, int left, int top, int right, int bottom, Mat frame, int origin_width, int origin_height)
{
	duaration = (double)(clock() - tStart);
	
	Rect rt(left, top, right - left, bottom - top);
	//Draw a rectangle displaying the bounding box
	rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 0, 255), 2);
		
	//Get the label for the class name and its confidence
	string label = std::to_string(right - left) + " Pixel";
		
	putText(frame, label, Point(left + origin_width / 3, top - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0), 2);
	label = std::to_string(bottom - top) + " Pixel";
		
	putText(frame, label, Point(left + origin_width + 5, top + (bottom - top) / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0), 2);
	cv::line(frame, cv::Point(left + origin_width / 2, bottom), cv::Point(left + origin_width / 2, bottom + (origin_height - (bottom - top))), Scalar(0, 255, 255), 2, 8, 0);
	label = std::to_string(origin_height - (bottom - top)) + " Pixel";
	putText(frame, label, Point(left + origin_width / 3, bottom + (origin_height - (bottom - top)) / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0), 2);
	//Display the label at the top of the bounding box
	int baseLine;
	Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
	top = max(top, labelSize.height);
	if (duaration > 2000)
	{
		road_sign_result = "Board";
		road_sign_result += ":" + std::to_string(left);
		road_sign_result += ":" + std::to_string(top);
		road_sign_result += ":" + to_string(right - left);
		road_sign_result += ":" + to_string(bottom - top);
		road_sign_result += ":" + to_string((origin_height - (bottom - top)));
		road_sign_count++;
	}
	tStart = clock();
}
Mat object_detection(Mat frame)
{	
	road_sign_result = "";
	back_frame = frame.clone();
    // Process frames.
    Mat blob;
    if (frame.empty())
    {
		return frame;
    }
    // Create a 4D blob from a frame.
	blobFromImage(frame, blob, 1 / 255.0, Size(inpWidth, inpHeight), Scalar(0, 0, 0), true, false);
	roadsing_net.setInput(blob);
    std::vector<Mat> outs;
	roadsing_net.forward(outs, getOutputsNames(roadsing_net));
    postprocess(frame, outs);
	return frame;
}


// process after detection
void postprocess(Mat& frame, const std::vector<Mat>& outs)
{
	vector<int> classIds;
	vector<float> confidences;
	vector<Rect> boxes;

	for (size_t i = 0; i < outs.size(); ++i)
	{
		float* data = (float*)outs[i].data;
		for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
		{
			Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
			Point classIdPoint;
			double confidence;
			// Get the value and location of the maximum score
			minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
			if (confidence > confThreshold)
			{
				int centerX = (int)(data[0] * frame.cols);
				int centerY = (int)(data[1] * frame.rows);
				int width = (int)(data[2] * frame.cols);
				int height = (int)(data[3] * frame.rows);
				int left = centerX - width / 2;
				int top = centerY - height / 2;
				classIds.push_back(classIdPoint.x);
				confidences.push_back((float)confidence);
				boxes.push_back(Rect(left, top, width, height));
			}
		}
	}

	vector<int> indices;
	NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
	
	for (size_t i = 0; i < indices.size(); ++i)
	{
		int idx = indices[i];
		Rect box = boxes[idx];


		int w, h;
		w = box.x + box.width;
		h = box.y + box.height;
		if (w >= frame.cols) w = frame.cols;
		if (h >= frame.rows) h = frame.rows;
		if (box.x < 0) box.x = 0;
		if (box.y < 0) box.y = 0;
		
		
		if (box.x < 0) box.x = 0;
		if (box.y < 0) box.y = 0;
		if (w > back_frame.cols) w = back_frame.cols;
		if (h > back_frame.rows) w = back_frame.rows;

		
		drawPred_image(classIds[idx], confidences[idx], box.x, box.y,
			w, h, frame);
		
		if (road_sign_classes[classIds[idx]] == "level")
		{
			if (h < frame.rows * 3 / 4 + 5 && h > frame.rows * 3 / 4 - 5)
			{
				cv::Rect myROI(box.x, box.y, w - box.x, h - box.y);
				// Crop the full image to that image contained by the rectangle myROI
				// Note that this doesn't copy the data
				cv::Mat croppedImage = back_frame(myROI);
				Mat blob;

				blobFromImage(croppedImage, blob, 1 / 255.0, Size(128, 128), Scalar(0, 0, 0), true, false);
				level_net.setInput(blob);
				std::vector<Mat> outs;
				level_net.forward(outs, getOutputsNames(level_net));
				postprocess_level(croppedImage, outs, box.x, box.y, w - box.x, h - box.y, frame);
			}
		}
		else
		{
			if ((box.x > 150 && box.x < 180) || (w > frame.cols - 180 && w < frame.cols - 150))
			{
				road_signDuration = (double)(clock() - t_rStart);
				if (road_signDuration > 500)
				{
					road_sign_result = road_sign_classes[classIds[idx]] + ":" + to_string(box.x) + ":" + to_string(box.y)
						+ ":" + to_string(w - box.x) + ":" + to_string(h - box.y);
					road_sign_count++;
				}

				t_rStart = clock();
			}
			
		}
		

	}
}
void postprocess_level(Mat& frame, const std::vector<Mat>& outs, int origin_X, int origin_Y, int origin_widht, int origin_height, Mat& origin_frame)
{
	vector<int> classIds;
	vector<float> confidences;
	vector<Rect> boxes;

	for (size_t i = 0; i < outs.size(); ++i)
	{
		float* data = (float*)outs[i].data;
		for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
		{
			Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
			Point classIdPoint;
			double confidence;
			// Get the value and location of the maximum score
			minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
			if (confidence > confThreshold)
			{
				int centerX = (int)(data[0] * frame.cols);
				int centerY = (int)(data[1] * frame.rows);
				int width = (int)(data[2] * frame.cols);
				int height = (int)(data[3] * frame.rows);
				int left = centerX - width / 2;
				int top = centerY - height / 2;

				classIds.push_back(classIdPoint.x);

				confidences.push_back((float)confidence);
				boxes.push_back(Rect(left, top, width, height));
			}
		}
	}
	vector<int> indices;
	NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

	for (size_t i = 0; i < indices.size(); ++i)
	{
		int idx = indices[i];
		Rect box = boxes[idx];


		int w, h;
		w = box.x + box.width;
		h = box.y + box.height;
		if (w >= frame.cols) w = frame.cols;
		if (h >= frame.rows) h = frame.rows;
		if (box.x < 0) box.x = 0;
		if (box.y < 0) box.y = 0;

		drawPred_image_level(classIds[idx], confidences[idx], origin_X + box.x, origin_Y + box.y,
			origin_X + w, origin_Y + h, origin_frame, origin_widht, origin_height);
	}
}


std::vector<String> getOutputsNames(const Net& net)
{
    static std::vector<String> names;
    if (names.empty())
    {
        std::vector<int> outLayers = net.getUnconnectedOutLayers();
        std::vector<String> layersNames = net.getLayerNames();
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}
