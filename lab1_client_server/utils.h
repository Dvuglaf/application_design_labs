#ifndef UTILS
#define UTILS

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <regex>


namespace my_utils {

	/**
	 * Read image from file.
	 *
	 * @param image_path: path to image
	 * 
	 * @return matrix with uint8_t values (0..255)
	 */
	cv::Mat read_image(const std::string& image_path);

	/**
	 * Discrete Fourier Transform on one-channel matrix.
	 *
	 * @param source: input one-channel matrix with uint8_t values (0..255)
	 * 
	 * @return array of two matrices: real part and imag part
	 */
	std::vector<cv::Mat> dft(const cv::Mat& source);

	/**
	 * Inverse Discrete Fourier Transform on two-channel matrix.
	 *
	 * @param source: input two-channel matrix: real and imag part
	 * 
	 * @return one-channel matrix with uint8_t values (0..255)
	 */
	cv::Mat idft(const cv::Mat& source);

	/**
	 * Split string by delimiter.
	 *
	 * @param string: input string
	 * @param delimiter: delimiter
	 * 
	 * @return array of substrings, splitted by delimiter
	 */
	std::vector<std::string> split(const std::string& string, const std::string& delimiter);

	/**
	 * Convert string to cv::Mat class matrix with double values.
	 *
	 * @param string: input string
	 * @param size: output matrix size
	 * 
	 * @return cv::Mat class matrix with double values (64 bytes)
	 */
	cv::Mat mat_from_string(const std::string& string, const cv::MatSize& size);

	/**
	 * Convert cv::Mat class matrix to string.
	 *
	 * @param matrix: input cv::Mat class matrix
	 *
	 * @return string
	 */
	std::string mat_to_string(const cv::Mat& matrix);

};
#endif 