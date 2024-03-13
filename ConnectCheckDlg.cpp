
// ConnectCheckDlg.cpp : ���� ����
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

// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.
bool CConnectCheckDlg::m_bIsConnected = false;
CString CConnectCheckDlg::m_strServerIp = _T("");
std::ofstream CConnectCheckDlg::logfile("log.txt", std::ios::app);

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

														// �����Դϴ�.
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


// CConnectCheckDlg ��ȭ ����
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


// CConnectCheckDlg �޽��� ó����

BOOL CConnectCheckDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
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
	// �� ��ȭ ������ �������� �����մϴ�.  ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

									// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

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
	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
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

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�.  ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CConnectCheckDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CConnectCheckDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


CString CConnectCheckDlg::GetIpAddress()
{
	PIP_ADAPTER_INFO adapterInfo;
	ULONG bufferSize = 0;
	CString IpAddress;
	// ù ��° ȣ��: ���� ũ�� ���
	if (GetAdaptersInfo(nullptr, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
		adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);

		// �� ��° ȣ��: ���� ���� ��������
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
	// TODO:  ���⼭ DC�� Ư���� �����մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	ZeroMemory(&nid, sizeof(nid));

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.uCallbackMessage = WM_TRAY_NOTIFYICACTION;

	if (m_bIsConnected) nid.hIcon = AfxGetApp()->LoadIconW(IDI_GREEN);
	else nid.hIcon = AfxGetApp()->LoadIconW(IDI_RED);

	nid.hWnd = m_hWnd;
	Shell_NotifyIcon(NIM_ADD, &nid);

	AfxGetApp()->m_pMainWnd->ShowWindow(SW_HIDE);    // ���̾�α״� ���´�.
}


LRESULT CConnectCheckDlg::OnTaryNotifyAction(WPARAM wParam, LPARAM lParam)
{
	switch (lParam) {

	case WM_RBUTTONDOWN:
	{
		CPoint ptMouse;
		GetCursorPos(&ptMouse);

		// ������ �� ���� ���콺 Ŭ�� �� �޴� ����
		AfxGetMainWnd()->SetForegroundWindow();

		CMenu menu;
		menu.LoadMenu(IDR_MENU1);
		CMenu* pMenu = menu.GetSubMenu(0); //Ȱ��ȭ �� �޴� ����
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, AfxGetMainWnd());
		break;
	}
	case WM_LBUTTONDBLCLK:
	{
		if (!Shell_NotifyIcon(NIM_DELETE, &nid))
		{
			if (AfxMessageBox(_T("Ʈ���̾����� ���Ž���")) == IDOK) OnOK();
		}
		AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW); //������ Ȱ��ȭ
		break;
	}
	default:
		break;
	}
	return 1;

}

void CConnectCheckDlg::OnMenuOpen()
{
	// TODO: ���⿡ ��� ó���� �ڵ带 �߰��մϴ�.
	Shell_NotifyIcon(NIM_DELETE, &nid); //Ʈ���̾����� ����
	AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW); //������ Ȱ��ȭ
}


void CConnectCheckDlg::OnMenuClose()
{
	// TODO: ���⿡ ��� ó���� �ڵ带 �߰��մϴ�.

	Shell_NotifyIcon(NIM_DELETE, &nid); //Ʈ���̾����� ����

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
			if (AfxMessageBox(_T("config.ini ���Ͽ� �ùٸ� ���� IP �ּҸ� �Է����ּ���")) == IDOK)
				OnOK();
		}
		else if (section == "CLIENT") {
			m_strClientIp = GetIpAddress();
			WritePrivateProfileString(_T("CLIENT"), _T("IP"), NULL, m_strIniPath);
			WritePrivateProfileString(_T("CLIENT"), _T("IP"), m_strClientIp, m_strIniPath);
			str[0] = 'X';
		}
		else {
			if (AfxMessageBox(_T("config.ini ���Ͽ� �ùٸ� IP �ּҸ� �Է����ּ���")) == IDOK)
				OnOK();
		}
		return;
	}

	if (section == "SERVER" && str[0] == 'X') {
		if (AfxMessageBox(_T("config.ini ���Ͽ� ������ ���� IP �ּҸ� �Է����ּ���")) == IDOK)
			OnOK();
	}
}

BOOL CConnectCheckDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
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
