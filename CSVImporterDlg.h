// CSVImporterDlg.h : header file
//

#pragma once
#include "gridlogon.h"
#include "gridviewer.h"
#include "rtdbeclient.h"
#include "afxwin.h"

// CCSVImporterDlg dialog
class CCSVImporterDlg : public CDialog, public CUtility
{
// Construction
public:
	CCSVImporterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CSVIMPORTER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	CRTDBEClient Client;

	HANDLE hImportEvent;
	HANDLE hNextEvent;

public:
	afx_msg void OnBnClickedButtonConnect();
	CEdit m_feedback;
	void PrintOut(CString sMsg);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CGridViewer m_imported_viewer;
	afx_msg void OnBnClickedButtonTest();
	LRESULT OnImportLine(WPARAM wp, LPARAM lp);
	LRESULT OnImportStop(WPARAM wp, LPARAM lp);
	LRESULT OnImportFileDone(WPARAM wp, LPARAM lp);
	LRESULT OnEncodedString(WPARAM wp, LPARAM lp);

	CGridLogon m_logon;
	afx_msg void OnGridViewLogin(NMHDR *pNotifyStruct, LRESULT* pResult);//
	afx_msg void OnGridViewLogout(NMHDR *pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridViewAddRecord(NMHDR *pNotifyStruct, LRESULT* pResult);
	void DoLayoutOnSize(void);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CButton m_select;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	int iDupe;
	int iInsert;
	void HandleResult(CString sResult);
	int iError;
	CStatic m_label_dupe;
	CEdit m_edit_dupe;
	CStatic m_label_insert;
	CEdit m_edit_insert;
	CStatic m_label_error;
	CEdit m_edit_error;
	int iSQLWaitingForServer;
	int iMaxWaiting;
	CButton m_showdata;
	afx_msg void OnBnClickedCheckShowdata();
};
