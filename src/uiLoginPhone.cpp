#include "uiLoginPhone.h"

#include "uiMainFrame.h"

CLoginPhoneWindow::CLoginPhoneWindow(bool useTestDataCenter) : wxPanel(g_mainFrame, wxID_ANY) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* optionSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* phoneNumberLable = new wxStaticText(this, wxID_ANY, "Phone Number");
    m_phoneNumber = new wxTextCtrl(this, wxID_ANY, "+", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_next = new wxButton(this, wxID_ANY, "&Next");
    m_cancel = new wxButton(this, wxID_ANY, "&Cancel");

    textSizer->Add(phoneNumberLable);
    textSizer->Add(m_phoneNumber);
    optionSizer->Add(m_next);
    optionSizer->Add(m_cancel);
    mainSizer->Add(textSizer);
    mainSizer->Add(optionSizer);
    this->SetSizer(mainSizer);
    m_cancel->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnCancelPressed, this);
    m_phoneNumber->SetFocus();
}

void CLoginPhoneWindow::OnCancelPressed(wxCommandEvent& event) {
    this->Destroy();
    auto* loginWindow = new CLoginWindow();
}
