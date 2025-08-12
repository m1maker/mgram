#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include "tdManager.h"

#include <algorithm>
#include <map>
#include <wx/simplebook.h>
#include <wx/splitter.h>
#include <wx/wx.h>

class CMainWindow final : public wxPanel {
  public:
    enum EChatWindowState : unsigned char {
        MESSAGING,
        PROFILE,
        EDIT
    };

    CMainWindow(wxSimplebook* book);

    void ProcessUpdate(td::td_api::object_ptr<td::td_api::Object> update);

    void SwitchChatWindowState(const EChatWindowState& state);

  private:
    void ProcessChatUpdate(td::td_api::object_ptr<td::td_api::chat> chat);
    void FormatAndUpdateChatListEntry(const td::td_api::object_ptr<td::td_api::chat>& chat,
                                      const td::td_api::user* user);

    wxString FormatMessageForView(const td::td_api::message* message, const wxString& sender_name);

    void OnChatSelected(wxCommandEvent& event);
    void OnSendPressed(wxCommandEvent& event);
    void LoadChats();
    void GetUser(long long userId, std::function<void(const td::td_api::user*)> callback);
    void LoadMessages(long long chatId);
    void AppendMessage(const td::td_api::object_ptr<td::td_api::message>& message);

    void UpdateChatInList(long long chatId);
    void OnMessageSelected(wxCommandEvent& event);

    wxSimplebook* m_book;
    wxListBox* m_chatList;
    wxListBox* m_messageView;
    wxStaticText* m_messageInputLabel; // We store it as member, because it can be broadcast or payed message.
    wxTextCtrl* m_messageInput;
    wxButton* m_attachMediaButton;
    wxButton* m_sendButton;
    wxSplitterWindow* m_splitter;
    long long m_currentChatId{0};

    long long m_lastChatId{0};
    long long m_lastChatOrder{0x7FFFFFFFFFFFFFFF};
    long long m_lastMessageId{0};
    EChatWindowState m_ChatState{MESSAGING};
    bool m_allChatsLoaded{false};
    bool m_loadingMore{false};
    std::map<long long, td::td_api::object_ptr<td::td_api::chat>> m_chats;
    std::map<long long, td::td_api::object_ptr<td::td_api::user>> m_users;
    std::map<long long, td::td_api::object_ptr<td::td_api::basicGroup>> m_basicGroups;
    std::map<long long, td::td_api::object_ptr<td::td_api::supergroup>> m_supergroups;
    std::map<long long, td::td_api::object_ptr<td::td_api::secretChat>> m_secretChats;
};

#endif
