#include "uiLogin.h"

#include <wx/string.h>

CMainFrame::CMainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {
    m_panel = new wxPanel(this, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* optionSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* description =
        new wxTextCtrl(m_panel, wxID_ANY, "Log in to Telegram by QR Code or phone number", wxDefaultPosition,
                       wxDefaultSize, wxTE_READONLY | wxTC_MULTILINE, wxDefaultValidator);
    m_login = new wxButton(m_panel, wxID_ANY, "Log in by phone number", wxDefaultPosition, wxDefaultSize, 0);
    textSizer->Add(description);
    optionSizer->Add(m_login);
    mainSizer->Add(textSizer);
    mainSizer->Add(optionSizer);
    m_panel->SetSizer(mainSizer);
}

bool CMgramEntry::OnInit() {
    auto* frame = new CMainFrame("MGram");
    frame->Show(true);
    return true;
}
