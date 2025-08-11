#include "uiMainFrame.h"

#include "constants.h"
#include "uiLogin.h"
#include "uiLoginPhone.h"
#include "uiMainWindow.h"

CMainFrame* g_mainFrame{nullptr};

CMainFrame::CMainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {
    auto* panel = new wxPanel(this, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_book = new wxSimplebook(panel, wxID_ANY);

    m_loginWindow = new CLoginWindow(m_book);
    m_loginPhoneWindow = new CLoginPhoneWindow(m_book, m_tdManager);

    m_book->AddPage(m_loginWindow, "Login");
    m_book->AddPage(m_loginPhoneWindow, "LoginPhone");

    sizer->Add(m_book, 1, wxEXPAND);
    panel->SetSizer(sizer);

    InitializeTdlib();
}

void CMainFrame::InitializeTdlib() {
    m_tdManager.setUpdateCallback([this](td::td_api::object_ptr<td::td_api::Object> update) {
        if (!update) {
            return;
        }

        if (update->get_id() == td::td_api::updateAuthorizationState::ID) {
            td::td_api::Object* raw_update_ptr = update.release();
            CallAfter([this, raw_update_ptr]() {
                td::td_api::object_ptr<td::td_api::Object> update_ptr(raw_update_ptr);
                OnAuthorizationStateUpdate(std::move(update_ptr));
            });
        } else if (m_mainWindow) {
            td::td_api::Object* raw_update_ptr = update.release();
            CallAfter([this, raw_update_ptr]() {
                td::td_api::object_ptr<td::td_api::Object> update_ptr(raw_update_ptr);
                m_mainWindow->ProcessUpdate(std::move(update_ptr));
            });
        }
    });

    auto set_params = td::td_api::make_object<td::td_api::setTdlibParameters>();
    set_params->api_id_ = API_ID;
    set_params->api_hash_ = API_HASH;
    set_params->database_directory_ = "tdlib";
    set_params->use_test_dc_ = false;
    set_params->device_model_ = "Desktop";
    set_params->system_language_code_ = "en";
    set_params->application_version_ = "0.1";
    set_params->use_message_database_ = true;

    m_tdManager.send(std::move(set_params), nullptr);
}

#include <wx/msgdlg.h>
void CMainFrame::OnAuthorizationStateUpdate(td::td_api::object_ptr<td::td_api::Object> update) {
    auto auth_state = td::td_api::move_object_as<td::td_api::updateAuthorizationState>(update);
    auto state_id = auth_state->authorization_state_->get_id();

    if (state_id == td::td_api::authorizationStateReady::ID) {
        m_mainWindow = new CMainWindow(m_book);
        m_book->AddPage(m_mainWindow, "Main");
        m_book->SetSelection(m_book->FindPage(m_mainWindow));
        return;
    } else if (state_id == td::td_api::authorizationStateWaitPhoneNumber::ID) {
        m_book->SetSelection(m_book->FindPage(m_loginWindow));
        return;
    }

    if (!m_loginPhoneWindow)
        return;

    m_book->SetSelection(m_book->FindPage(m_loginPhoneWindow));
    if (state_id == td::td_api::authorizationStateWaitCode::ID) {
        m_loginPhoneWindow->SwitchLoginState(CLoginPhoneWindow::LOGIN_CODE);
    } else if (state_id == td::td_api::authorizationStateWaitPassword::ID) {
        m_loginPhoneWindow->SwitchLoginState(CLoginPhoneWindow::LOGIN_PASSWORD);
    } else if (state_id == td::td_api::authorizationStateClosed::ID) {
        wxMessageBox("Authentication failed or was terminated. Please try again.", "Login Error", wxOK | wxICON_ERROR);
        wxCommandEvent evt;
        m_loginPhoneWindow->OnCancelPressed(evt);
    }
}

bool CMgramEntry::OnInit() {
    g_mainFrame = new CMainFrame("MGram");
    g_mainFrame->Show(true);
    return true;
}
