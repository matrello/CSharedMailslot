// CSharedMailslotDemoDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "SharedMailslot.h"

// CSharedMailslotDemoDlg dialog
class CSharedMailslotDemoDlg : public CDialog
{
// Construction
public:
	CSharedMailslotDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CSHAREDMAILSLOTDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	CSharedMailslot mailslotServer;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit messageTextBox;
	afx_msg void OnBnClickedReceive();
	afx_msg void OnBnClickedSend();
	CEdit mailslotNameTextBox;
	CEdit fromtoTextBox;
	CStatic fromtoLabel;
	afx_msg void OnEnChangeMessage();
	afx_msg void OnBnClickedReopen();
	CButton messngrFormatCheckBox;
	CButton sharedCheckBox;
	CString workstationName;
	void GetWorkstationName();
	CStatic onqueueLabel;
	CStatic threadidLabel;
	CButton reopenButton;
	CButton receiveButton;
	afx_msg void OnBnClickedOk();
};