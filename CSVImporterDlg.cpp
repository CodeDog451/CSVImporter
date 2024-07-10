// CSVImporterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CSVImporter.h"
#include "CSVImporterDlg.h"
#include "FolderDlg.h"
#include <string>
#include <vector>
#include <algorithm>
using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
struct importParam
{
	std::vector< CString > filenames;
	CString sDirectory;
	HWND hWnd;
	HANDLE hEvent;
	HANDLE hNextEvent;	
};
struct lineParam
{
	string sLine;
	string sFilename;
	string sDirectory;
};
UINT ImportFileThread( LPVOID arguments )
{
	int iFinishedFile = 0;
	importParam * _param = ((importParam *) arguments);		
	HWND hWnd = _param->hWnd;
	bool bGo = true;
	bool bReadNextLine = true;
	for(int x = 0; x < _param->filenames.size(); x++)
	{
		ifstream file (_param->filenames[x].GetBuffer());
		if (file.is_open())
		{
			while (!file.eof())
			{
				Sleep(10);
				bGo = (WaitForSingleObject(_param->hEvent, 0) != WAIT_OBJECT_0);
				if(!bGo) 
				{
					SendMessage(hWnd, OB_IMPORT_DONE, 0, 1);//cancel reading file
					return 1;
				}
				bReadNextLine = (WaitForSingleObject(_param->hNextEvent, 0) != WAIT_OBJECT_0);
				if(bReadNextLine)
				{				
					string line;
					getline(file, line);
					lineParam * _lineParam = new lineParam();
					_lineParam->sLine = line;
					CString sFilename(_param->filenames[x]);
					sFilename.Replace(_param->sDirectory + "\\", "");
					sFilename = sFilename.Left(sFilename.GetLength()-4);
					_lineParam->sFilename = string(sFilename);
					_lineParam->sDirectory = _param->sDirectory;
					//SetEvent(_param->hNextEvent);// don't read next line until this one is processed by the server
					SendMessage(hWnd, OB_IMPORT_LINE, 0, (LPARAM)_lineParam);
				}
			}
			file.close();
		}

		//iFinishedFile = x + 1;
		lineParam * _lineParam = new lineParam();
		string sFilename(_param->filenames[x]);
		_lineParam->sFilename = sFilename;
		SendMessage(hWnd, OB_FILE_DONE, 0, (LPARAM)_lineParam);//stopped reading file
	}
	CloseHandle(_param->hEvent);
	SendMessage(hWnd, OB_IMPORT_DONE, 0, iFinishedFile);//stopped reading file
	_param->filenames.clear();
	delete _param;
	return 1;
}



void ListFilesInDirectory(LPCTSTR dirName, std::vector<CString> &filepaths, std::vector<CString> &filters )
{
	// Check input parameters
	ASSERT( dirName != NULL );

	// Clear filename list
	filepaths.clear();

	// Object to enumerate files
	CFileFind finder;

	// Build a string using wildcards *.*,
	// to enumerate content of a directory
	CString wildcard( dirName );
	wildcard += _T("\\*.*");

	// Init the file finding job
	BOOL working = finder.FindFile( wildcard );

	// For each file that is found:
	while ( working )
	{
		// Update finder status with new file
		working = finder.FindNextFile();

		// Skip '.' and '..'
		if ( finder.IsDots() )
		{
			continue;
		}

		// Skip sub-directories
		if ( finder.IsDirectory() )
		{
			continue;
		}
		if(filters.size() > 0)
		{
			std::vector<CString>::const_iterator it(std::find(filters.begin(), filters.end(), finder.GetFilePath().Right(3)));

			if(it != filters.end())
			{
				// Add file path to container
				filepaths.push_back(finder.GetFilePath());
			}		
			
		}
		else
		{
			// Add file path to container
			filepaths.push_back(finder.GetFilePath());
		}
	}

	// Cleanup file finder
	finder.Close();
}
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


// CCSVImporterDlg dialog




CCSVImporterDlg::CCSVImporterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCSVImporterDlg::IDD, pParent)
	, iDupe(0)
	, iInsert(0)
	, iError(0)
	, iSQLWaitingForServer(0)
	, iMaxWaiting(10)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCSVImporterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FEEDBACK, m_feedback);
	DDX_Control(pDX, IDC_CUSTOM_IMPORTED, m_imported_viewer);
	DDX_Control(pDX, IDC_CUSTOM_LOGIN, m_logon);
	DDX_Control(pDX, IDC_BUTTON_TEST, m_select);
	DDX_Control(pDX, IDC_STATIC_DUPE, m_label_dupe);
	DDX_Control(pDX, IDC_EDIT_DUPE, m_edit_dupe);
	DDX_Control(pDX, IDC_STATIC_INSERT, m_label_insert);
	DDX_Control(pDX, IDC_EDIT_INSERT, m_edit_insert);
	DDX_Control(pDX, IDC_STATIC_ERROR, m_label_error);
	DDX_Control(pDX, IDC_EDIT_ERROR, m_edit_error);
	DDX_Control(pDX, IDC_CHECK_SHOWDATA, m_showdata);
}

BEGIN_MESSAGE_MAP(CCSVImporterDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CCSVImporterDlg::OnBnClickedButtonConnect)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CCSVImporterDlg::OnBnClickedButtonTest)
	ON_NOTIFY(GVN_LOGIN, IDC_CUSTOM_LOGIN, OnGridViewLogin)
	ON_NOTIFY(GVN_LOGOUT, IDC_CUSTOM_LOGIN, OnGridViewLogout)
	ON_NOTIFY(GVN_ADD_RECORD, IDC_CUSTOM_IMPORTED, OnGridViewAddRecord)
	ON_MESSAGE(OB_IMPORT_LINE, OnImportLine)
	ON_MESSAGE(OB_IMPORT_DONE, OnImportStop)
	ON_MESSAGE(OB_FILE_DONE, OnImportFileDone)
	ON_MESSAGE(GVN_EN_STR, OnEncodedString)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_CHECK_SHOWDATA, &CCSVImporterDlg::OnBnClickedCheckShowdata)
END_MESSAGE_MAP()


// CCSVImporterDlg message handlers

BOOL CCSVImporterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

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

	// TODO: Add extra initialization here
	//hImportEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	//hNextEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	SetTimer(1, 10, 0);
	m_logon.AddServer("localhost", "127.0.0.1");
	m_logon.AddServer("A6 Local", "192.168.1.165");
	m_logon.AddServer("A6 Remote", "76.79.240.178");
	m_logon.m_grid.AutoSize();
	m_logon.m_grid.SelectRow(1);
	Client.RegisterGridviewer(IDD_CSVIMPORTER_DIALOG, m_logon.GetDlgCtrlID(), &m_logon);

	m_imported_viewer.SetCaption("Import Results");
	m_imported_viewer.SetHasBorder(true);
	m_imported_viewer.m_grid.SetFixedRowCount(1);
	
	m_imported_viewer.m_grid.SetColumnCount(12);
	
	m_imported_viewer.m_grid.SetItemText(0,0, "Parser ID");
	//m_imported_viewer.m_grid.SetColumnWidth(0, 0);//hidden
	m_imported_viewer.m_grid.SetItemText(0,1, "Result");
	m_imported_viewer.m_grid.SetItemText(0,2, "First Name");
	m_imported_viewer.m_grid.SetItemText(0,3, "Last Name");
	m_imported_viewer.m_grid.SetItemText(0,4, "Address");
	m_imported_viewer.m_grid.SetItemText(0,5, "City");
	m_imported_viewer.m_grid.SetItemText(0,6, "State");
	m_imported_viewer.m_grid.SetItemText(0,7, "zip");
	m_imported_viewer.m_grid.SetItemText(0,8, "Phone");
	m_imported_viewer.m_grid.SetItemText(0,9, "Email");
	m_imported_viewer.m_grid.SetItemText(0,10, "Order ID");	
	m_imported_viewer.m_grid.SetItemText(0,11, "");	
	m_imported_viewer.m_grid.SetHeaderSort(true);
	m_imported_viewer.m_grid.SetSingleRowSelection(true);	
	
	m_imported_viewer.m_grid.SetEditable(false);
	m_imported_viewer.m_grid.EnableHiddenColUnhide(false);	
	m_imported_viewer.m_grid.SetListMode(true);	
	//m_imported_viewer.SetShowStop(false);
	m_imported_viewer.SetShowDataButtons(false);
	m_imported_viewer.m_grid.AutoSize();
	Client.RegisterGridviewer(IDD_CSVIMPORTER_DIALOG, IDC_CUSTOM_IMPORTED, &m_imported_viewer);
	//Client.RegisterDataReceiver(IDD_CSVIMPORTER_DIALOG, IDC_CUSTOM_IMPORTED, GetSafeHwnd());

	//BST_CHECKED == m_showdata.GetCheck()
	m_showdata.SetCheck(BST_UNCHECKED);
	m_imported_viewer.SetShowData(false);

	DoLayoutOnSize();
	
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}
LRESULT CCSVImporterDlg::OnEncodedString(WPARAM wp, LPARAM lp)
{
	//PrintOut("Got Encoded String Message");
	//RE_STR * re_str = (RE_STR*)lp;
	//do stuff
	/*CRecordEncoder aEncoder;
	aEncoder.Decode(re_str->sEncodedString);
	iSQLWaitingForServer--;
	if(iSQLWaitingForServer < iMaxWaiting)
	{
		ResetEvent(hNextEvent);
	}
	
	
	//PrintOut(sResult);
	HandleResult(aEncoder.GetAt(1).c_str());*/	
	
	return LRESULT();
}


LRESULT CCSVImporterDlg::OnImportLine(WPARAM wp, LPARAM lp)
{	
	lineParam * _param = ((lineParam *) lp);
	//PrintOut(CString(_param->sLine.c_str()));	
	CString sLine(_param->sLine.c_str());
	sLine.Replace(",", "\t");
	sLine.Replace("'", "`");
	sLine.Replace("\"", "`");

	
	CRecordEncoder aEncoder;
	aEncoder.Decode(sLine.GetBuffer(), false);

	string sEmail = aEncoder.GetAt(7).substr(0, 45);
	if(find(sEmail.begin(), sEmail.end(), '@') != sEmail.end())
	{
		CString sPhone = StripNonDigits(aEncoder.GetAt(6).substr(0, 45).c_str());//@phone

		string sSql = "EXECUTE	ImportCSV '";
		sSql.append(aEncoder.GetAt(0).substr(0, 255));//@lastname 		
		sSql.append("', '");sSql.append(aEncoder.GetAt(1).substr(0, 45));//@firstname 
		sSql.append("', '");sSql.append(aEncoder.GetAt(2).substr(0, 45));//@address
		sSql.append("', '");sSql.append(aEncoder.GetAt(3).substr(0, 45));//@city
		sSql.append("', '");sSql.append(aEncoder.GetAt(4).substr(0, 45));//@state
		sSql.append("', '");sSql.append(aEncoder.GetAt(5).substr(0, 45));//@zip
		sSql.append("', '");sSql.append(sPhone.GetBuffer());//@phone
		sSql.append("', '");sSql.append(sEmail);//@email
		sSql.append("', '");sSql.append(_param->sFilename.substr(0, 20));//@filename
		sSql.append("'");
		CRecordEncoder aImportEncoder = Client.GetSQLEncoder(IDD, m_imported_viewer.GetDlgCtrlID(), true, false, sSql);
		Client.ExecSQL(aImportEncoder);

		iSQLWaitingForServer++;
		//SetEvent(hNextEvent);
		if(iSQLWaitingForServer >= iMaxWaiting)
		{
			SetEvent(hNextEvent);
		}
	}
	else
	{
		//no email then output to error file
		string sErrFilename = _param->sDirectory + "\\" + _param->sFilename + ".err";
		ofstream out(sErrFilename.c_str(), ios::app); 
		if(!out) 
		{ 
			PrintOut("Cannot open file to write");			
		}
		else
		{
			HandleResult("Error");
			//PrintOut(_param->sLine.c_str());
			out << _param->sLine.c_str() << endl;
			
			//out << "This is a short text file." << endl; 
		}
		out.close();		  
		 
		  
	}
	delete _param;
	return LRESULT();
}
void CCSVImporterDlg::OnGridViewAddRecord(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;   
	iSQLWaitingForServer--;
	if(iSQLWaitingForServer < iMaxWaiting)
	{
		ResetEvent(hNextEvent);
	}
	//CString sResult = m_imported_viewer.m_grid.GetItemText(pItem->iRow, 1);
	RE_STR * re_str = (RE_STR*)pItem->iColumn;
	CRecordEncoder aEncoder;
	aEncoder.Decode(re_str->sEncodedString);
	
	//PrintOut(aEncoder.GetAt(1).c_str());
	HandleResult(aEncoder.GetAt(1).c_str());
	if(m_imported_viewer.m_grid.GetRowCount()>=25)
	{
		m_imported_viewer.m_grid.DeleteRow(1);
	}
}

void CCSVImporterDlg::OnGridViewLogout(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEWLOGIN* pItem = (NM_GRIDVIEWLOGIN*) pNotifyStruct;  
	Client.Disconnect(IDD_CSVIMPORTER_DIALOG, m_logon.GetDlgCtrlID());
	
}
void CCSVImporterDlg::OnGridViewLogin(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEWLOGIN* pItem = (NM_GRIDVIEWLOGIN*) pNotifyStruct;  
	Client.Connect(IDD_CSVIMPORTER_DIALOG, m_logon.GetDlgCtrlID(), pItem->sIP.GetBuffer(), "10008", pItem->sUsername.GetBuffer(), pItem->sPassword.GetBuffer());
	
}
LRESULT CCSVImporterDlg::OnImportStop(WPARAM wp, LPARAM lp)
{	
	PrintOut("Import Done");
	return LRESULT();
}
LRESULT CCSVImporterDlg::OnImportFileDone(WPARAM wp, LPARAM lp)
{
	lineParam * _param = ((lineParam *) lp);
	CString sStr(_param->sFilename.c_str());
	PrintOut(sStr + CString(": File Done"));
	delete _param;
	return LRESULT();
}
void CCSVImporterDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCSVImporterDlg::OnPaint()
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
HCURSOR CCSVImporterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CCSVImporterDlg::OnBnClickedButtonConnect()
{
	
	//Client.Connect("127.0.0.1", "10008", "peterson", "hathor");
}

void CCSVImporterDlg::PrintOut(CString sMsg)
{
	CString sNewline = "\r\n";	
	CString sFeedback = "";	
	m_feedback.GetWindowText(sFeedback);
	sFeedback = sMsg + sNewline + sFeedback;
	
	sFeedback = sFeedback.Left(4080);

	m_feedback.SetWindowText(sFeedback);
}

void CCSVImporterDlg::OnTimer(UINT_PTR nIDEvent)
{
	Client.ClientLoop();
	CString sFeedback(Client.GetFeedback().c_str());
	if(sFeedback.GetLength() > 0) PrintOut(sFeedback);
	CDialog::OnTimer(nIDEvent);
}

void CCSVImporterDlg::OnBnClickedButtonTest()
{
	CString m_strFolderPath = _T( "c:\\" ); // Just for sample    

    CString m_strDisplayName = "";
    
    CFolderDialog dlg(  _T( "Select Import From Directory" ), m_strFolderPath, this );
    if( dlg.DoModal() == IDOK  )
    {    
        m_strFolderPath  = dlg.GetFolderPath();
		m_strDisplayName = dlg.GetFolderName();
        // Use folder path and display name here ...
		PrintOut(m_strFolderPath);
		PrintOut(m_strDisplayName);
		CString directory = m_strFolderPath;
		std::vector< CString > filters;
		filters.push_back("csv");
		filters.push_back("txt");
		importParam * _param = new importParam();	
			
		_param->hWnd = m_hWnd;
		_param->sDirectory = directory;
		ListFilesInDirectory(directory, _param->filenames, filters);
		//ResetEvent(hImportEvent);
		//ResetEvent(hNextEvent);
		hImportEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		hNextEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		_param->hEvent = hImportEvent;
		_param->hNextEvent = hNextEvent;
		iDupe = 0;
		iInsert = 0;
		iError = 0;
		m_imported_viewer.m_grid.ClearGrid();
		AfxBeginThread(ImportFileThread, _param);

		//for(int x = 0; x < filenames.size(); x++)
		//{
			//PrintOut(filenames[x]);
		//}

    }    


	
	
	//CRecordEncoder aEncoder = Client.GetSQLEncoder(IDD, m_imported_viewer.GetDlgCtrlID(), true, false, "SELECT iUserID, sFirstname, sLastName FROM dbo.tblUsers");
	//Client.ExecSQL(aEncoder);
}

void CCSVImporterDlg::DoLayoutOnSize(void)
{
	RECT rc, viewrc, feedbackrc, logonrc;
	
	GetClientRect(&rc);
	GetClientRect(&viewrc);
	GetClientRect(&feedbackrc);	
	GetClientRect(&logonrc);
	int iMargin = 5;
	int iTop = 30;
	int iRight = 300;
	int iBottom = 200;
	logonrc.top = iMargin;
	logonrc.left = iMargin;
	logonrc.right = iRight;
	logonrc.bottom = iBottom;
	if(m_logon.GetSafeHwnd() != NULL)
	{
		m_logon.MoveWindow(&logonrc);
	}
	feedbackrc.top = iTop + iMargin;
	feedbackrc.left = iRight + iMargin;
	feedbackrc.right = rc.right - iMargin;
	feedbackrc.bottom = iBottom;
	if(m_feedback.GetSafeHwnd() != NULL)
	{
		m_feedback.MoveWindow(&feedbackrc);
	}
	viewrc.top = iBottom + iMargin;
	viewrc.left = iMargin;
	viewrc.right = rc.right - iMargin;;
	viewrc.bottom = rc.bottom - iMargin;
	if(m_imported_viewer.GetSafeHwnd() != NULL)
	{
		m_imported_viewer.MoveWindow(&viewrc);
	}
	MoveControl(&m_select, iMargin, iRight + iMargin);
	int iSelectWidth = 146;
	int iCountsLoc = iRight + iMargin + iSelectWidth;
	int iLabelWidth = 22;
	int iEditWidth = 45;
	//MoveControl(&m_label_dupe, iMargin, iCountsLoc);
	//MoveControl(&m_edit_dupe, iMargin, iCountsLoc + iLabelWidth + iMargin);

	//MoveControl(&m_label_insert, iMargin, iCountsLoc + iLabelWidth + iMargin + iEditWidth + iMargin);
	//MoveControl(&m_edit_insert, iMargin, iCountsLoc + iLabelWidth + iMargin + iEditWidth + iMargin + iLabelWidth + iMargin + 5);

	//MoveControl(&m_label_error, iMargin, iCountsLoc + iLabelWidth + iMargin + iEditWidth + iMargin + iLabelWidth + iMargin+ iEditWidth + iMargin + 5);
	//MoveControl(&m_edit_error, iMargin, iCountsLoc + iLabelWidth + iMargin + iEditWidth + iMargin + iLabelWidth + iMargin+ iEditWidth + iMargin + iLabelWidth + iMargin + 4);
	
}

void CCSVImporterDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	DoLayoutOnSize();
}

void CCSVImporterDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);

	m_logon.SetFocusOnUsername();
}

void CCSVImporterDlg::HandleResult(CString sResult)
{
	if(sResult == "Dupe") iDupe++;
	else if(sResult == "Insert") iInsert++;
	else if(sResult == "Error") iError ++;

	char buffer[65];

	_itoa( iDupe, buffer, 10 ); 
	CString sDupe(buffer);

	_itoa( iInsert, buffer, 10 ); 
	CString sInsert(buffer);

	_itoa( iError, buffer, 10 ); 
	CString sError(buffer);

	m_edit_dupe.SetWindowTextA(sDupe);
	m_edit_insert.SetWindowTextA(sInsert);
	m_edit_error.SetWindowTextA(sError);
}

void CCSVImporterDlg::OnBnClickedCheckShowdata()
{
	if(BST_CHECKED == m_showdata.GetCheck())
	{
		m_imported_viewer.SetShowData(true);
		PrintOut("Setting Show Data to True");
	}
	else
	{
		m_imported_viewer.SetShowData(false);
		PrintOut("Setting Show Data to False");
	}
}
