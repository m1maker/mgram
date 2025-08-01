#include "uiMainFrame.h"

#include "uiLogin.h"

#include <wx/string.h>

CMainFrame* g_mainFrame{nullptr};

CMainFrame::CMainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {}

bool CMgramEntry::OnInit() {
    g_mainFrame = new CMainFrame("MGram");
    auto* loginWindow = new CLoginWindow();
    g_mainFrame->Show(true);
    return true;
}
