#include "uiMainWindow.h"

#include "clientData.h"
#include "notificationSender.h"
#include "uiMainFrame.h"

#include <atomic>
#include <utility>
#include <wx/datetime.h>
#include <wx/listbox.h>
#include <wx/wx.h>

static wxString FormatTimestamp(int64_t unix_time) {
    if (unix_time == 0) {
        return "N/A";
    }
    wxDateTime dt(static_cast<time_t>(unix_time));
    return dt.Format("%Y-%m-%d %H:%M");
}

static wxString FormatMessageContentPreview(const td::td_api::MessageContent* content) {
    if (!content) {
        return "No messages";
    }
    switch (content->get_id()) {
        case td::td_api::messageText::ID: {
            auto* textContent = static_cast<const td::td_api::messageText*>(content);
            return wxString::FromUTF8(textContent->text_->text_);
        }
        case td::td_api::messageAnimation::ID:
            return "[Animation]";
        case td::td_api::messageAudio::ID:
            return "[Audio]";
        case td::td_api::messageDocument::ID:
            return "[File]";
        case td::td_api::messagePhoto::ID:
            return "[Photo]";
        case td::td_api::messageSticker::ID:
            return "[Sticker]";
        case td::td_api::messageVideo::ID:
            return "[Video]";
        case td::td_api::messageVoiceNote::ID:
            return "[Voice message]";
        case td::td_api::messageCall::ID:
            return "[Call]";
        case td::td_api::messageContact::ID:
            return "[Contact]";
        case td::td_api::messageLocation::ID:
            return "[Location]";
        case td::td_api::messagePoll::ID:
            return "[Poll]";
        case td::td_api::messageVideoNote::ID:
            return "[Video message]";
        case td::td_api::messageChatAddMembers::ID:
            return "[Service: New members]";
        case td::td_api::messageChatChangeTitle::ID: {
            auto* titleChange = static_cast<const td::td_api::messageChatChangeTitle*>(content);
            return "Title changed to " + wxString::FromUTF8(titleChange->title_);
        }
        case td::td_api::messagePinMessage::ID:
            return "[Service: Pinned a message]";
        default:
            return "[Unsupported message]";
    }
}

static wxString FormatMessageContent(const td::td_api::MessageContent* content) {
    if (!content) {
        return "[Empty message]";
    }
    wxString formatted_content;
    wxString type_str;

    switch (content->get_id()) {
        case td::td_api::messageText::ID: {
            auto* textContent = static_cast<const td::td_api::messageText*>(content);
            return wxString::FromUTF8(textContent->text_->text_);
        }
        case td::td_api::messageVoiceNote::ID: {
            auto* voice = static_cast<const td::td_api::messageVoiceNote*>(content);
            type_str = "Voice";
            if (voice->voice_note_) {
                formatted_content = wxString::Format("%d seconds", voice->voice_note_->duration_);
            }
            break;
        }
        case td::td_api::messageDocument::ID: {
            auto* doc = static_cast<const td::td_api::messageDocument*>(content);
            type_str = "File";
            formatted_content = wxString::FromUTF8(doc->document_->file_name_);
            break;
        }
        case td::td_api::messagePhoto::ID: {
            type_str = "Photo";
            auto* photo = static_cast<const td::td_api::messagePhoto*>(content);
            if (!photo->caption_->text_.empty()) {
                formatted_content = wxString::FromUTF8(photo->caption_->text_);
            }
            break;
        }
        case td::td_api::messageVideo::ID: {
            type_str = "Video";
            auto* video = static_cast<const td::td_api::messageVideo*>(content);
            if (!video->caption_->text_.empty()) {
                formatted_content = wxString::FromUTF8(video->caption_->text_);
            }
            break;
        }
        case td::td_api::messageChatAddMembers::ID:
            type_str = "Service";
            formatted_content = "Members added";
            break;
        case td::td_api::messageChatChangeTitle::ID: {
            type_str = "Service";
            auto* title_change = static_cast<const td::td_api::messageChatChangeTitle*>(content);
            formatted_content = "Title changed to " + wxString::FromUTF8(title_change->title_);
            break;
        }
        default:
            type_str = "Other";
            formatted_content = "Unsupported content";
            break;
    }

    return type_str + (formatted_content.IsEmpty() ? "" : ", " + formatted_content);
}

static bool IsPositionInCurrentList(const td::td_api::ChatList* position_list,
                                    const td::td_api::ChatList* current_list) {
    if (!position_list || !current_list)
        return false;
    if (position_list->get_id() != current_list->get_id())
        return false;

    if (current_list->get_id() == td::td_api::chatListFolder::ID) {
        auto* pos_folder = static_cast<const td::td_api::chatListFolder*>(position_list);
        auto* current_folder = static_cast<const td::td_api::chatListFolder*>(current_list);
        return pos_folder->chat_folder_id_ == current_folder->chat_folder_id_;
    }
    return true;
}

CMainWindow::CMainWindow(wxSimplebook* book)
    : wxPanel(book, wxID_ANY), m_book(book), m_currentChatId(0), m_lastMessageId(0), m_loadingMore(false) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* leftPanel = new wxPanel(m_splitter);
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);

    auto* folderListLabel = new wxStaticText(leftPanel, wxID_ANY, "&Folders");
    m_folderList = new wxListBox(leftPanel, wxID_ANY);
    leftSizer->Add(folderListLabel, 0, wxALL, 5);
    leftSizer->Add(m_folderList, 0, wxEXPAND | wxALL, 5);

    auto* chatListLabel = new wxStaticText(leftPanel, wxID_ANY, "&Chats");
    m_chatList = new wxListBox(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr);
    leftSizer->Add(chatListLabel, 0, wxALL, 5);
    leftSizer->Add(m_chatList, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    m_folderList->Bind(wxEVT_LISTBOX, &CMainWindow::OnFolderSelected, this);
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

    m_currentChatList = td::td_api::make_object<td::td_api::chatListMain>();
    m_chatList->SetFocus();
}

void CMainWindow::LoadChats() {
    if (!m_currentChatList)
        return;

    td::td_api::object_ptr<td::td_api::ChatList> chat_list_to_load;
    switch (m_currentChatList->get_id()) {
        case td::td_api::chatListMain::ID:
            chat_list_to_load = td::td_api::make_object<td::td_api::chatListMain>();
            break;
        case td::td_api::chatListArchive::ID:
            chat_list_to_load = td::td_api::make_object<td::td_api::chatListArchive>();
            break;
        case td::td_api::chatListFolder::ID: {
            auto* folder_list = static_cast<td::td_api::chatListFolder*>(m_currentChatList.get());
            chat_list_to_load = td::td_api::make_object<td::td_api::chatListFolder>(folder_list->chat_folder_id_);
            break;
        }
    }

    if (chat_list_to_load) {
        g_mainFrame->getTdManager()->send(
            td::td_api::make_object<td::td_api::loadChats>(std::move(chat_list_to_load), 100), {});
    }
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
            if (!user)
                return;
            auto chat_it = m_chats.find(chatId);
            if (chat_it != m_chats.end()) {
                FormatAndUpdateChatListEntry(chat_it->second, user);
            }
        });
        return;
    }
    FormatAndUpdateChatListEntry(chat, nullptr);
}

void CMainWindow::FormatAndUpdateChatListEntry(const td::td_api::object_ptr<td::td_api::chat>& chat,
                                               const td::td_api::user* user) {
    long long chatId = chat->id_;
    wxString display_str;

    wxString type_prefix;
    switch (chat->type_->get_id()) {
        case td::td_api::chatTypePrivate::ID:
            if (user && user->type_->get_id() == td::td_api::userTypeBot::ID) {
                type_prefix = "Bot. ";
            }
            break;
        case td::td_api::chatTypeBasicGroup::ID:
            type_prefix = "Group. ";
            break;
        case td::td_api::chatTypeSecret::ID:
            type_prefix = "Secret. ";
            break;
        case td::td_api::chatTypeSupergroup::ID: {
            auto* sg = static_cast<const td::td_api::chatTypeSupergroup*>(chat->type_.get());
            type_prefix = sg->is_channel_ ? "Channel. " : "Supergroup. ";
            break;
        }
    }
    display_str += type_prefix;

    if (user) {
        display_str += wxString::FromUTF8(user->first_name_ + " " + user->last_name_);
    } else {
        display_str += wxString::FromUTF8(chat->title_);
    }

    if (user && user->is_premium_)
        display_str += ", Premium account";

    if (user && user->status_) {
        wxString status_str;
        switch (user->status_->get_id()) {
            case td::td_api::userStatusOnline::ID:
                status_str = "online";
                break;
            case td::td_api::userStatusOffline::ID: {
                auto* offline = static_cast<const td::td_api::userStatusOffline*>(user->status_.get());
                status_str = "last seen at " + FormatTimestamp(offline->was_online_);
                break;
            }
            case td::td_api::userStatusRecently::ID:
                status_str = "last seen recently";
                break;
            default:
                break;
        }
        if (!status_str.IsEmpty()) {
            display_str += ", " + status_str;
        }
    }

    if (chat->unread_count_ > 0) {
        display_str += wxString::Format(", %d unread messages", chat->unread_count_);
    }

    if (chat->last_message_) {
        wxString timestamp = FormatTimestamp(chat->last_message_->date_);
        wxString content_preview = FormatMessageContentPreview(chat->last_message_->content_.get());
        display_str += wxString::Format(", received at %s: %s", timestamp, content_preview);
    }

    bool is_pinned = false;
    long long order = 0x7FFFFFFFFFFFFFFF;
    bool in_current_list = false;

    if (!chat->positions_.empty()) {
        for (const auto& pos : chat->positions_) {
            if (pos && IsPositionInCurrentList(pos->list_.get(), m_currentChatList.get())) {
                order = pos->order_;
                is_pinned = pos->is_pinned_;
                in_current_list = true;
                break;
            }
        }
    }
    if (!in_current_list)
        return;

    long long sortKey = (static_cast<long long>(is_pinned) << 62) | order;

    CallAfter([this, chatId, display_str, sortKey]() {
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
        m_chatList->Insert(display_str, insertPos);
        m_chatList->SetClientObject(insertPos, new CChatClientData(chatId, sortKey));
        m_chatList->Thaw();
    });
}

void CMainWindow::ProcessChatUpdate(td::td_api::object_ptr<td::td_api::chat> chat) {
    if (!chat) {
        return;
    }
    long long chatId = chat->id_;
    m_chats[chatId] = std::move(chat);
    UpdateChatInList(chatId);
}

wxString CMainWindow::FormatMessageForView(const td::td_api::message* message, const wxString& sender_name) {
    if (!message)
        return "";

    wxString sender_str = sender_name;
    wxString content_str = FormatMessageContent(message->content_.get());
    wxString timestamp_str = FormatTimestamp(message->date_);

    wxString sign_str;
    if (!message->author_signature_.empty()) {
        auto chat_it = m_chats.find(message->chat_id_);
        if (chat_it != m_chats.end()) {
            if (chat_it->second->type_->get_id() == td::td_api::chatTypeSupergroup::ID) {
                auto* sg = static_cast<const td::td_api::chatTypeSupergroup*>(chat_it->second->type_.get());
                if (sg && sg->is_channel_) {
                    sign_str = " user " + wxString::FromUTF8(message->author_signature_);
                }
            }
        }
    }
    return wxString::Format("%s%s: %s, received at %s", sender_str, sign_str, content_str, timestamp_str);
}

void CMainWindow::ProcessUpdate(td::td_api::object_ptr<td::td_api::Object> update) {
    switch (update->get_id()) {
        case td::td_api::updateChatFolders::ID: {
            auto chatFoldersUpdate = td::td_api::move_object_as<td::td_api::updateChatFolders>(update);

            auto folders_ptr = std::make_shared<std::vector<td::td_api::object_ptr<td::td_api::chatFolderInfo>>>(
                std::move(chatFoldersUpdate->chat_folders_));

            CallAfter([this, captured_folders_ptr = folders_ptr]() {
                m_folderList->Freeze();
                m_folderList->Clear();
                m_chatFolders.clear();

                m_folderList->Append("All Chats");
                m_folderList->SetClientObject(0, new CFolderClientData(CFolderClientData::ALL_CHATS));
                m_folderList->Append("Archive");
                m_folderList->SetClientObject(1, new CFolderClientData(CFolderClientData::ARCHIVE));

                for (const auto& chatFolderInfo : *captured_folders_ptr) {
                    int pos = m_folderList->GetCount();
                    if (chatFolderInfo && chatFolderInfo->name_) {
                        m_folderList->Append(wxString::FromUTF8(chatFolderInfo->name_->text_->text_));
                        m_folderList->SetClientObject(
                            pos, new CFolderClientData(CFolderClientData::FOLDER, chatFolderInfo->id_));
                    }
                }

                m_folderList->SetSelection(0);
                m_folderList->Thaw();
            });
            LoadChats();
            break;
        }
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
                if (!it->second->default_disable_notification_) {
                    wxString title = wxString::FromUTF8(it->second->title_);
                    wxString content = "No content";
                    if (it->second->last_message_ && it->second->last_message_->content_) {
                        content = FormatMessageContent(&*it->second->last_message_->content_);
                    }
                    g_notificationSender.Send(title, content);
                }
                it->second->unread_count_++;
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
        case td::td_api::updateBasicGroup::ID: {
            auto basic_group_update = td::td_api::move_object_as<td::td_api::updateBasicGroup>(update);
            m_basicGroups[basic_group_update->basic_group_->id_] = std::move(basic_group_update->basic_group_);
            break;
        }
        case td::td_api::updateSupergroup::ID: {
            auto supergroup_update = td::td_api::move_object_as<td::td_api::updateSupergroup>(update);
            m_supergroups[supergroup_update->supergroup_->id_] = std::move(supergroup_update->supergroup_);
            break;
        }
        case td::td_api::updateSecretChat::ID: {
            auto secret_chat_update = td::td_api::move_object_as<td::td_api::updateSecretChat>(update);
            m_secretChats[secret_chat_update->secret_chat_->id_] = std::move(secret_chat_update->secret_chat_);
            break;
        }
        default:
            break;
    }
}

void CMainWindow::OnFolderSelected(wxCommandEvent& event) {
    int selectedIndex = m_folderList->GetSelection();
    if (selectedIndex == wxNOT_FOUND)
        return;
    auto* clientData = static_cast<CFolderClientData*>(m_folderList->GetClientObject(selectedIndex));
    if (!clientData)
        return;

    switch (clientData->GetType()) {
        case CFolderClientData::ALL_CHATS:
            m_currentChatList = td::td_api::make_object<td::td_api::chatListMain>();
            break;
        case CFolderClientData::ARCHIVE:
            m_currentChatList = td::td_api::make_object<td::td_api::chatListArchive>();
            break;
        case CFolderClientData::FOLDER:
            m_currentChatList = td::td_api::make_object<td::td_api::chatListFolder>(clientData->GetFolderId());
            break;
    }

    m_chatList->Clear();
    m_messageView->Clear();
    m_currentChatId = 0;
    m_lastMessageId = 0;

    for (const auto& chat_pair : m_chats) {
        UpdateChatInList(chat_pair.first);
    }

    LoadChats();
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
        if (m_currentChatId != 0) {
            g_mainFrame->getTdManager()->send(td::td_api::make_object<td::td_api::closeChat>(m_currentChatId));
        }
        m_currentChatId = chatId;
        g_mainFrame->getTdManager()->send(td::td_api::make_object<td::td_api::openChat>(m_currentChatId));
        m_messageView->Clear();
        m_lastMessageId = 0;
        LoadMessages(m_currentChatId);
    }
}

void CMainWindow::OnMessageSelected(wxCommandEvent& event) {
    LoadMessages(m_currentChatId);
    OnMessageViewed();
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
    if (m_loadingMore || chatId == 0)
        return;
    m_loadingMore = true;

    if (chatId != m_currentChatId) {
        m_messageView->Clear();
        m_currentChatId = chatId;
        m_lastMessageId = 0;
    }

    auto getHistory = td::td_api::make_object<td::td_api::getChatHistory>(chatId, m_lastMessageId, 0, 50, false);
    g_mainFrame->getTdManager()->send(std::move(getHistory), [this, chatId](TdManager::Object object) {
        m_loadingMore = false;
        if (object->get_id() == td::td_api::messages::ID) {
            auto messages = td::td_api::move_object_as<td::td_api::messages>(object);
            if (messages->messages_.empty()) {
                return;
            }
            m_lastMessageId = messages->messages_.back()->id_;

            auto message_queue = std::make_shared<std::vector<td::td_api::object_ptr<td::td_api::message>>>(
                std::move(messages->messages_));
            auto history_strs = std::make_shared<std::vector<wxString>>(message_queue->size());
            auto message_data = std::make_shared<std::vector<std::pair<long long, long long>>>(message_queue->size());
            auto completed_count = std::make_shared<std::atomic<size_t>>(0);
            size_t total_messages = message_queue->size();

            size_t i = 0;
            for (auto it = message_queue->rbegin(); it != message_queue->rend(); ++it, ++i) {
                const auto& message = *it;
                const size_t history_idx = i;

                (*message_data)[history_idx] = std::make_pair(message->id_, message->chat_id_);

                long long sender_user_id = 0;
                wxString sender_name = "Unknown";

                if (message->sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
                    sender_user_id =
                        static_cast<const td::td_api::messageSenderUser*>(message->sender_id_.get())->user_id_;
                } else if (message->sender_id_->get_id() == td::td_api::messageSenderChat::ID) {
                    auto sender_chat_id =
                        static_cast<const td::td_api::messageSenderChat*>(message->sender_id_.get())->chat_id_;
                    auto chat_it = m_chats.find(sender_chat_id);
                    if (chat_it != m_chats.end()) {
                        sender_name = wxString::FromUTF8(chat_it->second->title_);
                    }
                    (*history_strs)[history_idx] = FormatMessageForView(message.get(), sender_name);
                    if (++(*completed_count) == total_messages) {
                        CallAfter([this, history_strs, message_data]() {
                            m_messageView->Freeze();
                            for (size_t idx = 0; idx < history_strs->size(); ++idx) {
                                if (!(*history_strs)[idx].IsEmpty()) {
                                    m_messageView->Insert((*history_strs)[idx], idx);
                                    m_messageView->SetClientObject(idx,
                                                                   new CMessageClientData((*message_data)[idx].first,
                                                                                          (*message_data)[idx].second));
                                }
                            }
                            m_messageView->Thaw();
                        });
                    }
                    continue;
                }

                GetUser(sender_user_id, [this, history_idx, message_queue, message_ptr = message.get(), history_strs,
                                         message_data, completed_count, total_messages](const td::td_api::user* user) {
                    wxString name =
                        (user) ? wxString::FromUTF8(user->first_name_ + " " + user->last_name_) : "Unknown User";
                    (*history_strs)[history_idx] = FormatMessageForView(message_ptr, name);

                    if (++(*completed_count) == total_messages) {
                        CallAfter([this, history_strs, message_data]() {
                            m_messageView->Freeze();
                            for (size_t idx = 0; idx < history_strs->size(); ++idx) {
                                if (!(*history_strs)[idx].IsEmpty()) {
                                    m_messageView->Insert((*history_strs)[idx], idx);
                                    m_messageView->SetClientObject(idx,
                                                                   new CMessageClientData((*message_data)[idx].first,
                                                                                          (*message_data)[idx].second));
                                }
                            }
                            m_messageView->Thaw();
                        });
                    }
                });
            }

            std::vector<long long> messageIds;
            for (const auto& msg : *message_queue) {
                messageIds.push_back(msg->id_);
            }
            MarkMessagesAsRead(chatId, messageIds);
        }
    });
}

void CMainWindow::AppendMessage(const td::td_api::object_ptr<td::td_api::message>& message) {
    long long sender_user_id = 0;
    if (message->sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
        sender_user_id = static_cast<const td::td_api::messageSenderUser*>(message->sender_id_.get())->user_id_;
    } else {
        wxString sender_name = "A Chat";
        if (message->sender_id_->get_id() == td::td_api::messageSenderChat::ID) {
            auto sender_chat_id =
                static_cast<const td::td_api::messageSenderChat*>(message->sender_id_.get())->chat_id_;
            auto it = m_chats.find(sender_chat_id);
            if (it != m_chats.end()) {
                sender_name = wxString::FromUTF8(it->second->title_);
            }
        }
        wxString formatted_msg = FormatMessageForView(message.get(), sender_name);
        CallAfter([this, formatted_msg, msg = message.get()]() {
            m_messageView->Append(formatted_msg);
            int newIndex = m_messageView->GetCount() - 1;
            m_messageView->SetClientObject(newIndex, new CMessageClientData(msg->id_, msg->chat_id_));
            m_messageView->SetSelection(newIndex);
        });
        return;
    }

    GetUser(sender_user_id, [this, msg = message.get()](const td::td_api::user* user) {
        if (user) {
            wxString name = wxString::FromUTF8(user->first_name_ + " " + user->last_name_);
            wxString formatted_msg = FormatMessageForView(msg, name);
            CallAfter([this, formatted_msg, msg]() {
                m_messageView->Append(formatted_msg);
                int newIndex = m_messageView->GetCount() - 1;
                m_messageView->SetClientObject(newIndex, new CMessageClientData(msg->id_, msg->chat_id_));
                m_messageView->SetSelection(newIndex);
            });
        }
    });
}

void CMainWindow::MarkMessagesAsRead(long long chatId, const std::vector<long long>& messageIds, bool forceRead) {
    auto viewMessages = td::td_api::make_object<td::td_api::viewMessages>();
    viewMessages->chat_id_ = chatId;
    for (const long long& message : messageIds) {
        viewMessages->message_ids_.push_back(message);
    }
    viewMessages->force_read_ = true;
    g_mainFrame->getTdManager()->send(std::move(viewMessages));
}

void CMainWindow::OnMessageViewed() {
    if (m_currentChatId == 0)
        return;

    int visibleStart = m_messageView->GetTopItem();
    int visibleEnd = visibleStart + m_messageView->GetCountPerPage();

    std::vector<long long> visibleMessageIds;
    for (int i = visibleStart; i <= visibleEnd && i < m_messageView->GetCount(); ++i) {
        auto* clientData = static_cast<CMessageClientData*>(m_messageView->GetClientObject(i));
        if (clientData) {
            visibleMessageIds.push_back(clientData->GetMessageId());
        }
    }

    if (!visibleMessageIds.empty()) {
        MarkMessagesAsRead(m_currentChatId, visibleMessageIds);
    }
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
