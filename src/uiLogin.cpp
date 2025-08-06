#include "uiLogin.h"

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

    auto set_params = td::td_api::make_object<td::td_api::setTdlibParameters>();
    set_params->api_id_ = 12345;
    set_params->api_hash_ = "";
    set_params->database_directory_ = "tdlib";
    set_params->use_test_dc_ = useTestDataCenter;
    set_params->device_model_ = "Desktop";
    set_params->system_language_code_ = "en";
    set_params->application_version_ = "0.1";

    g_mainFrame->getTdManager()->send(std::move(set_params), nullptr);
    if (m_book->FindPage(g_mainFrame->m_loginPhoneWindow) == wxNOT_FOUND) {
        g_mainFrame->m_loginPhoneWindow = new CLoginPhoneWindow(m_book);
        m_book->AddPage(g_mainFrame->m_loginPhoneWindow, "LoginPhone");
    }
    m_book->SetSelection(m_book->FindPage(g_mainFrame->m_loginPhoneWindow));
}
