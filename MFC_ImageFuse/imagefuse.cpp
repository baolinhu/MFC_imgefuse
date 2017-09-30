#include"stdafx.h"
#include"imagefuse.h"


//工具函数

//string TCHAR2STRING(TCHAR *STR)
//{
//	int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
//	char* chRtn = new char[iLen*sizeof(char)];
//	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
//	std::string str(chRtn);
//	return str;
//}
enum ConvolutionType {
	/* Return the full convolution, including border */
	CONVOLUTION_FULL = 0,

	/* Return only the part that corresponds to the original image */
	CONVOLUTION_SAME,
	/* Return only the submatrix containing elements that were not influenced by the border */
	CONVOLUTION_VALID
};
//MATLAB的卷积函数
Mat conv2(const Mat &img, const Mat& ikernel, ConvolutionType type=CONVOLUTION_FULL)
{
	Mat dest;
	Mat kernel;
	flip(ikernel, kernel, -1);
	Mat source = img;
	if (CONVOLUTION_FULL == type)
	{
		source = Mat();
		const int additionalRows = kernel.rows - 1, additionalCols = kernel.cols - 1;
		copyMakeBorder(img, source, (additionalRows + 1) / 2, additionalRows / 2, (additionalCols + 1) / 2, additionalCols / 2, BORDER_CONSTANT, Scalar(0));
	}
	Point anchor(kernel.cols - kernel.cols / 2 - 1, kernel.rows - kernel.rows / 2 - 1);
	int borderMode = BORDER_CONSTANT;
	filter2D(source, dest, img.depth(), kernel, anchor, 0, borderMode);

	if (CONVOLUTION_VALID == type)
	{
		dest = dest.colRange((kernel.cols - 1) / 2, dest.cols - kernel.cols / 2).rowRange((kernel.rows - 1) / 2, dest.rows - kernel.rows / 2);
	}
	return dest;
}
//按行或者列下采样，1-行，2-列
Mat_<float>  downSamp(const Mat_<float> &M, int flag = 3)
{
	int w = M.cols;
	int h = M.rows;
	Mat_<float>  ret;
	//if (flag == 1)
	//{
	//	resize(M, ret, Size(w, (h+1) / 2));//行下采样
	//}
	//else if (flag==2)
	//{
	//	resize(M, ret, Size((w+1) / 2, h));
	//}
	if (flag == 1)
	{
		ret = Mat(Size(w, (h + 1) / 2), CV_32FC1);
		//cout << ret.size() << endl;
		for (int i = 0; i < (h + 1) / 2; ++i)
		{
			ret.row(i) = M.row(i * 2).clone() + 0.0;
			//cout << ret.row(i) << endl;
		}
		//resize(M, ret, Size(w, h * 2));//行上采样
	}
	else if (flag == 2)
	{
		ret = Mat(Size((w + 1) / 2, h), CV_32FC1);
		for (int i = 0; i < (w + 1) / 2; ++i)
		{
			ret.col(i) = M.col(i * 2).clone() + 0.0;
		}
	}
	else
	{
		pyrDown(M, ret);
	}
	return ret;
}
//按行或者列上采样，1-行，2-列
Mat_<float> upSamp(const Mat_<float> &M, int flag = 3)
{
	int w = M.cols;
	int h = M.rows;
	Mat_<float> ret;
	if (flag == 1)
	{
		ret = Mat::zeros(Size(w, h * 2), CV_32FC1);
		//cout << ret.size() << endl;
		for (int i = 0; i < h * 2; ++i, ++i)
		{
			ret.row(i) = M.row(i / 2).clone() + 0.0;
			//cout << ret.row(i) << endl;
		}
		//resize(M, ret, Size(w, h * 2));//行上采样
	}
	else if (flag == 2)
	{
		ret = Mat::zeros(Size(w * 2, h), CV_32FC1);
		for (int i = 0; i < w * 2; ++i, ++i)
		{
			ret.col(i) = M.col(i / 2).clone() + 0.0;
		}
	}
	else
	{
		pyrUp(M, ret);
	}
	return ret;
}
//求协方差矩阵
Mat_<float> fuse_cov(const Mat_<float>& M)
{
	Mat data = M.clone();
	//cout << "data:" << endl << data << endl;
	Mat means;
	reduce(data, means, 0, CV_REDUCE_AVG);
	cout << "means:" << endl << means << endl;
	Mat tmp = repeat(means, data.rows, 1);
	data = data - tmp;    //源数据减去均值
	Mat covar = (data.t()*data) / (data.rows - 1);   // （X'*X)/n-1
	cout << "covar:" << endl << covar << endl;
	return covar;
}

//成员函数
void ImageFuse::buildLapPyramids() {
	buildLaplacianPyramid(ir, irPyr, irHighestLevel);
	buildLaplacianPyramid(vs, vsPyr, vsHighestLevel);
	//buildGaussianPyramid();
}
void ImageFuse::buildLaplacianPyramid(const Mat& img, vector<Mat_<float> >& lapPyr, Mat& HighestLevel) {
	lapPyr.clear();
	Mat currentImg = img;
	for (int l = 0; l<levels; l++) {
		Mat down, up;
		pyrDown(currentImg, down);
		pyrUp(down, up, currentImg.size());
		Mat lap = currentImg - up;
		lapPyr.push_back(lap);
		currentImg = down;
	}
	currentImg.copyTo(HighestLevel);
}

Mat_<float> ImageFuse::reconstructImgFromLapPyramid() {
	//将左右laplacian图像拼成的resultLapPyr金字塔中每一层  
	//从上到下插值放大并相加，即得blend图像结果  
	Mat currentImg = resultHighestLevel;
	for (int l = levels - 1; l >= 0; l--) {
		Mat up;

		pyrUp(currentImg, up, resultPyr[l].size());
		currentImg = up + resultPyr[l];
	}
	return currentImg;
}

void ImageFuse::blendPyrs() {
	//获得每层金字塔中直接用左右两图Laplacian变换拼成的图像resultLapPyr  
	//mul对应位相乘
	//resultHighestLevel = irHighestLevel.mul(maskGaussianPyramid.back()) +
	//	vsHighestLevel.mul(Scalar(1.0, 1.0, 1.0) - maskGaussianPyramid.back());
	resultHighestLevel = (irHighestLevel + vsHighestLevel) / 2;
	for (int l = 0; l<levels; l++) {
		Mat A = irPyr[l];
		//Mat antiMask = Scalar(1.0, 1.0, 1.0) - maskGaussianPyramid[l];
		Mat B = vsPyr[l];
		Mat_<float> blendedLevel = localMatch(A, B, 3);

		resultPyr.push_back(blendedLevel);
	}
}
Mat_<float> ImageFuse::lapFuse() {
	buildLapPyramids();  //construct Laplacian Pyramid and Gaussian Pyramid  
	blendPyrs();   //blend ir & vs Pyramids into one Pyramid  
	return reconstructImgFromLapPyramid();//reconstruct Image from Laplacian Pyramid  
}
Mat ImageFuse::borderExpand(const Mat m1, int x, int flag )
{
	Mat ret;
	if (flag == 1)
		copyMakeBorder(m1, ret, 0, 0, x, x, IPL_BORDER_REFLECT_101);//反向对称 的扩展方式
	else if (flag == 2)
		copyMakeBorder(m1, ret, x, x, 0, 0, IPL_BORDER_REFLECT_101);
	else
		copyMakeBorder(m1, ret, x, x, x, x, IPL_BORDER_REFLECT_101);
	return ret;
}
Mat_<float> ImageFuse::localMatch(const Mat_<float> &M1, const Mat_<float> &M2, int um )
{
	double th = 0.75;
	Mat_<float> S1, S2, Ma, ret;
	double eps = 1e-6;
	Mat_<float> km(um, um, 1.0);
	//% compute salience 基于区域显著性度量
	S1 = conv2(borderExpand(M1.mul(M1), um / 2), km, CONVOLUTION_VALID);
	S2 = conv2(borderExpand(M2.mul(M2), um / 2), km, CONVOLUTION_VALID);
	//% compute match 局部匹配度量
	Ma = conv2(borderExpand(M1.mul(M2), um / 2), km, CONVOLUTION_VALID);
	Ma = 2 * Ma / (S1 + S2 + eps);
	Mat_<float> m1, m2, w1;
	m1 = (Ma > th) / 255;
	m2 = (S1 > S2) / 255;
	//cout << m1 << endl;
	w1 = 0.5 - 0.5*(1 - Ma) / (1 - th);
	ret = (1 - m1).mul((m2.mul(M1)) + ((1 - m2).mul(M2)));
	ret = ret + (m1.mul(((m2.mul(M1).mul(1 - w1))) + (m2.mul(M2).mul(w1)) + ((1 - m2).mul(M2).mul(1 - w1)) + ((1 - m2).mul(M1).mul(w1))));
	return ret;
}
void ImageFuse::buildRatPyramids() {
	buildRatioPyramid(ir, irPyr, irHighestLevel);
	buildRatioPyramid(vs, vsPyr, vsHighestLevel);
}
void ImageFuse::buildRatioPyramid(const Mat& img, vector<Mat_<float> >& RatPyr, Mat& HighestLevel) {
	RatPyr.clear();
	Mat currentImg = img;
	for (int l = 0; l<levels; l++) {
		Mat down, up;
		pyrDown(currentImg, down);
		pyrUp(down, up, currentImg.size());
		Mat Rat = currentImg/(up+eps);	//ratio
		//Mat Rat = currentImg / (up + eps) - 1;		//contrast
		RatPyr.push_back(Rat);
		currentImg = down;
	}
	currentImg.copyTo(HighestLevel);
}
Mat_<float> ImageFuse::reconstructImgFromRatPyramid() {
	//将左右Ratio图像拼成的resultRatPyr金字塔中每一层  
	//从上到下插值放大并相加，即得blend图像结果  
	Mat currentImg = resultHighestLevel;
	for (int l = levels - 1; l >= 0; l--) {
		Mat up;
		pyrUp(currentImg, up, resultPyr[l].size());

		currentImg = (up +eps).mul(resultPyr[l]);	//ratio
		//currentImg = (up + eps).mul(resultPyr[l] + 1);	//contrast
	}
	return currentImg;
}
Mat_<float> ImageFuse::ratFuse() {
	buildRatPyramids();  //construct  Pyramid  
	blendPyrs();   //blend ir & vs Pyramids into one Pyramid  
	return reconstructImgFromRatPyramid();//reconstruct Image from  Pyramid  
}
void ImageFuse::buildConPyramids() {
	buildContrastPyramid(ir, irPyr, irHighestLevel);
	buildContrastPyramid(vs, vsPyr, vsHighestLevel);
}
void ImageFuse::buildContrastPyramid(const Mat& img, vector<Mat_<float> >& RatPyr, Mat& HighestLevel) {
	RatPyr.clear();
	Mat currentImg = img;
	for (int l = 0; l<levels; l++) {
		Mat down, up;
		pyrDown(currentImg, down);
		pyrUp(down, up, currentImg.size());
		//Mat Rat = currentImg/(up+eps);	//ratio
		Mat Rat = currentImg / (up + eps) - 1;		//contrast
		RatPyr.push_back(Rat);
		currentImg = down;
	}
	currentImg.copyTo(HighestLevel);
}
Mat_<float> ImageFuse::reconstructImgFromConPyramid() {
	//将左右Ratio图像拼成的resultRatPyr金字塔中每一层  
	//从上到下插值放大并相加，即得blend图像结果  
	Mat currentImg = resultHighestLevel;
	for (int l = levels - 1; l >= 0; l--) {
		Mat up;
		pyrUp(currentImg, up, resultPyr[l].size());

		//currentImg = (up +eps).mul(resultPyr[l]);	//ratio
		currentImg = (up + eps).mul(resultPyr[l] + 1);	//contrast
	}
	return currentImg;
}
Mat_<float> ImageFuse::conFuse() {
	buildConPyramids();  //construct  Pyramid 
	blendPyrs();   //blend ir & vs Pyramids into one Pyramid  
	return reconstructImgFromConPyramid();//reconstruct Image from  Pyramid  
}
//fuse_SIDWT, Wavelet is Haar
Mat_<float> ImageFuse::fuse_SIDWT()
{
	Mat_<float> M1 = ir.clone();
	Mat_<float> M2 = vs.clone();
	assert(M1.size() == M2.size());
	Mat_<float> ret;
	vector<vector<Mat_<float>>> coe;
	vector<Mat_<float>>  kerh1, kerg1;

	//loop over decomposition depth->analysis
	for (int i = 1; i <= levels; ++i)
	{
		vector<Mat_<float>> temp;
		//define actual filters(inserting zeros between coefficients)

		int fh = static_cast<int>(pow(2, i) + 1);
		Mat  zeromat(Size(fh, 1), CV_32FC1, Scalar(0));
		Mat_<float> h1, g1;

		if (i == 1)
		{
			zeromat.at <float>(0, 0) = 0.5;
			zeromat.at <float>(0, 1) = 0.5;
			h1 = zeromat.clone();
			zeromat.at <float>(0, 1) = -0.5;
			g1 = zeromat.clone();
		}
		else
		{
			int first = static_cast<int>(pow(2, i - 2));
			int second = first * 3;
			zeromat.at<float>(0, first) = 0.5;
			zeromat.at <float>(0, second) = 0.5;
			h1 = zeromat.clone();
			zeromat.at <float>(0, second) = -0.5;
			g1 = zeromat.clone();
		}

		//cout << h1 << endl;
		//cout << g1 << endl;
		kerg1.push_back(g1);
		kerh1.push_back(h1);
		fh /= 2;
		//DWORD tin = GetTickCount();
		// image 1
		Mat_<float> z1 = conv2(borderExpand(M1, fh, 1), g1, CONVOLUTION_VALID);
		Mat_<float> a1 = conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID);
		Mat_<float> a2 = conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID);
		z1 = conv2(borderExpand(M1, fh, 1), h1, CONVOLUTION_VALID);
		Mat_<float> a3 = conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID);
		Mat_<float> a4 = conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID);
		//image 2
		z1 = conv2(borderExpand(M2, fh, 1), g1, CONVOLUTION_VALID);
		Mat_<float> b1 = conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID);
		Mat_<float> b2 = conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID);
		z1 = conv2(borderExpand(M2, fh, 1), h1, CONVOLUTION_VALID);
		Mat_<float> b3 = conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID);
		Mat_<float> b4 = conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID);
		//% select coefficients and store them

		temp.push_back(localMatch(a1, b1, um));
		temp.push_back(localMatch(a2, b2, um));
		temp.push_back(localMatch(a3, b3, um));
		coe.push_back(temp);
		//temp.clear();
		// copy input image for next decomposition stage
		M1 = a4;
		M2 = b4;
		// cout << "构建kernel用时" << GetTickCount() - tin << endl;
	}

	// select base coefficients of last decompostion stage
	Mat_<float> a4 = (M1 + M2) / 2;
	// loop over decomposition depth->synthesis
	for (int i = levels; i > 0; --i)
	{
		//define actual filters(inserting zeros between coefficients)
		//DWORD tin = GetTickCount();
		Mat_<float> h2 = kerh1[i - 1];
		Mat_<float> g2 = kerg1[i - 1];
		flip(h2, h2, -1);//0--x轴翻转，<0y轴,   >0先x再y翻转
		flip(g2, g2, -1);
		int fh = static_cast<int>((pow(2, i) + 1)) / 2;
		//filter(rows)
		a4 = conv2(borderExpand(a4, fh, 2), h2.t(), CONVOLUTION_VALID);
		Mat_<float> a3 = conv2(borderExpand(coe[i - 1][2], fh, 2), g2.t(), CONVOLUTION_VALID);
		Mat_<float> a2 = conv2(borderExpand(coe[i - 1][1], fh, 2), h2.t(), CONVOLUTION_VALID);
		Mat_<float> a1 = conv2(borderExpand(coe[i - 1][0], fh, 2), g2.t(), CONVOLUTION_VALID);

		//% filter(columns)
		a4 = conv2(borderExpand(a4 + a3, fh, 1), h2, CONVOLUTION_VALID);
		a2 = conv2(borderExpand(a2 + a1, fh, 1), g2, CONVOLUTION_VALID);
		a4 = a4 + a2;
		//cout << "yongshi" << GetTickCount() - tin  << endl;
	}
	ret = a4;
	return ret;
}
//image fusion with DWT, Wavelet is DBSS(2, 2)
Mat_<float> ImageFuse::fuse_DWT()
{
	Mat_<float> M1 = ir.clone();
	Mat_<float> M2 = vs.clone();
	assert(M1.size() == M2.size());
	Mat_<float> ret;
	vector<vector<Mat_<float>>> coe;
	vector<int> z1, s1;
	double iscale = 4 * sqrt(2);
	//loop over decomposition depth->analysis
	for (int i = 1; i <= levels; ++i)
	{
		vector<Mat_<float>> temp;
		int w = M1.cols;
		int h = M1.rows;
		z1.push_back(h);
		s1.push_back(w);
		//% define filters, padd with zeros due to phase distortions
		int fh = 7;
		Mat h1 = (Mat_<float>(1, 7) << -1, 2, 6, 2, -1, 0, 0);
		Mat g1 = (Mat_<float>(1, 7) << 0, 0, -2, 4, -2, 0, 0);
		//cout << h1<<endl;
		h1 /= iscale;
		g1 /= iscale;

		// image 1
		Mat_<float> z1 = downSamp(conv2(borderExpand(M1, fh, 1), g1, CONVOLUTION_VALID), 2);
		Mat_<float> a1 = downSamp(conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID), 1);
		Mat_<float> a2 = downSamp(conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID), 1);
		z1 = downSamp(conv2(borderExpand(M1, fh, 1), h1, CONVOLUTION_VALID), 2);
		Mat_<float> a3 = downSamp(conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID), 1);
		Mat_<float> a4 = downSamp(conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID), 1);
		//image 2
		z1 = downSamp(conv2(borderExpand(M2, fh, 1), g1, CONVOLUTION_VALID), 2);
		Mat_<float> b1 = downSamp(conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID), 1);
		Mat_<float> b2 = downSamp(conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID), 1);
		z1 = downSamp(conv2(borderExpand(M2, fh, 1), h1, CONVOLUTION_VALID), 2);
		Mat_<float> b3 = downSamp(conv2(borderExpand(z1, fh, 2), g1.t(), CONVOLUTION_VALID), 1);
		Mat_<float> b4 = downSamp(conv2(borderExpand(z1, fh, 2), h1.t(), CONVOLUTION_VALID), 1);
		//% select coefficients and store them

		temp.push_back(localMatch(a1, b1, um));
		temp.push_back(localMatch(a2, b2, um));
		temp.push_back(localMatch(a3, b3, um));
		coe.push_back(temp);
		//temp.clear();
		// copy input image for next decomposition stage
		M1 = a4;
		M2 = b4;
	}
	//cout << z1;
	//cout << s1;
	// select base coefficients of last decompostion stage
	Mat_<float> a4 = (M1 + M2) / 2;
	//cout << "a4size(1)" << a4.size() << endl;
	// loop over decomposition depth->synthesis
	for (int i = levels; i > 0; --i)
	{
		// define filters, padd with zeros due to phase distortions
		Mat h2 = (Mat_<float>(1, 7) << 0, 0, 0, 2, 4, 2, 0);
		Mat g2 = (Mat_<float>(1, 7) << 0, -1, -2, 6, -2, -1, 0);
		h2 /= iscale;
		g2 /= iscale;
		int fh = 3;
		//undecimate and interpolate (rows)
		a4 = conv2(borderExpand(upSamp(a4, 1), fh, 2), h2.t(), CONVOLUTION_VALID);
		//cout << "a4size(2)" << a4.size() << endl;
		Mat_<float> a3 = conv2(borderExpand(upSamp(coe[i - 1][2], 1), fh, 2), g2.t(), CONVOLUTION_VALID);
		Mat_<float> a2 = conv2(borderExpand(upSamp(coe[i - 1][1], 1), fh, 2), h2.t(), CONVOLUTION_VALID);
		Mat_<float> a1 = conv2(borderExpand(upSamp(coe[i - 1][0], 1), fh, 2), g2.t(), CONVOLUTION_VALID);
		//% undecimate and interpolate (columns)
		a4 = conv2(borderExpand(upSamp(a4 + a3, 2), fh, 1), h2, CONVOLUTION_VALID);
		a2 = conv2(borderExpand(upSamp(a2 + a1, 2), fh, 1), g2, CONVOLUTION_VALID);
		//cout << "a4size()" << a4.size() << endl;
		//cout << s1[i - 1] << "  " << z1[i - 1] << endl;
		// add images and select valid part
		a4 = a4 + a2;
		Mat_<float> tempa4 = a4(cv::Rect(4, 4, s1[i - 1], z1[i - 1]));
		a4 = tempa4.clone();
		//cout << tempa4 << endl;
		//imwrite("tempa4.jpg", tempa4 );
	}
	ret = a4;
	return ret;
}
//image fusion with gradient pyramid
Mat_<float> ImageFuse::fuse_gra()
{
	Mat_<float> M1 = ir.clone();
	Mat_<float> M2 = vs.clone();
	assert(M1.size() == M2.size());
	Mat_<float> ret;
	vector<Mat_<float>> coe;
	vector<int> z1, s1;
	//define filters
	double iscale = sqrt(2);
	Mat v = (Mat_<float>(1, 3) << 1, 2, 1);
	v = v / 4.0;
	Mat d1 = (Mat_<float>(1, 2) << 1, -1);
	Mat d2 = (Mat_<float>(2, 2) << 0, -1, 1, 0);
	d2 /= iscale;
	Mat d3 = (Mat_<float>(1, 2) << -1, 1);
	Mat d4 = (Mat_<float>(2, 2) << -1, 0, 0, 1);
	d4 /= iscale;
	// compute derivatives
	Mat dle = Mat::zeros(Size(3, 3), CV_32FC1);
	dle.row(1) = conv2(d1, d1) + 0.0;
	Mat d2e = conv2(d2, d2);
	Mat d3e = dle.t();
	Mat d4e = conv2(d4, d4);
	//loop over decomposition depth->analysis
	for (int i = 1; i <= levels; ++i)
	{
		int w = M1.cols;
		int h = M1.rows;
		z1.push_back(h);
		s1.push_back(w);
		// perform filtering
		Mat G1, G2;
		pyrDown(M1, G1);
		pyrDown(M2, G2);
		Mat  z1 = borderExpand(M1 + conv2(conv2(borderExpand(M1, 1), v, CONVOLUTION_VALID), v.t(), CONVOLUTION_VALID), 1);
		Mat  z2 = borderExpand(M2 + conv2(conv2(borderExpand(M2, 1), v, CONVOLUTION_VALID), v.t(), CONVOLUTION_VALID), 1);
		//% compute directional derivatives
		Mat  b = Mat::zeros(Size(M1.size()), CV_32FC1);
		Mat d1 = conv2(z1, dle, CONVOLUTION_VALID);
		Mat d2 = conv2(z2, dle, CONVOLUTION_VALID);
		b = b + localMatch(d1, d2, um);

		d1 = conv2(z1, d2e, CONVOLUTION_VALID);
		d2 = conv2(z2, d2e, CONVOLUTION_VALID);
		b = b + localMatch(d1, d2, um);

		d1 = conv2(z1, d3e, CONVOLUTION_VALID);
		d2 = conv2(z2, d3e, CONVOLUTION_VALID);
		b = b + localMatch(d1, d2, um);

		d1 = conv2(z1, d4e, CONVOLUTION_VALID);
		d2 = conv2(z2, d4e, CONVOLUTION_VALID);
		b = b + localMatch(d1, d2, um);
		//% store coefficients
		coe.push_back(-b / 8);
		//% decimate
		M1 = G1;
		M2 = G2;
	}
	//cout << z1;
	//cout << s1;
	// select base coefficients of last decompostion stage
	M1 = (M1 + M2) / 2;
	// loop over decomposition depth->synthesis
	for (int i = levels; i > 0; --i)
	{
		Mat up;
		pyrUp(M1, up, coe[i - 1].size());
		M1 = up + coe[i - 1];
		Mat_<float> tempm1 = M1(cv::Rect(0, 0, s1[i - 1], z1[i - 1]));
		M1 = tempm1.clone();
	}
	ret = M1;
	return ret;
}
//image fusion with fsd pyramid
Mat_<float> ImageFuse::fuse_fsd()
{
	Mat_<float> M1 = ir.clone();
	Mat_<float> M2 = vs.clone();
	assert(M1.size() == M2.size());
	Mat_<float> ret;
	vector<Mat_<float>> coe;
	vector<int> z1, s1;
	// define filter
	Mat kerw = (Mat_<float>(1, 5) << 1, 4, 6, 4, 1);
	kerw /= 16.0;
	//loop over decomposition depth->analysis
	for (int i = 1; i <= levels; ++i)
	{
		int w = M1.cols;
		int h = M1.rows;
		z1.push_back(h);
		s1.push_back(w);
		// perform filtering
		//% perform filtering
		//	G1 = conv2(conv2(es2(M1, 2), w, 'valid'), w', 'valid');
		//	G2 = conv2(conv2(es2(M2, 2), w, 'valid'), w', 'valid');
		Mat G1 = conv2(conv2(borderExpand(M1, 2), kerw, CONVOLUTION_VALID), kerw.t(), CONVOLUTION_VALID);
		Mat G2 = conv2(conv2(borderExpand(M2, 2), kerw, CONVOLUTION_VALID), kerw.t(), CONVOLUTION_VALID);
		coe.push_back(localMatch(M1 - G1, M2 - G2, um));

		Mat gao1, gao2;
		//gao1 = downSamp(G1);
		//gao2 = downSamp(G2);
		pyrDown(M1, gao1);
		pyrDown(M2, gao2);

		//% decimate
		M1 = gao1;
		M2 = gao2;
	}
	//cout << z1;
	//cout << s1;
	// select base coefficients of last decompostion stage
	M1 = (M1 + M2) / 2;
	// loop over decomposition depth->synthesis
	for (int i = levels; i > 0; --i)
	{
		Mat up;
		pyrUp(M1, up, coe[i - 1].size());
		M1 = up + coe[i - 1];
		Mat_<float> tempm1 = M1(cv::Rect(0, 0, s1[i - 1], z1[i - 1]));
		M1 = tempm1.clone();
	}
	ret = M1;
	return ret;
}
//image fusion with pca
Mat_<float> ImageFuse::fuse_pca()
{
	Mat_<float> M1 = ir.clone();
	Mat_<float> M2 = vs.clone();
	assert(M1.size() == M2.size());
	Mat_<float> ret;
	Mat_<float> M12;
	int length = M1.rows*M1.cols;
	Mat M11 = M1.reshape(0, length);
	Mat M22 = M2.reshape(0, length);
	hconcat(M11, M22, M12);
	//cout << M12.size() << endl;
	Mat convmat, convmean;
	//convmat = fuse_cov(M12);
	calcCovarMatrix(M12, convmat, convmean, CV_COVAR_NORMAL | CV_COVAR_ROWS);
	//cout << convmat << endl;
	Mat eigenval, eigenvec;
	eigen(convmat, eigenval, eigenvec);
	//cout << eigenval << endl;
	//cout << eigenvec << endl;
	Mat a;
	Mat vecsum;
	reduce(eigenvec, vecsum, 0, CV_REDUCE_SUM);
	if (eigenval.at<double>(0, 0) > eigenval.at<double>(1, 0))
	{
		a = eigenvec.col(0) / vecsum.col(0);
	}
	else
	{
		a = eigenvec.col(1) / vecsum.col(1);
	}
	//cout << a << endl;
	//cout << a1 << "   " << a2 << endl;
	ret = a.at<double>(0, 0)*M1 + a.at<double>(1, 0)*M2;
	return ret;
}
//加权平均融合
Mat_<float> ImageFuse::fuse_ave(const double weight )
{
	Mat M1 = ir.clone();
	Mat M2 = vs.clone();
	Mat_<float> ret = M1*weight + M2*(1 - weight);
	return ret;
}
