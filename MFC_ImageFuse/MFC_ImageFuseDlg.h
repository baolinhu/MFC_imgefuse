
// MFC_ImageFuseDlg.h : 头文件
//
#include"CvvImage.h"
#include"cv.h"
#pragma once

#define _O_TEXT         0x4000 
// CMFC_ImageFuseDlg 对话框
class CMFC_ImageFuseDlg : public CDialogEx
{
// 构造
public:
	CMFC_ImageFuseDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFC_IMAGEFUSE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	int m_level;
	CComboBox m_combo;			//下拉框
	cv::Mat_<float> fir;
	cv::Mat_<float> fvs;
	cv::Mat fuse;
public:
	void  DrawPicToHDC(IplImage *img, UINT ID);
	afx_msg void OnBnClickedMfcbuttonSelect1();
	CString m_filepath1;
	CString m_filepath2;
	afx_msg void OnBnClickedMfcbuttonSelect2();
	afx_msg void OnBnClickedMfcbuttonFuse();
	afx_msg void OnBnClickedExit();
};
