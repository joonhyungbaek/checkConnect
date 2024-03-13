
// ConnectCheckDlg.cpp : 구현 파일
//
//#include <Ws2tcpip.h>
#include <iostream>
#include "stdafx.h"
#include "ConnectCheck.h"
#include "ConnectCheckDlg.h"

#include "afxdialogex.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <regex>

#include <sstream> 
#include <ctime>
#include <iomanip>
#include <chrono>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma warning( disable : 4996)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_TRAY_NOTIFYICACTION WM_APP+10

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.
bool CConnectCheckDlg::m_bIsConnected = false;
CString CConnectCheckDlg::m_strServerIp = _T("");
std::ofstream CConnectCheckDlg::logfile("log.txt", std::ios::app);

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

														// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CConnectCheckDlg 대화 상자
CConnectCheckDlg::CConnectCheckDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CONNECTCHECK_DIALOG, pParent)
	, m_strClientIp(_T(""))
	, m_strIniPath(_T(".\\config.ini"))
	, pThread(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CConnectCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_IPControl);
}

BEGIN_MESSAGE_MAP(CConnectCheckDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_TRAY_NOTIFYICACTION, OnTaryNotifyAction)
	ON_COMMAND(ID_MENU_OPEN, &CConnectCheckDlg::OnMenuOpen)
	ON_COMMAND(ID_MENU_CLOSE, &CConnectCheckDlg::OnMenuClose)
END_MESSAGE_MAP()


// CConnectCheckDlg 메시지 처리기

BOOL CConnectCheckDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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
	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

									// TODO: 여기에 추가 초기화 작업을 추가합니다.

	TCHAR clientTmpIp[32] = { 0 };
	readIni(_T("CLIENT"), _T("IP"), clientTmpIp);

	if (clientTmpIp[0] == 'X') {
		m_strClientIp = GetIpAddress();
		WritePrivateProfileString(_T("CLIENT"), _T("IP"), m_strClientIp, m_strIniPath);
	}
	else m_strClientIp = clientTmpIp;

	writeIPcontrol();

	TCHAR ServerTmpIp[32] = { 0 };
	readIni(_T("SERVER"), _T("IP"), ServerTmpIp);
	m_strServerIp = ServerTmpIp;

	pThread = AfxBeginThread(ThreadProc, nullptr);

	SetTimer(0, 500, NULL);
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CConnectCheckDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CConnectCheckDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CConnectCheckDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


CString CConnectCheckDlg::GetIpAddress()
{
	PIP_ADAPTER_INFO adapterInfo;
	ULONG bufferSize = 0;
	CString IpAddress;
	// 첫 번째 호출: 버퍼 크기 계산
	if (GetAdaptersInfo(nullptr, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
		adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);

		// 두 번째 호출: 실제 정보 가져오기
		if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
			IpAddress = adapterInfo->IpAddressList.IpAddress.String;
		}
		free(adapterInfo);
	}
	return IpAddress;
}

HBRUSH CConnectCheckDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	// TODO:  여기서 DC의 특성을 변경합니다.
	if (pWnd->GetDlgCtrlID() == ID_RESULT)
	{
		if (m_bIsConnected) {
			pDC->SetTextColor(RGB(0, 0, 255));
		}
		else {
			pDC->SetTextColor(RGB(255, 0, 0));
		}
	}
	return hbr;
}


void CConnectCheckDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == 0)
		Invalidate();

	CDialogEx::OnTimer(nIDEvent);
}


UINT CConnectCheckDlg::ThreadProc(LPVOID pParam)
{
	while (true)
	{
		HANDLE hIcmpFile;
		unsigned long ipaddr = INADDR_NONE;
		DWORD dwRetVal = 0;
		DWORD dwError = 0;
		char SendData[] = "Data Buffer";
		LPVOID ReplyBuffer = NULL;
		DWORD ReplySize = 0;
		std::string logMsg;
		std::string currentDateTime = getCurrentDateTime();

		ipaddr = inet_addr((CStringA)m_strServerIp);

		hIcmpFile = IcmpCreateFile();

		if (hIcmpFile == INVALID_HANDLE_VALUE) {
			m_bIsConnected = false;
			logMsg = " Unable to open handle, IcmpCreatefile returned error";
			logfile << (std::string)currentDateTime << logMsg << std::endl;
			continue;
		}

		// Allocate space for a single reply.
		ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
		ReplyBuffer = (VOID *)malloc(ReplySize);
		if (ReplyBuffer == NULL) {
			m_bIsConnected = false;
			logMsg = " Unable to allocate memory for reply buffer";
			logfile << (std::string)currentDateTime << logMsg << std::endl;
			continue;
		}

		dwRetVal = IcmpSendEcho2(
			hIcmpFile, NULL, NULL, NULL,
			ipaddr, SendData, sizeof(SendData), NULL,
			ReplyBuffer, ReplySize, 1000);

		if (dwRetVal != 0) {
			PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
			struct in_addr ReplyAddr;
			ReplyAddr.S_un.S_addr = pEchoReply->Address;

			switch (pEchoReply->Status) {
			case IP_DEST_HOST_UNREACHABLE:
				logMsg = " Destination host was unreachable";
				m_bIsConnected = false;
				break;
			case IP_DEST_NET_UNREACHABLE:
				logMsg = " Destination Network was unreachable";
				m_bIsConnected = false;
				break;
			case IP_REQ_TIMED_OUT:
				logMsg = " Request timed out";
				m_bIsConnected = false;
				break;
			default:
				logMsg = " Success";
				m_bIsConnected = true;
				break;
			}

		}
		else {
			dwError = GetLastError();
			m_bIsConnected = false;

			switch (dwError) {
			case IP_BUF_TOO_SMALL:
				logMsg = " ReplyBufferSize too small";
				break;
			case IP_REQ_TIMED_OUT:
				logMsg = " Request timed out";
				break;
			default:
				logMsg = " Extended error returned";
				break;
			}
		}
		if (!(logMsg == " Success")) {
			logfile << (std::string)currentDateTime << logMsg << std::endl;
		}
		Sleep(500);
	}
}


void CConnectCheckDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	ZeroMemory(&nid, sizeof(nid));

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.uCallbackMessage = WM_TRAY_NOTIFYICACTION;

	if (m_bIsConnected) nid.hIcon = AfxGetApp()->LoadIconW(IDI_GREEN);
	else nid.hIcon = AfxGetApp()->LoadIconW(IDI_RED);

	nid.hWnd = m_hWnd;
	Shell_NotifyIcon(NIM_ADD, &nid);

	AfxGetApp()->m_pMainWnd->ShowWindow(SW_HIDE);    // 다이얼로그는 숨는다.
}


LRESULT CConnectCheckDlg::OnTaryNotifyAction(WPARAM wParam, LPARAM lParam)
{
	switch (lParam) {

	case WM_RBUTTONDOWN:
	{
		CPoint ptMouse;
		GetCursorPos(&ptMouse);

		// 아이콘 외 구역 마우스 클릭 시 메뉴 꺼짐
		AfxGetMainWnd()->SetForegroundWindow();

		CMenu menu;
		menu.LoadMenu(IDR_MENU1);
		CMenu* pMenu = menu.GetSubMenu(0); //활성화 할 메뉴 지정
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, AfxGetMainWnd());
		break;
	}
	case WM_LBUTTONDBLCLK:
	{
		if (!Shell_NotifyIcon(NIM_DELETE, &nid))
		{
			if (AfxMessageBox(_T("트레이아이콘 제거실패")) == IDOK) OnOK();
		}
		AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW); //윈도우 활성화
		break;
	}
	default:
		break;
	}
	return 1;

}

void CConnectCheckDlg::OnMenuOpen()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	Shell_NotifyIcon(NIM_DELETE, &nid); //트레이아이콘 제거
	AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW); //윈도우 활성화
}


void CConnectCheckDlg::OnMenuClose()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.

	Shell_NotifyIcon(NIM_DELETE, &nid); //트레이아이콘 제거

	if (pThread != nullptr && pThread->m_hThread != nullptr)
	{
		TerminateThread(pThread->m_hThread, 0);
		pThread = nullptr;
	}
	OnOK();
}


void CConnectCheckDlg::readIni(CString section, CString key, TCHAR* str)
{
	GetPrivateProfileString(section, key, _T("X"), str, 32, m_strIniPath);

	std::wregex ipRegex(L"^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");

	if (!std::regex_match(str, ipRegex)) {
		if (section == "SERVER") {
			if (AfxMessageBox(_T("config.ini 파일에 올바른 서버 IP 주소를 입력해주세요")) == IDOK)
				OnOK();
		}
		else if (section == "CLIENT") {
			m_strClientIp = GetIpAddress();
			WritePrivateProfileString(_T("CLIENT"), _T("IP"), NULL, m_strIniPath);
			WritePrivateProfileString(_T("CLIENT"), _T("IP"), m_strClientIp, m_strIniPath);
			str[0] = 'X';
		}
		else {
			if (AfxMessageBox(_T("config.ini 파일에 올바른 IP 주소를 입력해주세요")) == IDOK)
				OnOK();
		}
		return;
	}

	if (section == "SERVER" && str[0] == 'X') {
		if (AfxMessageBox(_T("config.ini 파일에 접속할 서버 IP 주소를 입력해주세요")) == IDOK)
			OnOK();
	}
}

BOOL CConnectCheckDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
			return TRUE;

	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CConnectCheckDlg::writeIPcontrol()
{
	int nTokenPos = 0;
	CString strToken = m_strClientIp.Tokenize(_T("."), nTokenPos);

	CString str[4];
	int ip[4];

	int i = 0;

	while (!strToken.IsEmpty())
	{
		str[i++] = strToken;
		strToken = m_strClientIp.Tokenize(_T("."), nTokenPos);
	}

	for (size_t i = 0; i < 4; i++)
		ip[i] = _ttoi(str[i]);

	m_IPControl.SetAddress(ip[0], ip[1], ip[2], ip[3]);
}


std::string CConnectCheckDlg::getCurrentDateTime()
{
	auto now = std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm* localTime = std::localtime(&currentTime);

	std::ostringstream oss;
	oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}
