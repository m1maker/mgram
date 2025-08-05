#include "uiLogin.h"

#include "uiLoginPhone.h"
#include "uiMainFrame.h"

CLoginWindow::CLoginWindow() : wxPanel(g_mainFrame, wxID_ANY) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* optionSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* text = new wxTextCtrl(this, wxID_ANY, "Log in to Telegram by phone number or QR Code", wxDefaultPosition,
                                wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxTE_DONTWRAP);
    m_login = new wxButton(this, wxID_ANY, "Log in by phone number");
    m_useTestDataCenter = new wxCheckBox(this, wxID_ANY, "Use Test DC");
    textSizer->Add(text);
    optionSizer->Add(m_login);
    optionSizer->Add(m_useTestDataCenter);
    mainSizer->Add(textSizer);
    mainSizer->Add(optionSizer);
    this->SetSizer(mainSizer);
    m_login->Bind(wxEVT_BUTTON, &CLoginWindow::OnLoginPressed, this);
    text->SetFocus();
}

void CLoginWindow::OnLoginPressed(wxCommandEvent& event) {
    bool useTestDataCenter = m_useTestDataCenter->IsChecked();
    this->Destroy();
    auto* loginPhoneWindow = new CLoginPhoneWindow(useTestDataCenter);
}
