#include "uiMainFrame.h"

#include "uiLogin.h"

CMainFrame* g_mainFrame{nullptr};

CMainFrame::CMainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {
    auto* panel = new wxPanel(this, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_book = new wxSimplebook(panel, wxID_ANY);
    m_loginWindow = new CLoginWindow(m_book);
    m_book->AddPage(m_loginWindow, "Login");

    sizer->Add(m_book, 1, wxEXPAND);
    panel->SetSizer(sizer);
}

bool CMgramEntry::OnInit() {
    g_mainFrame = new CMainFrame("MGram");
    g_mainFrame->Show(true);
    return true;
}
