#pragma once
#include <string.h>
#include "opencv2\core.hpp"


typedef struct {
	std::string label;
	int start_position;
}ocr_char;

int init_roadsign_dnn(std::string &names, std::string &config, std::string &model);
int init_level_dnn(std::string &names, std::string &config, std::string &model);

cv::Mat object_detection(cv::Mat frame);
std::vector<std::string> getClasses();
std::vector<std::string> get_level_classes();
int get_count();

std::string getResult();
void set_count();


