
// AutoMouseClickDlg.cpp: 实现文件
//
#include "pch.h"
#include "framework.h"
#include "AutoMouseClick.h"
#include "AutoMouseClickDlg.h"
#include "afxdialogex.h"
#include <thread>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

char SettingFile[MAX_PATH];
BOOL IsRunning = FALSE;
LRESULT CALLBACK HookEvent(int nCode, WPARAM wParam, LPARAM lParam);
HHOOK g_hHook = NULL;
void AutoMouseClick(DlgParams params);
void CreateJsonFile(const char* filename);
// CAutoMouseClickDlg 对话框


CAutoMouseClickDlg::CAutoMouseClickDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTOMOUSECLICK_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAutoMouseClickDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, CB_TYPE, cb_type);
}

BEGIN_MESSAGE_MAP(CAutoMouseClickDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(B_GETPOS, &CAutoMouseClickDlg::OnBnClickedGetpos)
	ON_BN_CLICKED(C_AUTOPOS, &CAutoMouseClickDlg::OnBnClickedAutopos)
	ON_BN_CLICKED(C_AUTOCLICK, &CAutoMouseClickDlg::OnBnClickedAutoclick)
	ON_BN_CLICKED(B_START, &CAutoMouseClickDlg::OnBnClickedStart)
	ON_BN_CLICKED(B_STOP, &CAutoMouseClickDlg::OnBnClickedStop)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CAutoMouseClickDlg 消息处理程序

BOOL CAutoMouseClickDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	// TODO: 在此添加额外的初始化代码
	//GetFileNameWithoutExtension(SettingFile);
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookEvent, GetModuleHandle(NULL), NULL);
	CRect FrmRect;
	GetWindowRect(&FrmRect);
	GetModuleFileNameA(NULL, SettingFile, MAX_PATH);
	strcat_s(SettingFile, ".json");
	FILE* file = NULL;
	errno_t err = fopen_s(&file, SettingFile, "r");
	if (err != 0)
	{
		CreateJsonFile(SettingFile);
		SetFileAttributes(CString(SettingFile),FILE_ATTRIBUTE_HIDDEN);
		fopen_s(&file, SettingFile, "r");
	}
	rapidjson::Document doc;
	char buffer[1024];
	fgets(buffer, 1024, file);
	doc.Parse(buffer);
	int left = doc["location"]["left"].GetInt();
	int top = doc["location"]["top"].GetInt();
	MoveWindow(left, top, FrmRect.Width(), FrmRect.Height());
	((CButton*)GetDlgItem(C_AUTOPOS))->SetCheck(doc["parameters"]["CAUTOPOS"].GetBool());
	GetDlgItem(E_X)->SetWindowText(CString(doc["parameters"]["EX"].GetString()));
	GetDlgItem(E_Y)->SetWindowText(CString(doc["parameters"]["EY"].GetString()));
	GetDlgItem(E_INTERVAL)->SetWindowText(CString(doc["parameters"]["EINTERVAL"].GetString()));
	GetDlgItem(E_DELAYED)->SetWindowText(CString(doc["parameters"]["EDELAYED"].GetString()));
	GetDlgItem(E_COUNT)->SetWindowText(CString(doc["parameters"]["ECOUNT"].GetString()));
	((CButton*)GetDlgItem(C_AUTOCLICK))->SetCheck(doc["parameters"]["CAUTOCLICK"].GetBool());
	cb_type.SetCurSel(doc["parameters"]["CBTYPE"].GetInt());
	if (IsDlgButtonChecked(C_AUTOPOS))OnBnClickedAutopos();
	if (IsDlgButtonChecked(C_AUTOCLICK))OnBnClickedAutoclick();
	if (file) fclose(file);
	CEdit E_Interval;
	E_Interval.Attach(GetDlgItem(E_INTERVAL)->m_hWnd);
	int nlength = E_Interval.GetWindowTextLengthW();
	E_Interval.SetSel(nlength, nlength);
	E_Interval.SetFocus();
	E_Interval.Detach();
	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAutoMouseClickDlg::OnPaint()
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
HCURSOR CAutoMouseClickDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CAutoMouseClickDlg::OnClose()
{
	UnhookWindowsHookEx(g_hHook);
	FILE* file = NULL;
	fopen_s(&file, SettingFile, "r");
	rapidjson::Document doc;
	char strbuffer[1024];
	fgets(strbuffer, 1024, file);
	doc.Parse(strbuffer);
	fclose(file);
	SetFileAttributes(CString(SettingFile), FILE_ATTRIBUTE_NORMAL);
	fopen_s(&file, SettingFile, "w");
	RECT FrmRect;
	GetWindowRect(&FrmRect);
	doc["location"]["left"].SetInt(FrmRect.left);
	doc["location"]["top"].SetInt(FrmRect.top);
	doc["parameters"]["CAUTOPOS"].SetBool(IsDlgButtonChecked(C_AUTOPOS));
	doc["parameters"]["EX"].SetString(rapidjson::GenericStringRef<char>(GetEditText(E_X)));
	doc["parameters"]["EY"].SetString(rapidjson::GenericStringRef<char>(GetEditText(E_Y)));
	doc["parameters"]["EINTERVAL"].SetString(rapidjson::GenericStringRef<char>(GetEditText(E_INTERVAL)));
	doc["parameters"]["EDELAYED"].SetString(rapidjson::GenericStringRef<char>(GetEditText(E_DELAYED)));
	doc["parameters"]["ECOUNT"].SetString(rapidjson::GenericStringRef<char>(GetEditText(E_COUNT)));
	doc["parameters"]["CAUTOCLICK"].SetBool(IsDlgButtonChecked(C_AUTOCLICK));
	doc["parameters"]["CBTYPE"].SetInt(cb_type.GetCurSel());
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	fprintf_s(file, buffer.GetString());
	fclose(file);
	SetFileAttributes(CString(SettingFile), FILE_ATTRIBUTE_HIDDEN);
	CDialogEx::OnClose();
}

void CAutoMouseClickDlg::OnBnClickedGetpos()
{
	CString P_X, P_Y;
	CPoint m_point;
	GetCursorPos(&m_point);
	P_X.Format(_T("%ld"), m_point.x);
	P_Y.Format(_T("%ld"), m_point.y);
	GetDlgItem(E_X)->SetWindowText(P_X);
	GetDlgItem(E_Y)->SetWindowText(P_Y);
}


void CAutoMouseClickDlg::OnBnClickedAutopos()
{
	if (IsDlgButtonChecked(C_AUTOPOS)){
		GetDlgItem(T_X)->EnableWindow(FALSE);
		GetDlgItem(T_Y)->EnableWindow(FALSE);
		GetDlgItem(E_X)->EnableWindow(FALSE);
		GetDlgItem(E_Y)->EnableWindow(FALSE);
		GetDlgItem(B_GETPOS)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(T_X)->EnableWindow(TRUE);
		GetDlgItem(T_Y)->EnableWindow(TRUE);
		GetDlgItem(E_X)->EnableWindow(TRUE);
		GetDlgItem(E_Y)->EnableWindow(TRUE);
		GetDlgItem(B_GETPOS)->EnableWindow(TRUE);
	}
}


void CAutoMouseClickDlg::OnBnClickedAutoclick()
{
	if (IsDlgButtonChecked(C_AUTOCLICK)) {
		GetDlgItem(T_COUNT)->EnableWindow(FALSE);
		GetDlgItem(E_COUNT)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(T_COUNT)->EnableWindow(TRUE);
		GetDlgItem(E_COUNT)->EnableWindow(TRUE);
	}
}

LRESULT CALLBACK HookEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
	CWPSTRUCT* p = (CWPSTRUCT*)lParam;
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
	{
		if (p->lParam == VK_F10) {
			HWND hWnd = AfxGetMainWnd()->m_hWnd;
			CAutoMouseClickDlg* pWnd = (CAutoMouseClickDlg*)CAutoMouseClickDlg::FromHandle(hWnd);
			pWnd->OnBnClickedStart();
		}
		if (p->lParam == VK_F12) {
			HWND hWnd = AfxGetMainWnd()->m_hWnd;
			CAutoMouseClickDlg* pWnd = (CAutoMouseClickDlg*)CAutoMouseClickDlg::FromHandle(hWnd);
			pWnd->OnBnClickedStop();
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DlgParams CAutoMouseClickDlg::GetDlgParams()
{
	DlgParams Params;
	CString str;
	Params.AutoPos = IsDlgButtonChecked(C_AUTOPOS);
	GetDlgItem(E_X)->GetWindowText(str);
	Params.X = _ttoi(str);
	GetDlgItem(E_Y)->GetWindowText(str);
	Params.Y = _ttoi(str);
	GetDlgItem(E_INTERVAL)->GetWindowText(str);
	Params.Interval = _ttoi(str);
	GetDlgItem(E_DELAYED)->GetWindowText(str);
	Params.Delayed = _ttoi(str);
	GetDlgItem(E_COUNT)->GetWindowText(str);
	Params.Count = _ttoi(str);
	Params.AutoClick = IsDlgButtonChecked(C_AUTOCLICK);
	Params.ClickType = cb_type.GetCurSel();
	return Params;
}

void CAutoMouseClickDlg::OnBnClickedStart()
{
	IsRunning = TRUE;
	DlgParams m_params = GetDlgParams();
	std::thread t(AutoMouseClick, m_params);
	t.detach();
}


void CAutoMouseClickDlg::OnBnClickedStop()
{
	IsRunning = FALSE;
}

void AutoMouseClick(DlgParams params)
{
	CString str;
	CPoint m_point;
	int count = 0;
	if (!params.AutoPos) m_point = CPoint(params.X, params.Y);
	while (IsRunning)
	{
		if (params.AutoPos) GetCursorPos(&m_point);
		else SetCursorPos(m_point.x, m_point.y);		
		if (params.ClickType == 0) {
			mouse_event(MOUSEEVENTF_LEFTDOWN, m_point.x, m_point.y, 0, 0);
			Sleep(params.Delayed);
			mouse_event(MOUSEEVENTF_LEFTUP, m_point.x, m_point.y, 0, 0);
		}
		else if (params.ClickType == 1) 
		{
			mouse_event(MOUSEEVENTF_LEFTDOWN, m_point.x, m_point.y, 0, 0);
			Sleep(params.Delayed);
			mouse_event(MOUSEEVENTF_LEFTUP, m_point.x, m_point.y, 0, 0);
			Sleep(20);
			mouse_event(MOUSEEVENTF_LEFTDOWN, m_point.x, m_point.y, 0, 0);
			Sleep(params.Delayed);
			mouse_event(MOUSEEVENTF_LEFTUP, m_point.x, m_point.y, 0, 0);
		}
		else if (params.ClickType == 2)
		{
			mouse_event(MOUSEEVENTF_RIGHTDOWN, m_point.x, m_point.y, 0, 0);
			Sleep(params.Delayed);
			mouse_event(MOUSEEVENTF_RIGHTUP, m_point.x, m_point.y, 0, 0);
		}
		else if (params.ClickType == 3)
		{
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, m_point.x, m_point.y, 0, 0);
			Sleep(params.Delayed);
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, m_point.x, m_point.y, 0, 0);
		}
		Sleep(params.Interval);
		if (!params.AutoClick) {
			++count;
			if(count >= params.Count) break;
		}
	}
}

void CreateJsonFile(const char* filename)
{
	FILE* file;
	fopen_s(&file, filename, "w");
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	writer.StartObject();
	writer.Key("location");
	writer.StartObject();
	writer.Key("left");
	writer.Int(0);
	writer.Key("top");
	writer.Int(0);
	writer.EndObject();
	writer.Key("parameters");
	writer.StartObject();
	writer.Key("CAUTOPOS");
	writer.Bool(false);
	writer.Key("EX");
	writer.String("100");
	writer.Key("EY");
	writer.String("100");
	writer.Key("EINTERVAL");
	writer.String("500");
	writer.Key("EDELAYED");
	writer.String("20");
	writer.Key("ECOUNT");
	writer.String("10");
	writer.Key("CAUTOCLICK");
	writer.Bool(false);
	writer.Key("CBTYPE");
	writer.Int(0);
	writer.EndObject();
	writer.EndObject();
	fprintf_s(file, buffer.GetString());
	fclose(file);
}

const char* CAutoMouseClickDlg::GetEditText(int nID)
{
	CString WStr;
	GetDlgItem(nID)->GetWindowTextW(WStr);
	size_t len = wcslen(WStr) + 1;
	size_t converted = 0;
	char* CStr;
	CStr = (char*)malloc(len * sizeof(char));
	wcstombs_s(&converted, CStr, len, WStr, _TRUNCATE);
	return CStr;
}



