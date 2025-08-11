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
    CMainWindow(wxSimplebook* book);

    void ProcessUpdate(td::td_api::object_ptr<td::td_api::Object> update);

  private:
    void ProcessChatUpdate(td::td_api::object_ptr<td::td_api::chat> chat);

    void OnChatSelected(wxCommandEvent& event);
    void OnSendPressed(wxCommandEvent& event);
    void LoadChats();
    void GetUser(long long userId, std::function<void(const td::td_api::user*)> callback);
    void LoadMessages(long long chatId);
    void UpdateChatInList(long long chatId);
    void OnScroll(wxScrollWinEvent& event);

    wxSimplebook* m_book;
    wxListBox* m_chatList;
    wxTextCtrl* m_messageView;
    wxTextCtrl* m_messageInput;
    wxButton* m_sendButton;
    wxSplitterWindow* m_splitter;
    long long m_currentChatId{0};

    long long m_lastChatId{0};
    long long m_lastChatOrder{0x7FFFFFFFFFFFFFFF};
    bool m_allChatsLoaded{false};
    std::map<long long, td::td_api::object_ptr<td::td_api::chat>> m_chats;
    std::map<long long, td::td_api::object_ptr<td::td_api::user>> m_users;
};

#endif
