# Image Fuse（图像融合）

常见的红外图像和可见光（微光）图像融合算法，主要使用塔形分解融合算法。对应的[CSDN博客在这里](http://blog.csdn.net/baolinq)

关键词：图像融合，红外图像，可见光图像，微光图像

development environment（开发环境）：

* 程序描述：常见的红外图像和可见光（微光）图像融合算法
* 开发测试所用IDE版本：Visual Studio 2013
* 开发测试所用OpenCV版本：	2.4.13
* 操作系统：Windows 10
* 2017年8月 Created by @胡保林  hu_nobuone@163.com

Basic useful functions（基本功能）:

* Fuse_laplace 基于laplace金字塔的
* Fuse_dwt  基于小波变换，DBSS(2，2)
* Fuse_sidwt 基于另一种小波变换，Haar
* Fuse_con，基于对比度金字塔
* Fuse_rat，基于比率金字塔
* Fuse_gra，基于梯度金字塔
* Fuse_pca，基于pca
* Fuse_fsd，基于FSD金字塔
* 可以选择以上任意的方法和选择层数

input:一张红外图像和一张可见光（微光）图像，图像需要是已经配准过的，选择一个方法和层数就可以进行融合。默认是laplace金字塔，层数为4

output:融合后的图像


界面截图：
![](http://img.blog.csdn.net/20170930153258985?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYmFvbGlucQ==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/Center)
