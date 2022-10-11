#include "utils.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <regex>


cv::Mat read_image(const std::string& image_path) {
	cv::Mat image = cv::imread(cv::samples::findFile(image_path), cv::IMREAD_COLOR);
	if (image.empty())
		throw std::invalid_argument("Could not read the image");

	return image;
}

std::vector<cv::Mat> dft(const cv::Mat& source) {
	cv::Mat float_source;
	source.convertTo(float_source, CV_32FC1, 1. / 255.);

	cv::Mat complex_source[2] = { float_source, cv::Mat::zeros(float_source.size(), CV_32F) };
	cv::Mat dft_input, dft_output;

	merge(complex_source, 2, dft_input); //for cv::dft input (two-channel matrix instead of two matrices)

	cv::dft(dft_input, dft_output, cv::DFT_COMPLEX_OUTPUT);

	std::vector<cv::Mat> dft(2);
	split(dft_output, dft);

	return dft;
}

cv::Mat idft(const cv::Mat& source) {
	cv::Mat idft_output;

	dft(source, idft_output, cv::DFT_SCALE | cv::DFT_INVERSE | cv::DFT_REAL_OUTPUT);

	cv::Mat uint8_idft_output;
	idft_output.convertTo(uint8_idft_output, CV_8U, 255);

	return uint8_idft_output;
}

std::vector<std::string> split(const std::string& string, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t previous = 0, position = 0;
	do {
		position = string.find(delimiter, previous);
		if (position == std::string::npos) position = string.length();
		std::string token = string.substr(previous, position - previous);
		if (!token.empty()) tokens.push_back(token);
		previous = position + delimiter.length();
	} while (position < string.length() && previous < string.length());
	return tokens;
}

cv::Mat mat_from_string(const std::string& string, const cv::MatSize& size) {
	std::vector<double> vec;

	auto edit_string = std::regex_replace(string, std::regex(";"), ",");  //; -> ,
	edit_string = std::regex_replace(edit_string, std::regex("[^\\d,.-]"), "");  // delete all characters except digits, ',' '.' '-'

	auto splitted = split(edit_string, ",");

	vec.reserve(splitted.size());

	for (int i = 0; i < splitted.size(); ++i)
		vec.push_back(std::stod(splitted[i]));

	cv::Mat mat = cv::Mat::zeros(size[0], size[1], CV_64F);

	for (size_t i = 0; i < size[0]; ++i)
		for (size_t j = 0; j < size[1]; ++j)
			mat.at<double>(i, j) = vec[j + i * 224];

	return mat;
}

std::string mat_to_string(const cv::Mat& matrix) {
	std::string result;
	result << matrix;
	return result;
}
