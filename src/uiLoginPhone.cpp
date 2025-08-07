#include "uiLoginPhone.h"

#include "uiMainFrame.h"

#include <td/telegram/td_api.hpp>

extern CMainFrame* g_mainFrame;

CLoginPhoneWindow::CLoginPhoneWindow(wxSimplebook* book)
    : wxPanel(book, wxID_ANY), m_book(book), m_loginState(LOGIN_PHONE) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    m_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_entry = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_next = new wxButton(this, wxID_ANY, "&Next");
    m_cancel = new wxButton(this, wxID_ANY, "&Cancel");

    mainSizer->Add(m_label, 0, wxALL, 5);
    mainSizer->Add(m_entry, 0, wxEXPAND | wxALL, 5);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_next, 0, wxALL, 5);
    buttonSizer->Add(m_cancel, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxTOP, 10);

    SetSizerAndFit(mainSizer);

    m_next->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnNextPressed, this);
    m_cancel->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnCancelPressed, this);

    SwitchLoginState(m_loginState);

    g_mainFrame->getTdManager()->setUpdateCallback([this](td::td_api::object_ptr<td::td_api::Object> update) {
        if (update->get_id() == td::td_api::updateAuthorizationState::ID) {
            auto auth_state = td::td_api::move_object_as<td::td_api::updateAuthorizationState>(update);
            if (auth_state->authorization_state_->get_id() == td::td_api::authorizationStateWaitCode::ID) {
                CallAfter([this]() { SwitchLoginState(LOGIN_CODE); });
            } else if (auth_state->authorization_state_->get_id() == td::td_api::authorizationStateReady::ID) {
            }
        }
    });
}

void CLoginPhoneWindow::SwitchLoginState(const ELoginState& newState) {
    m_loginState = newState;
    switch (m_loginState) {
        case LOGIN_PHONE:
            m_label->SetLabelText("Enter your phone number:");
            m_entry->SetValue("");
            m_entry->SetHint("+12025550132");
            break;
        case LOGIN_CODE:
            m_label->SetLabelText("Enter the code you received:");
            m_entry->SetValue("");
            m_entry->SetHint("12345");
            break;
        case LOGIN_PASSWORD:
            m_label->SetLabelText("Enter your two-factor authentication password:");
            m_entry->SetValue("");
            m_entry->SetHint("Password");
            m_entry->SetWindowStyle(m_entry->GetWindowStyle() | wxTE_PASSWORD);
            break;
    }
    m_entry->SetFocus();
    Layout();
}

void CLoginPhoneWindow::OnNextPressed(wxCommandEvent& event) {
    wxString value = m_entry->GetValue();

    if (m_loginState == LOGIN_PHONE) {
        auto set_phone_number = td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>();
        set_phone_number->phone_number_ = value.ToStdString();

        g_mainFrame->getTdManager()->send(std::move(set_phone_number),
                                          [](td::td_api::object_ptr<td::td_api::Object> object) {});

    } else if (m_loginState == LOGIN_CODE) {
        auto check_code = td::td_api::make_object<td::td_api::checkAuthenticationCode>();
        check_code->code_ = value.ToStdString();

        g_mainFrame->getTdManager()->send(std::move(check_code), [](td::td_api::object_ptr<td::td_api::Object> object) {
            if (object->get_id() == td::td_api::error::ID) {}
        });
    }
}

void CLoginPhoneWindow::OnCancelPressed(wxCommandEvent& event) {
    SwitchLoginState(LOGIN_PHONE);

    if (m_book->FindPage(g_mainFrame->m_loginWindow) != wxNOT_FOUND) {
        m_book->SetSelection(m_book->FindPage(g_mainFrame->m_loginWindow));
    }
}
