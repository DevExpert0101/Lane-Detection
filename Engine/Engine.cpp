// Engine.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <Math.h>

#include "object_detection.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

typedef void(*get_current_frame)(std::string results);

void __declspec(dllexport) Init();
int LaneDetection(Mat image);
int __declspec(dllexport) Run(std::string filename);
void __declspec(dllexport) StopEngine();
void __declspec(dllexport) GetCurrentFrame(unsigned char *buffer);
void __declspec(dllexport) ReceivedHandle(get_current_frame handle);
int __declspec(dllexport) GetFrameWidth();
int __declspec(dllexport) GetFrameHeight();
int __declspec(dllexport) GetFrameChannels();

unsigned char *_current_buffer = 0;
HANDLE _mutex_current_buffer;
bool _is_stop = false;
vector<int> previous_lane_width;

get_current_frame _onFrame;
int _frame_width, _frame_height, _frame_channels;


void WaitEvent(HANDLE hEvent)
{
	if (hEvent != NULL)
		WaitForSingleObject(hEvent, INFINITE);
	ResetEvent(hEvent);
}

void SendEvent(HANDLE hEvent)
{
	SetEvent(hEvent);
}

int __declspec(dllexport) GetFrameWidth()
{
	return _frame_width;
}

int __declspec(dllexport) GetFrameHeight()
{
	return _frame_height;
}

int __declspec(dllexport) GetFrameChannels()
{
	return _frame_channels;
}

void __declspec(dllexport) ReceivedHandle(get_current_frame handle)
{
	_onFrame = handle;
}

void __declspec(dllexport) GetCurrentFrame(unsigned char *buffer)
{
	try {
		WaitEvent(_mutex_current_buffer);
		if (_current_buffer != NULL)
			memcpy(buffer, _current_buffer, _frame_width * _frame_height * _frame_channels);
		SendEvent(_mutex_current_buffer);
	}
	catch (std::exception ex)
	{

	}
}

void __declspec(dllexport) Init()
{
	std::string modelConfiguration_detect = "v.cfg";
	std::string modelBinary_detect = "v.weights";
	std::string modelDetectionNames = "v.names";
	init_roadsign_dnn(modelDetectionNames, modelConfiguration_detect, modelBinary_detect);

	modelConfiguration_detect = "level.cfg";
	modelBinary_detect = "level.weights";
	modelDetectionNames = "level.names";
	init_level_dnn(modelDetectionNames, modelConfiguration_detect, modelBinary_detect);

	std::string event_name = "my_vehicle_counter_system";
	_mutex_current_buffer = CreateEvent(NULL, TRUE, FALSE, (event_name.c_str()));
	SendEvent(_mutex_current_buffer);
}

double median(vector<double> vec) {

	// get size of vector
	int vecSize = vec.size();

	// if vector is empty throw error
	if (vecSize == 0) {
		return 0;
	}

	// sort vector
	if (vec.size() != 0)
		sort(vec.begin(), vec.end());

	// define middle and median
	int middle;
	double median;

	// if even number of elements in vec, take average of two middle values
	if (vecSize % 2 == 0) {
		// a value representing the middle of the array. If array is of size 4 this is 2
		// if it's 8 then middle is 4
		middle = vecSize / 2;

		// take average of middle values, so if vector is [1, 2, 3, 4] we want average of 2 and 3
		// since we index at 0 middle will be the higher one vec[2] in the above vector is 3, and vec[1] is 2
		median = (vec[middle - 1] + vec[middle]) / 2;
	}

	// odd number of values in the vector
	else {
		middle = vecSize / 2; // take the middle again

							  // if vector is 1 2 3 4 5, middle will be 5/2 = 2, and vec[2] = 3, the middle value
		median = vec[middle];
	}

	return median;
}

double round_up(double value, int decimal_places) {
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}

int LaneDetection(Mat image)
{
	if (image.empty())
	{
		cout << "Could not open or find the image" << endl;
		return 0;
	}
	
	cv::Mat imageGray;
	cv::cvtColor(image, imageGray, cv::COLOR_BGR2GRAY);
	int kernelSize = 1;
	cv::Mat smoothedIm;
	cv::GaussianBlur(imageGray, smoothedIm, cv::Size(kernelSize, kernelSize), 0, 0);
	int minVal = 60;
	int maxVal = 150;
	cv::Mat edgesIm;
	cv::Canny(smoothedIm, edgesIm, minVal, maxVal);
	cv::Mat mask(image.size().height, image.size().width, CV_8UC1, cv::Scalar(0)); // CV_8UC3 to make it a 3 channel

																				   // Define the points for the mask
																			   // Use cv::Point type for x,y points
	cv::Point p1 = cv::Point(0, image.size().height);
	cv::Point p2 = cv::Point(155, 580);
	cv::Point p3 = cv::Point(805, 580);
	cv::Point p4 = cv::Point(905, 660);
	cv::Point p5 = cv::Point(1005, 660);
	cv::Point p6 = cv::Point(image.size().width * 0.85, image.size().height);
	
	cv::Point vertices1[] = { p1,p2,p3, p4, p5, p6 };
	std::vector<cv::Point> vertices(vertices1, vertices1 + sizeof(vertices1) / sizeof(cv::Point));

	
	std::vector<std::vector<cv::Point> > verticesToFill;
	verticesToFill.push_back(vertices);

	cv::fillPoly(mask, verticesToFill, cv::Scalar(255, 255, 255));
	//cv::imshow("mask", mask);
	

	cv::Mat maskedIm = edgesIm.clone();
	cv::bitwise_and(edgesIm, mask, maskedIm);

	//cv::imshow("maskedIm", maskedIm);
	//cv::waitKey(1);

	//------------------------HOUGH LINES----------------------------
	float rho = 2;
	float pi = 3.14159265358979323846;
	float theta = pi / 180;
	float threshold = 45;
	int minLineLength = 40;
	int maxLineGap = 50;
	double length = 0;
	//bool gotLines = false;

	// Variables for once we have line averages
	//float posSlopeMean = 0;
	//double xInterceptPosMean = 0;
	//float negSlopeMean = 0;
	//double xInterceptNegMean = 0;



	vector<cv::Vec4i> lines; // A Vec4i is a type holding 4 integers
	cv::HoughLinesP(maskedIm, lines, rho, theta, threshold, minLineLength, maxLineGap);

	// Check if we got more than one line
	if (!lines.empty() && lines.size() > 2) {

		// Initialize lines image
		cv::Mat allLinesIm(image.size().height, image.size().width, CV_8UC3, cv::Scalar(0, 0, 0)); // CV_8UC3 to make it a 3 channel)

																								   // Loop through lines
																								   // std::size_t can store the maximum size of a theoretically possible object of any type
		for (size_t i = 0; i != lines.size(); ++i) {

			// Draw line onto image
			cv::line(allLinesIm, cv::Point(lines[i][0], lines[i][1]),
				cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 0, 255), 3, 8);
		}

		//---------------Separate Lines Into Positive/Negative Slope--------------------
		// Separate line segments by their slope to decide left line vs. the right line

		// Define arrays for positive/negative lines
		vector< vector<double> > slopePositiveLines; // format will be [x1 y1 x2 y2 slope]
		vector< vector<double> > slopeNegativeLines;
		vector<float> yValues;
		//float slopePositiveLines[] = {};
		//float slopeNegativeLines[] = {};

		// keep track of if we added one of each, want at least one of each to proceed
		bool addedPos = false;
		bool addedNeg = false;

		// array counter for appending new lines
		int negCounter = 0;
		int posCounter = 0;

		// Loop through all lines
		for (size_t i = 0; i != lines.size(); ++i) {

			// Get points for current line
			float x1 = lines[i][0];
			float y1 = lines[i][1];
			float x2 = lines[i][2];
			float y2 = lines[i][3];

			// get line length
			float lineLength = pow(pow(x2 - x1, 2) + pow(y2 - y1, 2), .5);

			// if line is long enough
			if (lineLength > 30) {

				// dont divide by zero
				if (x2 != x1) {

					// get slope
					float slope = (y2 - y1) / (x2 - x1);

					// Check if slope is positive
					if (slope > 0) {

						// Find angle of line wrt x axis.
						float tanTheta = tan((abs(y2 - y1)) / (abs(x2 - x1))); // tan(theta) value
						float angle = atan(tanTheta) * 180 / pi;

						// Only pass good line angles,  dont want verticalish/horizontalish lines
						if (abs(angle) < 85 && abs(angle) > 20) {

							// Add a row to the matrix
							slopeNegativeLines.resize(negCounter + 1);

							// Reshape current row to 5 columns [x1, y1, x2, y2, slope]
							slopeNegativeLines[negCounter].resize(5);

							// Add values to row
							slopeNegativeLines[negCounter][0] = x1;
							slopeNegativeLines[negCounter][1] = y1;
							slopeNegativeLines[negCounter][2] = x2;
							slopeNegativeLines[negCounter][3] = y2;
							slopeNegativeLines[negCounter][4] = -slope;

							// add yValues
							yValues.push_back(y1);
							yValues.push_back(y2);

							// Note that we added a positive slope line
							addedPos = true;

							// iterate the counter
							negCounter++;

						}

					}

					// Check if slope is Negative
					if (slope < 0) {

						// Find angle of line wrt x axis.
						float tanTheta = tan((abs(y2 - y1)) / (abs(x2 - x1))); // tan(theta) value
						float angle = atan(tanTheta) * 180 / pi;

						// Only pass good line angles,  dont want verticalish/horizontalish lines
						if (abs(angle) < 85 && abs(angle) > 20) {

							// Add a row to the matrix
							slopePositiveLines.resize(posCounter + 1);

							// Reshape current row to 5 columns [x1, y1, x2, y2, slope]
							slopePositiveLines[posCounter].resize(5);

							// Add values to row
							slopePositiveLines[posCounter][0] = x1;
							slopePositiveLines[posCounter][1] = y1;
							slopePositiveLines[posCounter][2] = x2;
							slopePositiveLines[posCounter][3] = y2;
							slopePositiveLines[posCounter][4] = -slope;

							// add yValues
							yValues.push_back(y1);
							yValues.push_back(y2);

							// Note that we added a positive slope line
							addedNeg = true;

							// iterate counter
							posCounter++;

						}

					} // if slope < 0
				} // if x2 != x1
			}// if lineLength > 30
			 // cout << endl;
		} // looping though all lines


	  // If we didn't get any positive lines, go though again and just add any positive slope lines
	  // Be less strict
		if (addedPos == false) { // if we didnt add any positive lines

								 // loop through lines
			for (size_t i = 0; i != lines.size(); ++i) {

				// Get points for current line
				float x1 = lines[i][0];
				float y1 = lines[i][1];
				float x2 = lines[i][2];
				float y2 = lines[i][3];

				// Get slope
				float slope = (y2 - y1) / (x2 - x1);

				// Check if slope is positive
				if (slope > 0 && x2 != x1) {

					// Find angle of line wrt x axis.
					float tanTheta = tan((abs(y2 - y1)) / (abs(x2 - x1))); // tan(theta) value
					float angle = atan(tanTheta) * 180 / pi;

					// Only pass good line angles,  dont want verticalish/horizontalish lines
					if (abs(angle) < 85 && abs(angle) > 15) {

						// Add a row to the matrix
						slopeNegativeLines.resize(negCounter + 1);

						// Reshape current row to 5 columns [x1, y1, x2, y2, slope]
						slopeNegativeLines[negCounter].resize(5);

						// Add values to row
						slopeNegativeLines[negCounter][0] = x1;
						slopeNegativeLines[negCounter][1] = y1;
						slopeNegativeLines[negCounter][2] = x2;
						slopeNegativeLines[negCounter][3] = y2;
						slopeNegativeLines[negCounter][4] = -slope;

						// add yValues
						yValues.push_back(y1);
						yValues.push_back(y2);

						// Note that we added a positive slope line
						addedPos = true;

						// iterate the counter
						negCounter++;
					}
				}

			}
		} // if addedPos == false


		  // If we didn't get any negative lines, go though again and just add any positive slope lines
		  // Be less strict
		if (addedNeg == false) { // if we didnt add any positive lines

								 // loop through lines
			for (size_t i = 0; i != lines.size(); ++i) {

				// Get points for current line
				float x1 = lines[i][0];
				float y1 = lines[i][1];
				float x2 = lines[i][2];
				float y2 = lines[i][3];

				// Get slope
				float slope = (y2 - y1) / (x2 - x1);

				// Check if slope is positive
				if (slope > 0 && x2 != x1) {

					// Find angle of line wrt x axis.
					float tanTheta = tan((abs(y2 - y1)) / (abs(x2 - x1))); // tan(theta) value
					float angle = atan(tanTheta) * 180 / pi;

					// Only pass good line angles,  dont want verticalish/horizontalish lines
					if (abs(angle) < 85 && abs(angle) > 15) {

						// Add a row to the matrix
						slopePositiveLines.resize(posCounter + 1);

						// Reshape current row to 5 columns [x1, y1, x2, y2, slope]
						slopePositiveLines[posCounter].resize(5);

						if (slopeNegativeLines.size() > posCounter)
						{
							// Add values to row
							slopeNegativeLines[posCounter][0] = x1;
							slopeNegativeLines[posCounter][1] = y1;
							slopeNegativeLines[posCounter][2] = x2;
							slopeNegativeLines[posCounter][3] = y2;
							slopeNegativeLines[posCounter][4] = -slope;

							// add yValues
							yValues.push_back(y1);
							yValues.push_back(y2);

							// Note that we added a positive slope line
							addedNeg = true;

							// iterate the counter
							posCounter++;
						}

					}
				}

			}
		} // if addedNeg == false

		if (slopeNegativeLines.size() > 20)
		{

			for (int z = 0; z < slopeNegativeLines.size(); z++)
			{
				if (z < slopeNegativeLines.size())
				{
					if (slopeNegativeLines[z][4] > -0.8)
					{
						slopeNegativeLines.erase(slopeNegativeLines.begin() + z);
						z--;
					}
				}

			}
		}


		vector<float> positiveSlopes;
		for (unsigned int i = 0; i != slopePositiveLines.size(); ++i) {
			positiveSlopes.push_back(slopePositiveLines[i][4]);
		}

		// Get median of positiveSlopes
		sort(positiveSlopes.begin(), positiveSlopes.end()); // sort vec
		int middle; // define middle value
		double posSlopeMedian; // define positive slope median

							   // if even number of elements in vec, take average of two middle values
		if (positiveSlopes.size() % 2 == 0) {

			// a value representing the middle of the array. If array is of size 4 this is 2
			// if it's 8 then middle is 4
			middle = positiveSlopes.size() / 2;

			// take average of middle values, so if vector is [1, 2, 3, 4] we want average of 2 and 3
			// since we index at 0 middle will be the higher one vec[2] in the above vector is 3, and vec[1] is 2
			if (positiveSlopes.size() > middle - 1)
				posSlopeMedian = (positiveSlopes[middle - 1] + positiveSlopes[middle]) / 2;
		}

		// odd number of values in the vector
		else {
			middle = positiveSlopes.size() / 2; // take the middle again
												// if vector is 1 2 3 4 5, middle will be 5/2 = 2, and vec[2] = 3, the middle value
			posSlopeMedian = positiveSlopes[middle];
		}

		// Define vector of 'good' slopes, slopes that are drastically different than the others are thrown out
		vector<float> posSlopesGood;
		float posSum = 0.0; // sum so we'll be able to get mean

							// Loop through positive slopes and add the good ones
		for (size_t i = 0; i != positiveSlopes.size(); ++i) {

			// check difference between current slope and the median. If the difference is small enough it's good
			if (abs(positiveSlopes[i] - posSlopeMedian) < posSlopeMedian*.2) {
				posSlopesGood.push_back(positiveSlopes[i]); // Add slope to posSlopesGood
				posSum += positiveSlopes[i]; // add to sum
			}
		}

		// Get mean of good positive slopes
		float posSlopeMean = posSum / posSlopesGood.size();

		////////////////////////////////////////////////////////////////////////

		// Add negative slopes from slopeNegativeLines into a vector negative slopes
		vector<float> negativeSlopes;
		for (size_t i = 0; i != slopeNegativeLines.size(); ++i) {
			negativeSlopes.push_back(slopeNegativeLines[i][4]);
		}

		// Get median of negativeSlopes
		sort(negativeSlopes.begin(), negativeSlopes.end()); // sort vec
		int middleNeg; // define middle value
		double negSlopeMedian; // define negative slope median

							   // if even number of elements in vec, take average of two middle values
		if (negativeSlopes.size() % 2 == 0) {

			// a value representing the middle of the array. If array is of size 4 this is 2
			// if it's 8 then middle is 4
			middleNeg = negativeSlopes.size() / 2;

			// take average of middle values, so if vector is [1, 2, 3, 4] we want average of 2 and 3
			// since we index at 0 middle will be the higher one vec[2] in the above vector is 3, and vec[1] is 2
			if (negativeSlopes.size() > middleNeg - 1)
				negSlopeMedian = (negativeSlopes[middleNeg - 1] + negativeSlopes[middleNeg]) / 2;
		}

		// odd number of values in the vector
		else {
			middleNeg = negativeSlopes.size() / 2; // take the middle again

				negSlopeMedian = negativeSlopes[middleNeg];
		}

		// Define vector of 'good' slopes, slopes that are drastically different than the others are thrown out
		vector<float> negSlopesGood;
		float negSum = 0.0; // sum so we'll be able to get mean

							//std::cout << "negativeSlopes.size(): " << negativeSlopes.size() << endl;
							//std::cout << "condition: " << negSlopeMedian*.2 << endl;

							// Loop through positive slopes and add the good ones
		for (size_t i = 0; i != negativeSlopes.size(); ++i) {

			//cout << "check: " << negativeSlopes[i]  << endl;

			// check difference between current slope and the median. If the difference is small enough it's good
			if (abs(negativeSlopes[i] - negSlopeMedian) < .9) { // < negSlopeMedian*.2
				negSlopesGood.push_back(negativeSlopes[i]); // Add slope to negSlopesGood
				negSum += negativeSlopes[i]; // add to sum
			}
		}

		//cout << endl;
		// Get mean of good positive slopes
		float negSlopeMean = negSum / negSlopesGood.size();
		//std::cout << "negSum: " << negSum << " negSlopesGood.size(): " << negSlopesGood.size() << " negSlopeMean: " << negSlopeMean << endl;


		//----------------GET AVERAGE X COORD WHEN Y COORD OF LINE = 0--------------------
		// Positive Lines
		vector<double> xInterceptPos; // define vector for x intercepts of positive slope lines

									  // Loop through positive slope lines, find and store x intercept values
		for (size_t i = 0; i != slopePositiveLines.size(); ++i) {
			double x1 = slopePositiveLines[i][0]; // x value
			double y1 = image.rows - slopePositiveLines[i][1]; // y value...yaxis is flipped
			double slope = slopePositiveLines[i][4];
			double yIntercept = y1 - slope*x1; // yintercept of line
			double xIntercept = -yIntercept / slope; // find x intercept based off y = mx+b
			if (isnan(xIntercept) == 0) { // check for nan
				xInterceptPos.push_back(xIntercept); // add value
			}
		}

		// Get median of x intercepts for positive slope lines
		double xIntPosMed = median(xInterceptPos);

		// Define vector storing 'good' x intercept values, same concept as the slope calculations before
		vector<double> xIntPosGood;
		double xIntSum = 0; // for finding avg

							// Now that we got median, loop through lines again and compare values against median
		for (size_t i = 0; i != slopePositiveLines.size(); ++i) {
			double x1 = slopePositiveLines[i][0]; // x value
			double y1 = image.rows - slopePositiveLines[i][1]; // y value...yaxis is flipped
			double slope = slopePositiveLines[i][4];
			double yIntercept = y1 - slope*x1; // yintercept of line
			double xIntercept = -yIntercept / slope; // find x intercept based off y = mx+b

													 // check for nan and check if it's close enough to the median
			if (isnan(xIntercept) == 0 && abs(xIntercept - xIntPosMed) < .35*xIntPosMed) {
				xIntPosGood.push_back(xIntercept); // add to 'good' vector
				xIntSum += xIntercept;
			}
		}

		// Get mean x intercept value for positive slope lines
		double xInterceptPosMean = xIntSum / xIntPosGood.size();

		/////////////////////////////////////////////////////////////////
		// Negative Lines
		vector<double> xInterceptNeg; // define vector for x intercepts of negative slope lines

									  // Loop through negative slope lines, find and store x intercept values
		for (size_t i = 0; i != slopeNegativeLines.size(); ++i) {
			double x1 = slopeNegativeLines[i][0]; // x value
			double y1 = image.rows - slopeNegativeLines[i][1]; // y value...yaxis is flipped
			double slope = slopeNegativeLines[i][4];
			double yIntercept = y1 - slope*x1; // yintercept of line
			double xIntercept = -yIntercept / slope; // find x intercept based off y = mx+b
			if (isnan(xIntercept) == 0) { // check for nan
				xInterceptNeg.push_back(xIntercept); // add value
			}
		}

		// Get median of x intercepts for negative slope lines
		double xIntNegMed = median(xInterceptNeg);

		// Define vector storing 'good' x intercept values, same concept as the slope calculations before
		vector<double> xIntNegGood;
		double xIntSumNeg = 0; // for finding avg

							   // Now that we got median, loop through lines again and compare values against median
		for (size_t i = 0; i != slopeNegativeLines.size(); ++i) {
			double x1 = slopeNegativeLines[i][0]; // x value
			double y1 = image.rows - slopeNegativeLines[i][1]; // y value...yaxis is flipped
			double slope = slopeNegativeLines[i][4];
			double yIntercept = y1 - slope*x1; // yintercept of line
			double xIntercept = -yIntercept / slope; // find x intercept based off y = mx+b

													 // check for nan and check if it's close enough to the median
			if (isnan(xIntercept) == 0 && abs(xIntercept - xIntNegMed) < .35*xIntNegMed) {
				xIntNegGood.push_back(xIntercept); // add to 'good' vector
				xIntSumNeg += xIntercept;
			}
		}

		// Get mean x intercept value for negative slope lines
		double xInterceptNegMean = xIntSumNeg / xIntNegGood.size();
		//gotLines = true;


		//-----------------------PLOT LANE LINES------------------------
		// Need end points of line to draw in. Have x1,y1 (xIntercept,im.shape[1]) where
		// im.shape[1] is the bottom of the image. take y2 as some num (min/max y in the good lines?)
		// then find corresponding x

		// Create image, lane lines on real image
		cv::Mat laneLineImage = image.clone();
		cv::Mat laneFill = image.clone();

		// Positive Slope Line
		float slope = posSlopeMean;
		double x1 = xInterceptPosMean;
		int y1 = 0;
		double y2 = image.size().height - (image.size().height - image.size().height*.35);
		double x2 = (y2 - y1) / slope + x1;

		// Add positive slope line to image
		x1 = int(x1 + .5);
		x2 = int(x2 + .5);
		y1 = int(y1 + .5);
		y2 = int(y2 + .5);
		cv::line(laneLineImage, cv::Point(x1, image.size().height - y1), cv::Point(x2, image.size().height - y2),
			cv::Scalar(0, 255, 0), 3, 8);


		// Negative Slope Line
		slope = negSlopeMean;
		double x1N = xInterceptNegMean;
		int y1N = 0;
		double x2N = (y2 - y1N) / slope + x1N;

		// Add negative slope line to image
		x1N = int(x1N + .5);
		x2N = int(x2N + .5);
		y1N = int(y1N + .5);
		cv::line(laneLineImage, cv::Point(x1N, image.size().height - y1N), cv::Point(x2N, image.size().height - y2),
			cv::Scalar(0, 255, 0), 3, 8);




		// -----------------BLEND IMAGE-----------------------
		// Use cv::Point type for x,y points
		cv::Point v1 = cv::Point(x1, image.size().height - y1);
		cv::Point v2 = cv::Point(x2, image.size().height - y2);
		cv::Point v3 = cv::Point(x1N, image.size().height - y1N);
		cv::Point v4 = cv::Point(x2N, image.size().height - y2);

		// create vector from array of corner points of lane
		cv::Point verticesBlend[] = { v1,v3,v4,v2 };
		std::vector<cv::Point> verticesVecBlend(verticesBlend, verticesBlend + sizeof(verticesBlend) / sizeof(cv::Point));

		// Create vector of vectors to be used in fillPoly, add the vertices we defined above
		std::vector<std::vector<cv::Point> > verticesfp;
		verticesfp.push_back(verticesVecBlend);

		// Fill area created from vector points
		cv::fillPoly(laneFill, verticesfp, cv::Scalar(0, 255, 255));

		// Blend image
		float opacity = .25;
		cv::Mat blendedIm;
		cv::addWeighted(laneFill, opacity, image, 1 - opacity, 0, blendedIm);

		// Plot lane lines
		length = std::abs(x1N - x1);
		//length = length * lan
	//	length = length*0.00598;
		//length = length * widthfactor;
		//length = ceilf(length * 100) / 100;

		if (length > 0)
		{
			int value_count = 0;
			for (int i = 0; i < previous_lane_width.size(); i++)
			{
				if (previous_lane_width[i] > 0 && length < previous_lane_width[i] * 1.2 && length > previous_lane_width[i] * 0.8)
					value_count++;
			}

			if (value_count == 2)
			{	
				cv::line(image, cv::Point(x1, image.size().height - y1), cv::Point(x1N, image.size().height - y1N),
					cv::Scalar(0, 255, 0), 30, 30);
				//string label = std::to_string(length) + " meter";
				//putText(image, label, Point(x1 + length / 2, image.size().height - y1 - 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 0), 2);	
			}
		}
		
		previous_lane_width.push_back(length);

		if (previous_lane_width.size() > 2)
		{
			previous_lane_width.erase(previous_lane_width.begin());
		}

	} // end if we got more than one line


	  // We do none of that if we don't see enough lines
	else {
		length = 0;
	}
	return length;
}

void __declspec(dllexport) StopEngine()
{
	_is_stop = true;
}

int __declspec(dllexport) Run(std::string filename)
{
	set_count();
	VideoCapture cap;
	//if (filename == filename)
	//{
	//	if (!cap.open(filename))
	//	{
	//		cout << "cann't open " << filename << " file." << endl;
	//		return 0;
	//	}
	//}
	//else
	//{
		if (!cap.open(1))
		{
			cout << "cann't open " << filename << " file." << endl;

			return 0;
		}
	/*}*/
		
	Mat frame, rawFrame;
	_is_stop = false;
	if (!cap.read(frame))
		return 0;

	_frame_channels = frame.channels();
	_frame_width = frame.cols;
	_frame_height = frame.rows;

	WaitEvent(_mutex_current_buffer);
	_current_buffer = (unsigned char*)malloc(_frame_channels * _frame_width * _frame_height);
	SendEvent(_mutex_current_buffer);

	//namedWindow("Track", CV_WINDOW_NORMAL);

	char buf[64];
	std::vector<std::string> classes = getClasses();
	while (true) {
		// Capture frame-by-frame
		cap >> rawFrame;
		// If the frame is empty, break immediately
		if (rawFrame.empty())
			break;

		Mat frame2;
		
		frame2 = rawFrame;
		int lane_length = LaneDetection(frame2);

	

		Mat result = object_detection(frame2);
		if (lane_length < 0 || lane_length > 1200)
			lane_length = 0;
		
		
		cv::line(frame2, cv::Point(0, frame2.rows * 3 / 4 ), cv::Point(frame2.cols, frame2.rows * 3 / 4),
			cv::Scalar(0, 255, 0), 1, 1);

		int object_count = get_count();
		putText(frame2, "Count = " + to_string(object_count), Point(50, 50), FONT_HERSHEY_SIMPLEX, .5, Scalar(255, 0, 0), 2);
		//MainWndow::imgPreview.Source = Bitmap2BitmapImage(bitmap);
		//GlobalObjectCounter = object_count;
		//imgPreview_2.Source = imgPreview.Source;
		//MainWindow.user = "hello";

		std::string res = "lane_length:" + std::to_string(lane_length) + "\n";
		
		res += getResult();
		memset(buf, 0, 64);
		WaitEvent(_mutex_current_buffer);
		memcpy(_current_buffer, result.data, _frame_channels * _frame_width * _frame_height);
		SendEvent(_mutex_current_buffer);

		if (_onFrame != NULL)
			_onFrame(res);
		waitKey(1);

		if (_is_stop)
			break;
	}

	cap.release();

	return 0;
}

