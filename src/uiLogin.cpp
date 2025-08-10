#include "uiLogin.h"

#include "constants.h"
#include "uiLoginPhone.h"
#include "uiMainFrame.h"

CLoginWindow::CLoginWindow(wxSimplebook* book) : wxPanel(book, wxID_ANY), m_book(book) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* optionSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* text = new wxTextCtrl(this, wxID_ANY, "Log in to Telegram by phone number or QR Code", wxDefaultPosition,
                                wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxTE_DONTWRAP);
    m_login = new wxButton(this, wxID_ANY, "Log in by phone number");
    m_useTestDataCenter = new wxCheckBox(this, wxID_ANY, "Use Test DC");

    textSizer->Add(text, 1, wxEXPAND | wxALL, 5);
    optionSizer->Add(m_login, 0, wxALL, 5);
    optionSizer->Add(m_useTestDataCenter, 0, wxALL, 5);
    mainSizer->Add(textSizer, 1, wxEXPAND);
    mainSizer->Add(optionSizer, 0, wxALIGN_CENTER);

    SetSizerAndFit(mainSizer);

    m_login->Bind(wxEVT_BUTTON, &CLoginWindow::OnLoginPressed, this);
    text->SetFocus();
}

extern CMainFrame* g_mainFrame;

void CLoginWindow::OnLoginPressed(wxCommandEvent& event) {
    bool useTestDataCenter = m_useTestDataCenter->IsChecked();

    if (m_book->FindPage(g_mainFrame->m_loginPhoneWindow) == wxNOT_FOUND) {
        m_book->AddPage(g_mainFrame->m_loginPhoneWindow, "LoginPhone");
    }
    m_book->SetSelection(m_book->FindPage(g_mainFrame->m_loginPhoneWindow));
}
