#include "uiMainFrame.h"

#include "constants.h"
#include "uiLogin.h"
#include "uiLoginPhone.h"
#include "uiMainWindow.h"
#include "uiTaskBarIcon.h"

#include <wx/artprov.h>
#include <wx/notifmsg.h>

CMainFrame* g_mainFrame{nullptr};

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
EVT_CLOSE(CMainFrame::OnClose)
END_EVENT_TABLE()

CMainFrame::CMainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {
    auto* taskBarIcon = new CMgramTaskBarIcon(this);
    taskBarIcon->SetIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_OTHER, wxSize(16, 16)), "MGram");
#ifdef _WIN32
    wxNotificationMessage::UseTaskBarIcon(taskBarIcon);
#endif

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
        m_loginWindow->Hide();
        m_loginPhoneWindow->Hide();
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

void CMainFrame::OnClose(wxCloseEvent& event) {
    if (event.CanVeto()) {
        event.Veto();
        Hide();
    } else {
        Destroy();
    }
}

bool CMgramEntry::OnInit() {
    wxLog::SetActiveTarget(new wxLogStderr);
    const wxString name = wxString::Format("MMADE-MGRAM-%s", wxGetUserId().c_str());
    m_instanceChecker = new wxSingleInstanceChecker(name);

    if (m_instanceChecker->IsAnotherRunning()) {
        wxClient* client = new wxClient();
        wxConnectionBase* connection = client->MakeConnection("localhost", IPC_SERVICE_PORT, "Focus");

        if (connection) {
            connection->Execute("Focus");
            delete connection;
        }
        delete client;
        return false;
    }

    m_server = new CMgramServer();
    m_server->Create(IPC_SERVICE_PORT);

    g_mainFrame = new CMainFrame("MGram");
    g_mainFrame->Show(true);
    return true;
}

int CMgramEntry::OnExit() {
    delete m_instanceChecker;
    delete m_server;
    return 0;
}

wxConnectionBase* CMgramServer::OnAcceptConnection(const wxString& topic) {
    if (topic.Lower() == "focus") {
        return new CMgramConnection();
    }
    return nullptr;
}

bool CMgramConnection::OnExec(const wxString& topic, const wxString& data) {

    if (topic.Lower() == "focus" || data.Lower() == "focus") {
        if (g_mainFrame) {
            g_mainFrame->CallAfter([]() {
                if (!g_mainFrame->IsShown()) {
                    g_mainFrame->Show(true);
                }

                if (g_mainFrame->IsIconized()) {
                    g_mainFrame->Iconize(false);
                }

                g_mainFrame->Raise();
                g_mainFrame->RequestUserAttention();
            });
        }
        return true;
    }
    return false;
}
