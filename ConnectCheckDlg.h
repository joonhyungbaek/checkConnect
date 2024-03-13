
// ConnectCheckDlg.h : 헤더 파일
//

#pragma once

#include <fstream>
// CConnectCheckDlg 대화 상자
class CConnectCheckDlg : public CDialogEx
{
	// 생성입니다.
public:
	CConnectCheckDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

											// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONNECTCHECK_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


														// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


private:
	static bool m_bIsConnected;
	static CString m_strServerIp;
	CString m_strClientIp;
	const CString m_strIniPath;
	CWinThread* pThread;
	CIPAddressCtrl m_IPControl;
	NOTIFYICONDATA nid;
	static std::ofstream logfile;

public:
	void readIni(CString section, CString key, TCHAR* str);
	CString GetIpAddress();
	void writeIPcontrol();
	static UINT ThreadProc(LPVOID pParam);
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	void OnTimer(UINT_PTR nIDEvent);
	BOOL PreTranslateMessage(MSG* pMsg);
	LRESULT OnTaryNotifyAction(WPARAM wParam, LPARAM lParam);
	void OnClose();
	void OnMenuOpen();
	void OnMenuClose();
	static std::string getCurrentDateTime();
};