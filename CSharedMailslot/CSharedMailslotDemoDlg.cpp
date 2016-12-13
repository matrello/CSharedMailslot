// CSharedMailslotDemoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CSharedMailslotDemo.h"
#include ".\csharedmailslotdemodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CSharedMailslotDemoDlg dialog



CSharedMailslotDemoDlg::CSharedMailslotDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSharedMailslotDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSharedMailslotDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MESSAGE, messageTextBox);
	DDX_Control(pDX, IDC_MAILSLOT, mailslotNameTextBox);
	DDX_Control(pDX, IDC_FROMTO, fromtoTextBox);
	DDX_Control(pDX, IDC_FROMTOLB, fromtoLabel);
	DDX_Control(pDX, IDC_MESSENGERFORMAT, messngrFormatCheckBox);
	DDX_Control(pDX, IDC_ONQUEUE, onqueueLabel);
	DDX_Control(pDX, IDC_SHARED, sharedCheckBox);
	DDX_Control(pDX, IDC_THREADID, threadidLabel);
	DDX_Control(pDX, IDC_REOPEN, reopenButton);
	DDX_Control(pDX, IDC_RECEIVE, receiveButton);
}

BEGIN_MESSAGE_MAP(CSharedMailslotDemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_RECEIVE, OnBnClickedReceive)
	ON_BN_CLICKED(IDC_SEND, OnBnClickedSend)
	ON_EN_CHANGE(IDC_MESSAGE, OnEnChangeMessage)
	ON_BN_CLICKED(IDC_REOPEN, OnBnClickedReopen)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CSharedMailslotDemoDlg message handlers

BOOL CSharedMailslotDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	char threadId[8], threadMsg[128];
	itoa(GetCurrentThreadId(), threadId, 16);
	strcpy(threadMsg, "thread\n");
	strcat(threadMsg, threadId);
	threadidLabel.SetWindowText(threadMsg);

	GetWorkstationName();
	fromtoTextBox.SetWindowText("MATRO");
	mailslotNameTextBox.SetWindowText("CPDEMO");
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSharedMailslotDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSharedMailslotDemoDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSharedMailslotDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSharedMailslotDemoDlg::OnBnClickedReceive()
{
	CString message, onqueue;
	char *buffer=NULL, *bufferField;
	DWORD bufferSize=0, messagesWaiting=0;

	fromtoLabel.SetWindowText("from:");

	if (mailslotServer.Read(buffer, bufferSize, messagesWaiting))
	{
		onqueue.Format("%d on queue", messagesWaiting);
		onqueueLabel.SetWindowText(onqueue.GetBuffer());
		onqueue.ReleaseBuffer();

		if (bufferSize==0)
		{
			messageTextBox.SetWindowText("");
			fromtoTextBox.SetWindowText("");
		}
		else
		{
			bufferField=buffer;
			if (messngrFormatCheckBox.GetCheck()==BST_CHECKED)
			{
				fromtoTextBox.SetWindowText(bufferField);
				bufferField+=strlen(bufferField)+1;

				// 'To' field is not used here
				bufferField+=strlen(bufferField)+1;

				// perform an OEM to ANSI conversion
				message=bufferField;
				message.OemToAnsi();
				messageTextBox.SetWindowText(message.GetBuffer());
				message.ReleaseBuffer();
			}
			else
			{
				messageTextBox.SetWindowText(buffer);
				fromtoTextBox.SetWindowText("<unknown>");
			}
		}
	}
}

void CSharedMailslotDemoDlg::OnBnClickedSend()
{
	CSharedMailslot sendMailslot;
	CString textToWrite, toName;

	fromtoLabel.SetWindowText("to:");

	fromtoTextBox.GetWindowText(toName);

	if (messngrFormatCheckBox.GetCheck()==BST_CHECKED)
	{
		CString messageBody;
		messageTextBox.GetWindowText(messageBody);

		// perform an ANSI to OEM conversion
		messageBody.AnsiToOem();

		textToWrite=workstationName + (char)0 + toName + (char)0 + messageBody;
		sendMailslot.Write(textToWrite.GetBuffer(), textToWrite.GetLength()+1, mailslotServer.GetMailslotName(), toName.GetBuffer());
	}
	else
	{
		messageTextBox.GetWindowText(textToWrite);
		sendMailslot.Write(textToWrite.GetBuffer(), textToWrite.GetLength()+1, mailslotServer.GetMailslotName(), toName.GetBuffer());
	}

	textToWrite.ReleaseBuffer();
	toName.ReleaseBuffer();
}

void CSharedMailslotDemoDlg::OnEnChangeMessage()
{
	fromtoLabel.SetWindowText("to:");
}

void CSharedMailslotDemoDlg::OnBnClickedReopen()
{
	CString mailslotName;

	if (mailslotServer.isOpen())
		mailslotServer.Close();

	mailslotNameTextBox.GetWindowText(mailslotName);
	if (mailslotServer.Open(true, mailslotName.GetBuffer(), NULL, (sharedCheckBox.GetCheck()==BST_CHECKED)))
	{
		reopenButton.SetWindowText("reopen");
		receiveButton.EnableWindow(true);
	}
	else
	{
		reopenButton.SetWindowText("open");
		receiveButton.EnableWindow(false);
		AfxMessageBox("could not open the mailslot", MB_OK+MB_ICONEXCLAMATION);
	}

	mailslotName.ReleaseBuffer();
}

void CSharedMailslotDemoDlg::GetWorkstationName()
{
	DWORD dwCount=255;
	HKEY key=NULL;
	int pos=0;

	workstationName="CODEPROJECT";
	
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\control\\ComputerName\\ComputerName", 0, KEY_READ, &key)==ERROR_SUCCESS)
		RegQueryValueEx(key, "ComputerName", NULL, NULL, (LPBYTE) workstationName.GetBuffer(255), &dwCount);

	workstationName.ReleaseBuffer();
	workstationName.MakeUpper();
	RegCloseKey(key);
}
void CSharedMailslotDemoDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}
