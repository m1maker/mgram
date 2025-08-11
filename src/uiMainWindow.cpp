#include "uiMainWindow.h"

#include "uiMainFrame.h"

#include <wx/datetime.h>
#include <wx/listbox.h>
#include <wx/wx.h>

class CChatClientData : public wxClientData {
  public:
    CChatClientData(long long chatId, long long sortKey) : m_chatId(chatId), m_sortKey(sortKey) {}

    long long GetChatId() const { return m_chatId; }
    long long GetSortKey() const { return m_sortKey; }

  private:
    long long m_chatId;
    long long m_sortKey;
};

CMainWindow::CMainWindow(wxSimplebook* book) : wxPanel(book, wxID_ANY), m_book(book), m_currentChatId(0) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* leftPanel = new wxPanel(m_splitter);
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);

    m_chatList = new wxListBox(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr);
    leftSizer->Add(m_chatList, 1, wxEXPAND | wxALL);
    leftPanel->SetSizer(leftSizer);

    m_chatList->Bind(wxEVT_LISTBOX, &CMainWindow::OnChatSelected, this);

    auto* rightPanel = new wxPanel(m_splitter);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    m_messageView = new wxTextCtrl(rightPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    rightSizer->Add(m_messageView, 1, wxEXPAND | wxALL);

    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_messageInput = new wxTextCtrl(rightPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_sendButton = new wxButton(rightPanel, wxID_ANY, "Send");
    bottomSizer->Add(m_messageInput, 1, wxEXPAND | wxALL);
    bottomSizer->Add(m_sendButton, 0, wxEXPAND | wxALL);
    rightSizer->Add(bottomSizer, 0, wxEXPAND);
    rightPanel->SetSizer(rightSizer);

    m_splitter->SplitVertically(leftPanel, rightPanel, 350);
    sizer->Add(m_splitter, 1, wxEXPAND);
    SetSizerAndFit(sizer);

    m_sendButton->Bind(wxEVT_BUTTON, &CMainWindow::OnSendPressed, this);
    m_messageInput->Bind(wxEVT_TEXT_ENTER, &CMainWindow::OnSendPressed, this);

    LoadChats();
}

void CMainWindow::LoadChats() {
    g_mainFrame->getTdManager()->send(
        td::td_api::make_object<td::td_api::loadChats>(td::td_api::make_object<td::td_api::chatListMain>(), 100), {});
}

void CMainWindow::ProcessChatUpdate(td::td_api::object_ptr<td::td_api::chat> chat) {
    if (!chat) {
        return;
    }

    long long chatId = chat->id_;
    s_chats[chatId] = std::move(chat);
    UpdateChatInList(chatId);
}

void CMainWindow::UpdateChatInList(long long chatId) {
    auto it = s_chats.find(chatId);
    if (it == s_chats.end()) {
        return;
    }

    const auto& chat = it->second;

    wxString type_str;
    switch (chat->type_->get_id()) {
        case td::td_api::chatTypeBasicGroup::ID:
        case td::td_api::chatTypeSupergroup::ID:
            type_str = "Group, ";
            break;
        case td::td_api::chatTypeSecret::ID:
            type_str = "Secret, ";
            break;
    }

    wxString unread_str = chat->unread_count_ > 0 ? wxString::Format("[%d unread], ", chat->unread_count_) : "";

    wxString time_str;
    if (chat->last_message_) {
        wxDateTime dt(static_cast<time_t>(chat->last_message_->date_));
        time_str = dt.Format("%H:%M");
    }

    std::string lastMessageText = "No messages";
    if (chat->last_message_) {
        if (chat->last_message_->content_->get_id() == td::td_api::messageText::ID) {
            auto* textContent = static_cast<const td::td_api::messageText*>(chat->last_message_->content_.get());
            lastMessageText = textContent->text_->text_;
        } else {
            lastMessageText = "[Media]";
        }
    }

    wxString display_str =
        wxString::Format("%s%s%s: %s", type_str, unread_str, time_str, wxString::FromUTF8(lastMessageText));
    wxString name_str = wxString::FromUTF8(chat->title_);
    wxString final_display_str = wxString::Format("%s\n%s", name_str, display_str);

    bool is_pinned = false;
    long long order = 0;
    if (!chat->positions_.empty()) {
        for (const auto& pos : chat->positions_) {
            if (pos->list_->get_id() == td::td_api::chatListMain::ID) {
                is_pinned = pos->is_pinned_;
                order = pos->order_;
                break;
            }
        }
    }

    CallAfter([this, chatId, final_display_str, is_pinned, order]() {
        m_chatList->Freeze();

        long long sortKey = (static_cast<long long>(is_pinned) << 62) | order;

        for (unsigned int i = 0; i < m_chatList->GetCount(); ++i) {
            auto* clientData = static_cast<CChatClientData*>(m_chatList->GetClientObject(i));
            if (clientData && clientData->GetChatId() == chatId) {
                m_chatList->Delete(i);
                break;
            }
        }

        unsigned int insertPos = 0;
        for (; insertPos < m_chatList->GetCount(); ++insertPos) {
            auto* existingClientData = static_cast<CChatClientData*>(m_chatList->GetClientObject(insertPos));
            if (existingClientData && sortKey > existingClientData->GetSortKey()) {
                break;
            }
        }

        m_chatList->Insert(final_display_str, insertPos);
        m_chatList->SetClientObject(insertPos, new CChatClientData(chatId, sortKey));

        m_chatList->Thaw();
    });
}

void CMainWindow::ProcessUpdate(td::td_api::object_ptr<td::td_api::Object> update) {
    switch (update->get_id()) {
        case td::td_api::updateNewChat::ID: {
            auto new_chat_update = td::td_api::move_object_as<td::td_api::updateNewChat>(update);
            ProcessChatUpdate(std::move(new_chat_update->chat_));
            break;
        }
        case td::td_api::updateNewMessage::ID: {
            auto msg_update = td::td_api::move_object_as<td::td_api::updateNewMessage>(update);
            UpdateChatInList(msg_update->message_->chat_id_);
            if (msg_update->message_->chat_id_ == m_currentChatId) {
                LoadMessages(m_currentChatId);
            }
            break;
        }
        case td::td_api::updateChatLastMessage::ID: {
            auto last_msg_update = td::td_api::move_object_as<td::td_api::updateChatLastMessage>(update);
            UpdateChatInList(last_msg_update->chat_id_);
            break;
        }
        case td::td_api::updateChatTitle::ID: {
            auto title_update = td::td_api::move_object_as<td::td_api::updateChatTitle>(update);
            UpdateChatInList(title_update->chat_id_);
            break;
        }
        case td::td_api::updateUser::ID: {
            auto user_update = td::td_api::move_object_as<td::td_api::updateUser>(update);
            s_users[user_update->user_->id_] = std::move(user_update->user_);
            break;
        }
        default:
            break;
    }
}

void CMainWindow::OnChatSelected(wxCommandEvent& event) {
    int selectedIndex = m_chatList->GetSelection();
    if (selectedIndex != wxNOT_FOUND) {
        auto* clientData = static_cast<CChatClientData*>(m_chatList->GetClientObject(selectedIndex));

        if (clientData) {
            long long chatId = clientData->GetChatId();

            if (chatId != 0 && chatId != m_currentChatId) {
                m_currentChatId = chatId;
                m_messageView->Clear();
                LoadMessages(m_currentChatId);
            }
        }
    }
}

void CMainWindow::GetUser(long long userId, std::function<void(const td::td_api::user*)> callback) {
    auto it = s_users.find(userId);
    if (it != s_users.end()) {
        callback(it->second.get());
    } else {
        g_mainFrame->getTdManager()->send(td::td_api::make_object<td::td_api::getUser>(userId),
                                          [this, callback](TdManager::Object userObject) {
                                              if (userObject->get_id() == td::td_api::user::ID) {
                                                  auto user = td::td_api::move_object_as<td::td_api::user>(userObject);
                                                  s_users[user->id_] = std::move(user);
                                                  callback(s_users[user->id_].get());
                                              } else {
                                                  callback(nullptr);
                                              }
                                          });
    }
}

void CMainWindow::LoadMessages(long long chatId) {
    auto getHistory = td::td_api::make_object<td::td_api::getChatHistory>(chatId, 0, 0, 50, false);

    g_mainFrame->getTdManager()->send(std::move(getHistory), [this, chatId](TdManager::Object object) {
        if (object->get_id() == td::td_api::messages::ID) {
            auto messages = td::td_api::move_object_as<td::td_api::messages>(object);

            wxString full_history;
            for (auto it = messages->messages_.rbegin(); it != messages->messages_.rend(); ++it) {
                auto& message = *it;

                wxString sender_name = "Unknown";
                if (message->sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
                    long long sender_id =
                        static_cast<td::td_api::messageSenderUser*>(message->sender_id_.get())->user_id_;
                    GetUser(sender_id, [&sender_name](const td::td_api::user* user) {
                        if (user) {
                            sender_name = wxString::FromUTF8(user->first_name_);
                        }
                    });
                } else if (message->sender_id_->get_id() == td::td_api::messageSenderChat::ID) {
                    long long sender_chat_id =
                        static_cast<td::td_api::messageSenderChat*>(message->sender_id_.get())->chat_id_;
                    auto chat_it = s_chats.find(sender_chat_id);
                    if (chat_it != s_chats.end()) {
                        sender_name = wxString::FromUTF8(chat_it->second->title_);
                    }
                }

                if (message->content_->get_id() == td::td_api::messageText::ID) {
                    auto* textContent = static_cast<const td::td_api::messageText*>(message->content_.get());
                    full_history +=
                        wxString::Format("%s: %s\n", sender_name, wxString::FromUTF8(textContent->text_->text_));
                } else {
                    full_history += wxString::Format("%s: [Unsupported Message Type]\n", sender_name);
                }
            }

            CallAfter([this, full_history]() { m_messageView->SetValue(full_history); });
        }
    });
}

void CMainWindow::OnSendPressed(wxCommandEvent& event) {
    wxString messageText = m_messageInput->GetValue();
    if (messageText.IsEmpty() || m_currentChatId == 0) {
        return;
    }

    auto content = td::td_api::make_object<td::td_api::inputMessageText>();
    content->text_ = td::td_api::make_object<td::td_api::formattedText>();
    content->text_->text_ = messageText.ToStdString(wxConvUTF8);

    auto sendMessage = td::td_api::make_object<td::td_api::sendMessage>();
    sendMessage->chat_id_ = m_currentChatId;
    sendMessage->input_message_content_ = std::move(content);

    g_mainFrame->getTdManager()->send(std::move(sendMessage), [this](TdManager::Object object) {
        if (object->get_id() == td::td_api::error::ID) {
            auto error = td::td_api::move_object_as<td::td_api::error>(object);
            wxString error_msg = wxString::Format("Failed to send message: %s", error->message_);
            CallAfter([error_msg]() { wxMessageBox(error_msg, "Error", wxOK | wxICON_ERROR); });
        }
    });

    m_messageInput->Clear();
    m_messageInput->SetFocus();
}
