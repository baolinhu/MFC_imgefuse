#include<iostream>
#include <math.h>
#include<opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/core/core.hpp>
#include"Shlobj.h"
#include<vector>
#include<string>
using namespace std;
using namespace cv;
const double eps = 1e-6;
class ImageFuse
{
private:
	Mat_<float> ir;
	Mat_<float> vs;
	int levels;
	int um;
	vector<Mat_<float> > irPyr, vsPyr, resultPyr;//Laplacian Pyramids  
	Mat irHighestLevel, vsHighestLevel, resultHighestLevel;
	//lap
	void buildLapPyramids();
	void buildLaplacianPyramid(const Mat& img, vector<Mat_<float> >& lapPyr, Mat& HighestLevel);
	Mat_<float> reconstructImgFromLapPyramid();
	//ratio
	void buildRatPyramids();
	void buildRatioPyramid(const Mat& img, vector<Mat_<float> >& RatPyr, Mat& HighestLevel);
	Mat_<float> reconstructImgFromRatPyramid();
	//contrast
	void buildConPyramids();
	void buildContrastPyramid(const Mat& img, vector<Mat_<float> >& RatPyr, Mat& HighestLevel);
	Mat_<float> reconstructImgFromConPyramid();

	void blendPyrs();

public:
	ImageFuse(const Mat_<float>& _ir, const Mat_<float>& _vs, int _levels=5,int _um=3) ://construct function, used in LaplacianBlending lb(l,r,m,4);  
		ir(_ir), vs(_vs), levels(_levels), um(_um)
	{
		assert(_ir.size() == _vs.size());
	};
	Mat_<float> lapFuse();
	Mat_<float> ratFuse();
	Mat_<float> conFuse();
private:
	Mat borderExpand(const Mat m1, int x, int flag = 3);
	Mat_<float> localMatch(const Mat_<float> &M1, const Mat_<float> &M2, int um = 3);
public:
	//fuse_SIDWT, Wavelet is Haar
	Mat_<float> fuse_SIDWT();
	//image fusion with DWT, Wavelet is DBSS(2, 2)
	Mat_<float> fuse_DWT();
	//image fusion with gradient pyramid
	Mat_<float> fuse_gra();
	//image fusion with fsd pyramid
	Mat_<float> fuse_fsd();
	//image fusion with pca
	Mat_<float> fuse_pca();
	//加权平均融合
	Mat_<float> fuse_ave(const double weight = 0.5);
};