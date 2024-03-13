
// ConnectCheckDlg.h : ��� ����
//

#pragma once

#include <fstream>
// CConnectCheckDlg ��ȭ ����
class CConnectCheckDlg : public CDialogEx
{
	// �����Դϴ�.
public:
	CConnectCheckDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

											// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONNECTCHECK_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


														// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
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