//--------------------------------------【程序说明】-------------------------------------------
//		程序说明：常见的图像融合算法
//		程序描述：常见的图像融合算法
//		开发测试所用IDE版本：Visual Studio 2013
//		开发测试所用OpenCV版本：	2.4.13
//		操作系统：Windows 10
//		测试图像：红外图像和可见光图像（微光图像） 
//		2017年8月 Created by @胡保林 hu_nobuone@163.com
//------------------------------------------------------------------------------------------------
// MFC_ImageFuseDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_ImageFuse.h"
#include "MFC_ImageFuseDlg.h"
#include "afxdialogex.h"
#include<io.h>

#include"imagefuse.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//static ImageFuse   imagefuse;				//实例化一个对象
//显示控制台
void InitConsole()
{
	int nRet = 0;
	FILE* fp;
	AllocConsole();
	system("color b0");
	system("title  图像融合系统输出窗口");
	nRet = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	fp = _fdopen(nRet, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
string wstring2string(const wstring &wstr)
{
	string result;

	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];

	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), buffer, len, NULL, NULL);
	buffer[len] = '\0';

	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;

	return result;
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFC_ImageFuseDlg 对话框



CMFC_ImageFuseDlg::CMFC_ImageFuseDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFC_ImageFuseDlg::IDD, pParent)
	, m_level(4)
	, m_filepath1(_T(""))
	, m_filepath2(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFC_ImageFuseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_LEVEL, m_level);
	DDV_MinMaxInt(pDX, m_level, 1, 5);
	DDX_Control(pDX, IDC_COMBO_METHOD, m_combo);
	DDX_Text(pDX, IDC_EDIT_PATH1, m_filepath1);
	DDX_Text(pDX, IDC_EDIT_PATH2, m_filepath2);
}

BEGIN_MESSAGE_MAP(CMFC_ImageFuseDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MFCBUTTON_SELECT1, &CMFC_ImageFuseDlg::OnBnClickedMfcbuttonSelect1)
	ON_BN_CLICKED(IDC_MFCBUTTON_SELECT2, &CMFC_ImageFuseDlg::OnBnClickedMfcbuttonSelect2)
	ON_BN_CLICKED(IDC_MFCBUTTON_FUSE, &CMFC_ImageFuseDlg::OnBnClickedMfcbuttonFuse)
	ON_BN_CLICKED(ID_EXIT, &CMFC_ImageFuseDlg::OnBnClickedExit)
END_MESSAGE_MAP()

//画图到pic控件
void CMFC_ImageFuseDlg::DrawPicToHDC(IplImage *img, UINT ID)
{
	CDC *pDC = GetDlgItem(ID)->GetDC();
	HDC hDC = pDC->GetSafeHdc();
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);
	CvvImage cimg;
	cimg.CopyOf(img); // 复制图片  
	cimg.DrawToHDC(hDC, &rect); // 将图片绘制到显示控件的指定区域内  
	ReleaseDC(pDC);
}

// CMFC_ImageFuseDlg 消息处理程序

BOOL CMFC_ImageFuseDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	InitConsole();
	// 为 组合框控件的列表框添加列表项:  
	m_combo.AddString(_T("拉普拉斯lap"));
	m_combo.AddString(_T("对比度"));
	m_combo.AddString(_T("比率"));
	m_combo.AddString(_T("小波sidwt"));
	m_combo.AddString(_T("小波"));
	m_combo.AddString(_T("fsd金字塔"));
	m_combo.AddString(_T("gra梯度"));
	m_combo.AddString(_T("pca主成分"));
	m_combo.AddString(_T("ave平均"));
	m_combo.SetCurSel(0);			//默认选择原图

	// TODO:  在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFC_ImageFuseDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFC_ImageFuseDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFC_ImageFuseDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CMFC_ImageFuseDlg::OnBnClickedMfcbuttonSelect1()
{
	// TODO:  在此添加控件通知处理程序代码
	TCHAR defaultpath[] = TEXT(".\\test_image\\");
	CFileDialog dlg(TRUE, _T("jpg"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("(*.jpg;*.png;*.bmp)|*.jpg;*.png;*.bmp|(All files)|*.*"));
	dlg.m_ofn.lpstrTitle = _T("请红外图像");
	dlg.m_ofn.lpstrInitialDir = defaultpath;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	m_filepath1 = dlg.GetPathName();
	SetDlgItemText(IDC_EDIT_PATH1, m_filepath1);
	IplImage *image1 = cvLoadImage(wstring2string(m_filepath1.GetBuffer(0)).c_str(), 0); //显示灰度图片  
	Mat infr = Mat(image1, 1);	//1表示复制数据，0表示不复制数据
	infr.convertTo(fir, CV_32FC1, 1 / 255.0);// 
	DrawPicToHDC(image1, IDC_STATIC_IR);
}


void CMFC_ImageFuseDlg::OnBnClickedMfcbuttonSelect2()
{
	// TODO:  在此添加控件通知处理程序代码
	TCHAR defaultpath[] = TEXT(".\\test_image\\");
	CFileDialog dlg(TRUE, _T("jpg"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("(*.jpg;*.png;*.bmp)|*.jpg;*.png;*.bmp||"));
	dlg.m_ofn.lpstrTitle = _T("请可见光图像");
	dlg.m_ofn.lpstrInitialDir = defaultpath;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	m_filepath2 = dlg.GetPathName();
	SetDlgItemText(IDC_EDIT_PATH2, m_filepath2);
	IplImage *image2 = cvLoadImage(wstring2string(m_filepath2.GetBuffer(0)).c_str(), 0); //显示灰度图片 
	Mat visi = Mat(image2, 1);
	visi.convertTo(fvs, CV_32FC1, 1 / 255.0);
	DrawPicToHDC(image2, IDC_STATIC_VS);
}


void CMFC_ImageFuseDlg::OnBnClickedMfcbuttonFuse()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	ImageFuse imagefuse(fir, fvs, m_level, 3);
	int64 tin = cv::getTickCount();
	switch (m_combo.GetCurSel())
	{
	case 0:
		fuse=imagefuse.lapFuse();
		break;
	case 1:
		fuse = imagefuse.conFuse();
		break;
	case 2:
		fuse = imagefuse.ratFuse();
		break;
	case 3:
		fuse = imagefuse.fuse_SIDWT();
		break;
	case 4:
		fuse = imagefuse.fuse_DWT();
		break;
	case 5:
		fuse = imagefuse.fuse_fsd();
		break;
	case 6:
		fuse = imagefuse.fuse_gra();
		break;
	case 7:
		fuse = imagefuse.fuse_pca();
		break;
	case 8:
		fuse = imagefuse.fuse_ave();
		break;
	default:
		break;
	}
	std::cout << "用时：" << 1000.0*(getTickCount()-tin)/getTickFrequency() << endl;
	IplImage *image = &(IplImage)fuse;
	DrawPicToHDC(image, IDC_STATIC_FUSE);
}




void CMFC_ImageFuseDlg::OnBnClickedExit()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialog::OnDestroy();
	FreeConsole();
	CDialogEx::OnOK();
}
