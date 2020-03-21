// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "ProPerforate.h"

#include "MainFrm.h"
#include "WellDocument.h"
#include "CurveChartView.h"
#include "CompressTool.h"
#include "PulsFrac.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame
// Test
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	// 全局帮助命令
	ON_COMMAND(ID_HELP_FINDER, &CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, &CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, &CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, &CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_SWITCH_VIEW, &CMainFrame::OnSwitchView)
  ON_UPDATE_COMMAND_UI(ID_SWITCH_VIEW, &CMainFrame::OnUpdateSwitchView)
	ON_COMMAND(ID_CALC_RUN, &CMainFrame::OnCalcRun)
	ON_UPDATE_COMMAND_UI(ID_CALC_RUN, &CMainFrame::OnUpdateCalcRun)
	ON_COMMAND(ID_CALC_STOP, &CMainFrame::OnCalcStop)
	ON_UPDATE_COMMAND_UI(ID_CALC_STOP, &CMainFrame::OnUpdateCalcStop)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // 状态行指示器
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	deactive_view_ = nullptr;
  hSemaphore = NULL;

  calcRun = false;
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("未能创建状态栏\n");
		return -1;      // 未能创建
	}

	// 创建三个额外的工具条
	if (!calculate_toolbar_.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!calculate_toolbar_.LoadToolBar(IDR_TOOLBAR_CALCULATE))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}
	if (!well_toolbar_.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!well_toolbar_.LoadToolBar(IDR_TOOLBAR_WELL))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}
	if (!chart_toolbar_.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!chart_toolbar_.LoadToolBar(IDR_TOOLBAR_CHART))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}

	//如果不需要工具栏可停靠，则删除这三行
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	calculate_toolbar_.EnableDocking(CBRS_ALIGN_ANY);
	well_toolbar_.EnableDocking(CBRS_ALIGN_ANY);
	chart_toolbar_.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	DockControlBarLeftOf(&calculate_toolbar_, &m_wndToolBar);
	DockControlBarLeftOf(&well_toolbar_, &calculate_toolbar_);
	DockControlBarLeftOf(&chart_toolbar_, &well_toolbar_);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}


// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame 消息处理程序

CView* CMainFrame::getDeactiveView() {
  if(deactive_view_ == nullptr) {
    ASSERT_VALID(GetActiveDocument());
	  CCreateContext newContext;
    newContext.m_pNewViewClass = NULL;
    newContext.m_pNewDocTemplate = NULL;
    newContext.m_pLastView = NULL;
    newContext.m_pCurrentFrame = this;
    newContext.m_pCurrentDoc = GetActiveDocument();
    deactive_view_ = new CurveChartView();
  	deactive_view_->Create(NULL, _T("曲线图"), AFX_WS_DEFAULT_VIEW,CRect(0,0,0,0), this, AFX_IDW_PANE_FIRST+1, &newContext );
    deactive_view_->SendMessage(WM_INITIALUPDATE, 0, 0);
  	deactive_view_->ShowWindow(SW_HIDE);
  }
  return deactive_view_;
}
void CMainFrame::OnSwitchView()
{
	CView *active_view = GetActiveView();
	ASSERT(active_view != nullptr);	//理论上不会发生
	int active_id = active_view->GetDlgCtrlID();
	ASSERT(active_id != 0);
	active_view->SetDlgCtrlID(active_id + 1);

  deactive_view_ = getDeactiveView();
  ASSERT(deactive_view_ != nullptr);
	deactive_view_->SetDlgCtrlID(active_id);

	SetActiveView(deactive_view_);

	deactive_view_->ShowWindow(SW_SHOW);
	active_view->ShowWindow(SW_HIDE);

	deactive_view_ = active_view;
	RecalcLayout();
}
void CMainFrame::OnUpdateSwitchView(CCmdUI *pCmdUI) {
  pCmdUI->Enable(!calcRun);
}
// 工作线程
UINT startCalculateThread(LPVOID param)
{
	ShellLogic *shell = static_cast<ShellLogic*>(param);
  shell->run();
	return 0;
}
struct TRANS { HANDLE hSemaphore; ShellLogic *shell; };
UINT refreshThread(LPVOID param) {
  struct TRANS *tran = static_cast<struct TRANS*>(param);
  HANDLE hSema = tran->hSemaphore;
  ShellLogic *shell = tran->shell;
  while(WAIT_TIMEOUT == WaitForSingleObject(hSema, 210)) {//3分半钟
    TRACE("Refresh the data ...............\n");
    shell->pauseRun();
    break;
  }
  TRACE("Stop refreshing ...........\n");
  return 0;
}
void CMainFrame::OnCalcRun()
{
	CProPerforateApp *myapp = static_cast<CProPerforateApp*>(AfxGetApp());
  AfxBeginThread(startCalculateThread, myapp->shell);	// 启动计算线程
  hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
  struct TRANS trans = {hSemaphore, myapp->shell};
  AfxBeginThread(refreshThread, &trans);

  CView *act = GetActiveView();
  if(!act->IsKindOf(RUNTIME_CLASS(CurveChartView)))
    OnSwitchView();
  CurveChartView *ccv = static_cast<CurveChartView*>(GetActiveView());
  ccv->run();

  calcRun = true;
}

void CMainFrame::OnUpdateCalcRun(CCmdUI *pCmdUI)
{
  //auto puls = CPulsFrac::RunPulsFrac();
  pCmdUI->Enable(!calcRun);// && puls->IsModule());
}

UINT stopCalculateThread(LPVOID param)
{
	ShellLogic *shell = static_cast<ShellLogic*>(param);
  shell->stop();
  return 0;
}

void CMainFrame::OnCalcStop()
{
  ReleaseSemaphore(hSemaphore, 1, NULL); //terminate refreshThread()
  CloseHandle(hSemaphore);

  CurveChartView *ccv = static_cast<CurveChartView*>(GetActiveView());
  ccv->stop();

	CProPerforateApp *myapp = static_cast<CProPerforateApp*>(AfxGetApp());

  AfxBeginThread(stopCalculateThread, myapp->shell);	// 停止计算线程

  calcRun = false;
}

void CMainFrame::OnUpdateCalcStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(calcRun);
}

// 一个来自codeproject的私有方法
void CMainFrame::DockControlBarLeftOf(CToolBar *Bar, CToolBar *LeftOf)
{
	CRect rect;
	DWORD dw;
	UINT n;

	// get MFC to adjust the dimensions of all docked ToolBars

	// so that GetWindowRect will be accurate

	RecalcLayout(TRUE);

	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(1,0);
	dw=LeftOf->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;

	// When we take the default parameters on rect, DockControlBar will dock

	// each Toolbar on a seperate line. By calculating a rectangle, we

	// are simulating a Toolbar being dragged to that location and docked.

	DockControlBar(Bar,n,&rect);
}
