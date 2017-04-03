#include <stdio.h>
#include <opencv\cv.h>
#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <array>
#include "PerlinNoise.h"

using namespace cv;
using namespace std;

Point prevPt(-1, -1);

void applySobelXY(Mat &baseImage, Mat &sobelFinal) {
	//Sobel kernels
	float arrKernelX[3][3] = { { -1,0,1 },{ -2,0,2 },{ -1,0,1 } };
	float arrKernelY[3][3] = { { 1,2,1 },{ 0,0,0 },{ -1,-2,-1 } };

	//Applying sobel and getting the final sobel magnitude
	Mat kernelSobel = Mat(3, 3, CV_32F, arrKernelX), sobelX, sobelY;
	filter2D(baseImage, sobelX, -1, kernelSobel);

	kernelSobel = Mat(3, 3, CV_32F, arrKernelY);
	filter2D(baseImage, sobelY, -1, kernelSobel);

	sobelFinal.convertTo(sobelFinal, CV_32F); //Need 32F to do the power and sqrt operations
	sobelX.convertTo(sobelX, CV_32F);
	sobelY.convertTo(sobelY, CV_32F);

	pow(sobelX, 2, sobelX);
	pow(sobelY, 2, sobelY);
	sobelFinal = sobelX + sobelY;
	sqrt(sobelFinal, sobelFinal); //Final sobel magniture
}

void calculateSDMean(vector<unsigned char> data, float &SD, float &mean)
{
	float sum = 0.0, standardDeviation = 0.0;

	int i;

	for (i = 0; i < data.size(); ++i)
	{
		sum += data[i];
	}

	mean = sum / data.size();

	for (i = 0; i < data.size(); ++i)
		standardDeviation += pow(data[i] - mean, 2);

	SD = sqrt(standardDeviation / data.size());
}

map<int, std::vector<std::array<int, 2>>> get_watershed_groups(Mat thresholdImage, Mat baseImage) {
	int i, j, compCount = 0;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	map<int, std::vector<std::array<int, 2>>> points_per_segment;

	findContours(thresholdImage, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	if (contours.empty())
		return map<int, std::vector<std::array<int, 2>>>();

	Mat markers(thresholdImage.size(), CV_32S);
	markers = Scalar::all(0);
	int idx = 0;
	for (; idx >= 0; idx = hierarchy[idx][0], compCount++)
		drawContours(markers, contours, idx, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

	vector<Vec3b> colorTab;
	for (i = 0; i < compCount; i++)
	{
		int b = 0;//theRNG().uniform(0, 255);
		int g = 0;//theRNG().uniform(0, 255);
		int r = 0;//theRNG().uniform(0, 255);

		colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
	}

	double t = (double)getTickCount();
	watershed(baseImage, markers);
	t = (double)getTickCount() - t;
	printf("execution time = %gms\n", t*1000. / getTickFrequency());

	//Mat calmesMask(markers.size(), CV_8UC3);

	// paint the watershed image
	for (i = 0; i < markers.rows; i++)
		for (j = 0; j < markers.cols; j++)
		{
			int index = markers.at<int>(i, j);
			points_per_segment[index].push_back({ i, j });
			/*if (index == -1)
				calmesMask.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
			else if (index <= 0 || index > compCount)
				calmesMask.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
			else
				calmesMask.at<Vec3b>(i, j) = colorTab[index - 1];*/
		}

	//imshow(to_string(getTickCount()), calmesMask);
	return points_per_segment;
}

map<int, std::vector<std::array<int, 2>>> get_watershed_groups(Mat thresholdImage, Mat baseImage, Mat &calmesMask) {
	int i, j, compCount = 0;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	map<int, std::vector<std::array<int, 2>>> points_per_segment;

	findContours(thresholdImage, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

	if (contours.empty())
		return map<int, std::vector<std::array<int, 2>>>();

	Mat markers(thresholdImage.size(), CV_32S);
	markers = Scalar::all(0);
	int idx = 0;
	for (; idx >= 0; idx = hierarchy[idx][0], compCount++)
		drawContours(markers, contours, idx, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

	vector<Vec3b> colorTab;
	for (i = 0; i < compCount; i++)
	{
		int b = 0;//theRNG().uniform(0, 255);
		int g = 0;//theRNG().uniform(0, 255);
		int r = 0;//theRNG().uniform(0, 255);

		colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
	}

	double t = (double)getTickCount();
	watershed(baseImage, markers);
	t = (double)getTickCount() - t;
	printf("execution time = %gms\n", t*1000. / getTickFrequency());

	calmesMask = Mat(markers.size(), CV_8UC3);

	// paint the watershed image
	for (i = 0; i < markers.rows; i++)
		for (j = 0; j < markers.cols; j++)
		{
			int index = markers.at<int>(i, j);
			points_per_segment[index].push_back({ i, j });
			if (index == -1)
				calmesMask.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
			else if (index <= 0 || index > compCount)
				calmesMask.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
			else
				calmesMask.at<Vec3b>(i, j) = colorTab[index - 1];
		}

	Mat element = getStructuringElement(MORPH_RECT, Size(9, 9));
	dilate(calmesMask, calmesMask, element);
	//imshow(to_string(getTickCount()), calmesMask);
	//calmesMask = calmesMask*0.5 + baseImage*0.5;
	//imshow(to_string(getTickCount()), calmesMask);
	return points_per_segment;
}

int main(int argc, char** argv)
{
	string filename = "flor.jpg";
Reset:
	double threshFactor = 0.38;
	bool inverse = true;
	int autoTechnique = 0;
	namedWindow("Result", 1);
Start: //I'm sorry, I'm truly sorry!
	string techniqueName = "";
	switch (autoTechnique)
	{
	case 0:
		techniqueName = "None";
		break;
	case 8:
		techniqueName = "Otsu";
		break;
	case 16:
		techniqueName = "Triangle";
		break;

	}
	setWindowTitle("Result", "Result - Technique: " + techniqueName + " - Threshold: " + to_string(threshFactor) + " - Threshold Type: " + (inverse ? "Inverse" : "Regular"));
	Mat img0 = imread(filename, 1), imgGray, imgThresh, imgBase, imgGlass = imread("vitral.jpg", 1), imgGlassThres, calmesMask, imgZinc = imread("zinc.jpg", 1);
	//Resizing the metal image to match the image
	resize(imgZinc, imgZinc, img0.size(), 0, 0, InterpolationFlags::INTER_NEAREST);


	if (img0.empty())
	{
		cout << "Image not found!";
		return 0;
	}

	auto technique = inverse ? THRESH_BINARY_INV + autoTechnique : THRESH_BINARY + autoTechnique;

	//Get a grayscale copy of the img
	img0.copyTo(imgBase);
	cvtColor(imgBase, imgThresh, COLOR_BGR2GRAY);
	cvtColor(imgThresh, imgGray, COLOR_GRAY2BGR);

	threshold(imgThresh, imgThresh, threshFactor * 255, 255, technique);

	auto kernel = Mat::ones(3, 3, CV_32F);
	morphologyEx(imgThresh, imgThresh, MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
	morphologyEx(imgThresh, imgThresh, MORPH_OPEN, kernel, cv::Point(-1, -1), 10);

	//Get a grayscale copy of stained glass image
	cvtColor(imgGlass, imgGlassThres, COLOR_BGR2GRAY);
	threshold(imgGlassThres, imgGlassThres, 0.1 * 255, 255, THRESH_BINARY); //hand tuned parameters

	//Get points of watershed-determined pieces
	map<int, std::vector<std::array<int, 2>>> pieces_vitral;
	map<int, std::vector<std::array<int, 2>>> pieces_image;

	pieces_vitral = get_watershed_groups(imgGlassThres, imgGlass);
	pieces_image = get_watershed_groups(imgThresh, imgBase, calmesMask);

	assert(!pieces_vitral.empty() && !pieces_image.empty());

	cv::Mat perlin_noise_img = CreatePerlinNoiseImage(imgBase.size(), 3.0 / 255.0);
	cvtColor(perlin_noise_img, perlin_noise_img, COLOR_GRAY2BGR);
	imgBase = imgBase*0.8 + perlin_noise_img*0.2;

	//Calculate the average color of each piece
	map<int, Vec3f> image_average_colors;
	map<int, Vec3f> vitral_average_colors;

	//Converting to Lab, because it allows us to transform without colors bleeding
	//additionally, euclidean distance for color works on Lab because its homogeneous

	cvtColor(imgBase, imgBase, COLOR_BGR2Lab);
	cvtColor(imgGlass, imgGlass, COLOR_BGR2Lab);

	for (auto piece : pieces_image) {
		Vec3f averageColor;
		for (auto point : piece.second) {
			averageColor += imgBase.at<Vec3b>(point[0], point[1]);
		}
		averageColor = averageColor / (float)piece.second.size();
		image_average_colors[piece.first] = averageColor;
		//cout << averageColor << endl;
	}

	for (auto piece : pieces_vitral) {
		Vec3f averageColor;
		for (auto point : piece.second) {
			averageColor += imgGlass.at<Vec3b>(point[0], point[1]);
		}
		averageColor = averageColor / (float)piece.second.size();
		vitral_average_colors[piece.first] = averageColor;
	}

	//maps image piece:stained glass piece
	map<int, int> matching_pieces;

	//Piece Correspondence
	for (auto piece : image_average_colors) {
		double closestDist = std::numeric_limits<double>().max();
		for (auto piece_vitral : vitral_average_colors) {
			auto distance = norm(piece.second, piece_vitral.second, NORM_L2);
			if (distance < closestDist) {
				matching_pieces[piece.first] = piece_vitral.first;
				closestDist = distance;
			}
		}
	}

	Mat imgCoerced;
	imgBase.copyTo(imgCoerced);

	//Color Coercing
	for (auto piece : pieces_image) {
		//Made this int to handle possible negative values when coercing color
		vector<unsigned char> L, A, B;
		vector<unsigned char> L_V, A_V, B_V;

		float meanL, meanA, meanB;
		float meanL_V, meanA_V, meanB_V;
		float sdL, sdA, sdB;
		float sdL_V, sdA_V, sdB_V;

		for (auto point : piece.second) {
			L.push_back(imgBase.at<Vec3b>(point[0], point[1])[0]);
			A.push_back(imgBase.at<Vec3b>(point[0], point[1])[1]);
			B.push_back(imgBase.at<Vec3b>(point[0], point[1])[2]);
		}

		for (auto point : pieces_vitral[matching_pieces[piece.first]]) {
			L_V.push_back(imgGlass.at<Vec3b>(point[0], point[1])[0]);
			A_V.push_back(imgGlass.at<Vec3b>(point[0], point[1])[1]);
			B_V.push_back(imgGlass.at<Vec3b>(point[0], point[1])[2]);
		}

		calculateSDMean(L, sdL, meanL);
		calculateSDMean(A, sdA, meanA);
		calculateSDMean(B, sdB, meanB);
		calculateSDMean(L_V, sdL_V, meanL_V);
		calculateSDMean(A_V, sdA_V, meanA_V);
		calculateSDMean(B_V, sdB_V, meanB_V);

		for (auto point : piece.second) {
			auto currPixel = imgCoerced.at<Vec3b>(point[0], point[1]);
			int l = currPixel.val[0];
			int a = currPixel.val[1];
			int b = currPixel.val[2];

			l -= meanL;
			l = /*(sdL_V / sdL) **/ l + meanL_V;

			a -= meanA;
			a = (sdA_V / sdA) * a + meanA_V;

			b -= meanB;
			b = (sdB_V / sdB) * b + meanB_V;

			currPixel = Vec3b(l, a, b);

			imgCoerced.at<Vec3b>(point[0], point[1]) = currPixel;
		}
	}

	//Convert images back to BGR
	cvtColor(imgCoerced, imgCoerced, COLOR_Lab2BGR);
	cvtColor(imgBase, imgBase, COLOR_Lab2BGR);

	//Simplified synthesis of calmes, however, unlike the paper, I add the calmes after the glass-filter
	//I also changed the way we generate the calmes, mine is simpler and kinda works.
	Mat colorCalmes, blurredCalmesMask;
	blur(calmesMask, blurredCalmesMask, Size(9, 9));//Lets blur the calmes to get some soft edges later
	imgZinc.copyTo(colorCalmes, calmesMask);
	Mat softEdges = blurredCalmesMask - calmesMask; //softedges

	//Smooth out and emboss the calmes
	float embossKernel[3][3] = { { -2,-1,0 },{ -1,1,1},{ 0,1,2 } };
	GaussianBlur(colorCalmes, colorCalmes, Size(7, 7), 0, 0);
	Mat kernelSobel = Mat(3, 3, CV_32F, embossKernel);
	filter2D(colorCalmes, colorCalmes, -1, kernelSobel); //applying emboss to the calmes

	//Applying sobel and blurring the result
	Mat sobelFinal;
	applySobelXY(imgCoerced, sobelFinal);
	GaussianBlur(sobelFinal, sobelFinal, Size(9, 9), 3, 3);

	//Getting grayscale version of the sobel image, but with 3 channels
	sobelFinal.convertTo(sobelFinal, CV_8UC1); //Going back to uchar color values
	cvtColor(sobelFinal, sobelFinal, COLOR_BGR2GRAY);
	cvtColor(sobelFinal, sobelFinal, COLOR_GRAY2BGR);


	colorCalmes.copyTo(imgCoerced, calmesMask);

	imgCoerced = imgCoerced + sobelFinal;
	imgCoerced = imgCoerced - softEdges * 0.5;
	//imshow("Calmes Mask", calmesMask);
	//imshow("Sobel", sobelFinal);
	imshow("Base Image", img0);
	imshow("Result", imgCoerced);

	for (;;)
	{
		char c = (char)waitKey(0);

		if (c == 27)
			break;
		if (c == '1') {
			filename = "flor.jpg";
			threshFactor = 0.38;
			autoTechnique = 0;
			inverse = true;
			goto Start;//*cries in C++*
		}
		if (c == '2') {
			filename = "flor3.jpg";
			threshFactor = 0.62;
			autoTechnique = 0;
			inverse = true;
			goto Start;
		}
		if (c == '3') {
			filename = "flor2.jpg";
			threshFactor = 0.62;
			autoTechnique = 0;
			inverse = false;
			goto Start;
		}
		if (c == '4') {
			filename = "arara.jpg";
			threshFactor = 0.38;
			autoTechnique = 0;
			inverse = true;
			goto Start;
		}
		if (c == '5') {
			filename = "japan.jpg";
			threshFactor = 0.68;
			autoTechnique = 0;
			inverse = false;
			goto Start;
		}
		if (c == '6') {
			filename = "girl.jpg";
			threshFactor = 0.66;
			autoTechnique = 0;
			inverse = false;
			goto Start;
		}
		if (c == '7') {
			filename = "rock.jpg";
			threshFactor = 0.8;
			autoTechnique = 0;
			inverse = true;
			goto Start;
		}
		if (c == '8') {
			filename = "asians.jpg";
			threshFactor = 0.44;
			autoTechnique = 0;
			inverse = true;
			goto Start;
		}
		if (c == 'o') {
			cin >> filename;
			threshFactor = 0.3;
			autoTechnique = 0;
			inverse = true;
			goto Start;
		}
		if (c == 'i') {
			inverse = !inverse;
			goto Start;
		}
		if (c == '=') {
			threshFactor += 0.02;
			goto Start;
		}
		if (c == '-') {
			threshFactor -= 0.02;
			goto Start;
		}
		if (c == 'r') {
			goto Reset;
		}
		if (c == 't') {
			switch (autoTechnique) {
			case 0:
				autoTechnique = 8;
				goto Start;
			case 8:
				autoTechnique = 16;
				goto Start;
			case 16:
				autoTechnique = 0;
				goto Start;
			}
		}
	}

	return 0;
}