#include "uiMainWindow.h"

#include "uiMainFrame.h"

#include <utility>
#include <wx/datetime.h>
#include <wx/listbox.h>
#include <wx/wx.h>

class CChatClientData final : public wxClientData {
  public:
    explicit CChatClientData(long long chatId, long long sortKey) : m_chatId(chatId), m_sortKey(sortKey) {}
    long long GetChatId() const { return m_chatId; }
    long long GetSortKey() const { return m_sortKey; }

  private:
    long long m_chatId;
    long long m_sortKey;
};

CMainWindow::CMainWindow(wxSimplebook* book)
    : wxPanel(book, wxID_ANY), m_book(book), m_currentChatId(0), m_lastMessageId(0), m_loadingMore(false) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* leftPanel = new wxPanel(m_splitter);
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    auto* chatListLabel = new wxStaticText(leftPanel, wxID_ANY, "&Chats");
    m_chatList = new wxListBox(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr);
    leftSizer->Add(chatListLabel, 0, wxALL, 5);
    leftSizer->Add(m_chatList, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    m_chatList->Bind(wxEVT_LISTBOX, &CMainWindow::OnChatSelected, this);

    auto* rightPanel = new wxPanel(m_splitter);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    auto* messagesLabel = new wxStaticText(rightPanel, wxID_ANY, "&Messages");
    m_messageView = new wxListBox(rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr);
    rightSizer->Add(messagesLabel, 0, wxALL, 5);
    rightSizer->Add(m_messageView, 1, wxEXPAND | wxALL, 5);

    m_messageView->Bind(wxEVT_LISTBOX, &CMainWindow::OnMessageSelected, this);

    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_messageInputLabel = new wxStaticText(rightPanel, wxID_ANY, "Message:", wxDefaultPosition, wxDefaultSize);
    m_messageInput = new wxTextCtrl(rightPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_sendButton = new wxButton(rightPanel, wxID_ANY, "Send");
    bottomSizer->Add(m_messageInputLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    bottomSizer->Add(m_messageInput, 1, wxEXPAND | wxALL, 5);
    bottomSizer->Add(m_sendButton, 0, wxEXPAND | wxALL, 5);
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
    m_chats[chatId] = std::move(chat);
    UpdateChatInList(chatId);
}

void CMainWindow::UpdateChatInList(long long chatId) {
    auto it = m_chats.find(chatId);
    if (it == m_chats.end()) {
        return;
    }

    auto& chat = it->second;

    if (chat->type_->get_id() == td::td_api::chatTypePrivate::ID) {
        auto* privateChat = static_cast<td::td_api::chatTypePrivate*>(chat->type_.get());

        GetUser(privateChat->user_id_, [this, chatId](const td::td_api::user* user) {
            if (!user) {
                return;
            }

            auto it_lambda = m_chats.find(chatId);
            if (it_lambda == m_chats.end()) {
                return;
            }
            auto& chat_lambda = it_lambda->second;

            chat_lambda->title_ = user->first_name_ + " " + user->last_name_;

            wxString unread_str =
                chat_lambda->unread_count_ > 0 ? wxString::Format("[%d] ", chat_lambda->unread_count_) : "";
            wxString lastMessageText = "No messages";
            if (chat_lambda->last_message_) {
                if (chat_lambda->last_message_->content_->get_id() == td::td_api::messageText::ID) {
                    auto* textContent =
                        static_cast<const td::td_api::messageText*>(chat_lambda->last_message_->content_.get());
                    lastMessageText = wxString::FromUTF8(textContent->text_->text_);
                } else {
                    lastMessageText = "[Media]";
                }
            }

            wxString final_display_str =
                wxString::Format("%s%s: %s", unread_str, wxString::FromUTF8(chat_lambda->title_), lastMessageText);
            long long order = 0;
            bool is_pinned = false;

            if (!chat_lambda->positions_.empty()) {
                for (const auto& pos : chat_lambda->positions_) {
                    if (pos != nullptr && pos->list_->get_id() == td::td_api::chatListMain::ID) {
                        order = pos->order_;
                        is_pinned = pos->is_pinned_;
                        break;
                    }
                }
            }

            long long sortKey = (static_cast<long long>(is_pinned) << 62) | order;

            CallAfter([this, chatId, final_display_str, sortKey]() {
                m_chatList->Freeze();
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
        });

        return;
    }

    wxString unread_str = chat->unread_count_ > 0 ? wxString::Format("[%d] ", chat->unread_count_) : "";
    wxString lastMessageText = "No messages";
    if (chat->last_message_) {
        if (chat->last_message_->content_->get_id() == td::td_api::messageText::ID) {
            auto* textContent = static_cast<const td::td_api::messageText*>(chat->last_message_->content_.get());
            lastMessageText = wxString::FromUTF8(textContent->text_->text_);
        } else {
            lastMessageText = "[Media]";
        }
    }

    wxString final_display_str =
        wxString::Format("%s%s: %s", unread_str, wxString::FromUTF8(chat->title_), lastMessageText);
    long long order = 0;
    bool is_pinned = false;

    if (!chat->positions_.empty()) {
        for (const auto& pos : chat->positions_) {
            if (pos != nullptr && pos->list_->get_id() == td::td_api::chatListMain::ID) {
                order = pos->order_;
                is_pinned = pos->is_pinned_;
                break;
            }
        }
    }

    long long sortKey = (static_cast<long long>(is_pinned) << 62) | order;
    CallAfter([this, chatId, final_display_str, sortKey]() {
        m_chatList->Freeze();
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
            ProcessChatUpdate(std::move(td::td_api::move_object_as<td::td_api::updateNewChat>(update)->chat_));
            break;
        }
        case td::td_api::updateChatTitle::ID: {
            auto title_update = td::td_api::move_object_as<td::td_api::updateChatTitle>(update);
            auto it = m_chats.find(title_update->chat_id_);
            if (it != m_chats.end()) {
                it->second->title_ = title_update->title_;
                UpdateChatInList(title_update->chat_id_);
            }
            break;
        }
        case td::td_api::updateChatLastMessage::ID: {
            auto last_msg_update = td::td_api::move_object_as<td::td_api::updateChatLastMessage>(update);
            auto it = m_chats.find(last_msg_update->chat_id_);
            if (it != m_chats.end()) {
                it->second->last_message_ = std::move(last_msg_update->last_message_);
                it->second->positions_ = std::move(last_msg_update->positions_);
                UpdateChatInList(last_msg_update->chat_id_);
            }
            break;
        }
        case td::td_api::updateNewMessage::ID: {
            auto msg_update = td::td_api::move_object_as<td::td_api::updateNewMessage>(update);
            if (msg_update->message_->chat_id_ == m_currentChatId) {
                AppendMessage(msg_update->message_);
            }
            auto it = m_chats.find(msg_update->message_->chat_id_);
            if (it != m_chats.end()) {
                it->second->last_message_ = std::move(msg_update->message_);
                UpdateChatInList(it->first);
            }
            break;
        }
        case td::td_api::updateChatPosition::ID: {
            auto pos_update = td::td_api::move_object_as<td::td_api::updateChatPosition>(update);
            auto it = m_chats.find(pos_update->chat_id_);
            if (it != m_chats.end()) {
                bool found = false;
                for (auto& pos : it->second->positions_) {
                    if (pos->list_->get_id() == pos_update->position_->list_->get_id()) {
                        pos = std::move(pos_update->position_);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    it->second->positions_.push_back(std::move(pos_update->position_));
                }
                UpdateChatInList(pos_update->chat_id_);
            }
            break;
        }
        case td::td_api::updateMessageContent::ID: {
            auto content_update = td::td_api::move_object_as<td::td_api::updateMessageContent>(update);
            if (content_update->chat_id_ == m_currentChatId) {
                m_lastMessageId = 0;
                LoadMessages(m_currentChatId);
            }
            auto it = m_chats.find(content_update->chat_id_);
            if (it != m_chats.end() && it->second->last_message_ &&
                it->second->last_message_->id_ == content_update->message_id_) {
                UpdateChatInList(content_update->chat_id_);
            }
            break;
        }
        case td::td_api::updateChatReadInbox::ID: {
            auto read_update = td::td_api::move_object_as<td::td_api::updateChatReadInbox>(update);
            auto it = m_chats.find(read_update->chat_id_);
            if (it != m_chats.end()) {
                it->second->unread_count_ = read_update->unread_count_;
                UpdateChatInList(read_update->chat_id_);
            }
            break;
        }
        case td::td_api::updateUser::ID: {
            auto user_update = td::td_api::move_object_as<td::td_api::updateUser>(update);
            m_users[user_update->user_->id_] = std::move(user_update->user_);
            break;
        }
        default:
            break;
    }
}

void CMainWindow::OnChatSelected(wxCommandEvent& event) {
    int selectedIndex = m_chatList->GetSelection();
    if (selectedIndex == wxNOT_FOUND)
        return;
    auto* clientData = static_cast<CChatClientData*>(m_chatList->GetClientObject(selectedIndex));
    if (!clientData)
        return;

    long long chatId = clientData->GetChatId();
    if (chatId != 0 && chatId != m_currentChatId) {
        m_currentChatId = chatId;
        m_messageView->Clear();
        m_lastMessageId = 0;
        LoadMessages(m_currentChatId);
    }
}

void CMainWindow::OnMessageSelected(wxCommandEvent& event) {
    LoadMessages(m_currentChatId);
    event.Skip();
}

void CMainWindow::GetUser(long long userId, std::function<void(const td::td_api::user*)> callback) {
    auto it = m_users.find(userId);
    if (it != m_users.end()) {
        callback(it->second.get());
    } else {
        g_mainFrame->getTdManager()->send(td::td_api::make_object<td::td_api::getUser>(userId),
                                          [this, userId, callback](TdManager::Object userObject) {
                                              if (userObject->get_id() == td::td_api::user::ID) {
                                                  auto user = td::td_api::move_object_as<td::td_api::user>(userObject);
                                                  m_users[userId] = std::move(user);
                                                  callback(m_users[userId].get());
                                              } else {
                                                  callback(nullptr);
                                              }
                                          });
    }
}

void CMainWindow::LoadMessages(long long chatId) {
    if (m_loadingMore)
        return;
    m_loadingMore = true;

    auto getHistory = td::td_api::make_object<td::td_api::getChatHistory>(chatId, m_lastMessageId, 0, 50, false);
    g_mainFrame->getTdManager()->send(std::move(getHistory), [this, chatId](TdManager::Object object) {
        if (object->get_id() == td::td_api::messages::ID) {
            auto messages = td::td_api::move_object_as<td::td_api::messages>(object);
            if (!messages->messages_.empty()) {
                m_lastMessageId = messages->messages_.back()->id_;
            }

            std::vector<wxString> history;

            for (auto it = messages->messages_.rbegin(); it != messages->messages_.rend(); ++it) {
                const auto& message = *it;

                wxString sender_name = "Unknown";
                if (message->sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
                    auto* sender = static_cast<const td::td_api::messageSenderUser*>(message->sender_id_.get());
                    auto user_it = m_users.find(sender->user_id_);
                    if (user_it != m_users.end()) {
                        sender_name = wxString::FromUTF8(user_it->second->first_name_);
                    }
                }

                wxString text = "[Unsupported Message Type]";
                if (message->content_->get_id() == td::td_api::messageText::ID) {
                    auto* textContent = static_cast<const td::td_api::messageText*>(message->content_.get());
                    text = wxString::FromUTF8(textContent->text_->text_);
                }

                history.push_back(wxString::Format("%s: %s", sender_name, text));
            }

            CallAfter([this, history = std::move(history)]() {
                if (history.empty()) {
                    return;
                }

                m_messageView->Freeze();

                int insert_pos = 0;
                for (const auto& msg_str : history) {
                    m_messageView->Insert(msg_str, insert_pos++);
                }

                m_messageView->Thaw();
            });
        }
        m_loadingMore = false;
    });
}

void CMainWindow::AppendMessage(const td::td_api::object_ptr<td::td_api::message>& message) {
    long long sender_user_id = 0;
    if (message->sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
        auto* sender = static_cast<const td::td_api::messageSenderUser*>(message->sender_id_.get());
        sender_user_id = sender->user_id_;
    } else {
        return;
    }

    wxString text = "[Unsupported Message Type]";
    if (message->content_->get_id() == td::td_api::messageText::ID) {
        auto* textContent = static_cast<const td::td_api::messageText*>(message->content_.get());
        text = wxString::FromUTF8(textContent->text_->text_);
    }

    GetUser(sender_user_id, [this, text](const td::td_api::user* user) {
        if (user) {
            wxString name = wxString::FromUTF8(user->first_name_);
            CallAfter([this, name, text]() {
                m_messageView->Append(wxString::Format("%s: %s", name, text));
                m_messageView->SetSelection(m_messageView->GetCount() - 1);
            });
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
            wxString error_msg = wxString::Format("Failed to send message: %s", wxString::FromUTF8(error->message_));
            CallAfter([error_msg]() { wxMessageBox(error_msg, "Error", wxOK | wxICON_ERROR); });
        }
    });

    m_messageInput->Clear();
    m_messageInput->SetFocus();
}
