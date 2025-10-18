#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H
#include <wx/wx.h>

class CFolderClientData final : public wxClientData {
  public:
    enum EFolderType {
        ALL_CHATS,
        ARCHIVE,
        FOLDER
    };
    explicit CFolderClientData(const EFolderType& type, int32_t folderId = 0) : m_type(type), m_folderId(folderId) {}
    EFolderType GetType() const { return m_type; }
    int32_t GetFolderId() const { return m_folderId; }

  private:
    EFolderType m_type;
    int32_t m_folderId;
};

class CChatClientData final : public wxClientData {
  public:
    explicit CChatClientData(long long chatId, long long sortKey) : m_chatId(chatId), m_sortKey(sortKey) {}
    long long GetChatId() const { return m_chatId; }
    long long GetSortKey() const { return m_sortKey; }

  private:
    long long m_chatId;
    long long m_sortKey;
};

class CMessageClientData final : public wxClientData {
  public:
    explicit CMessageClientData(long long messageId, long long chatId) : m_messageId(messageId), m_chatId(chatId) {}
    long long GetMessageId() const { return m_messageId; }
    long long GetChatId() const { return m_chatId; }

  private:
    long long m_messageId;
    long long m_chatId;
};

#endif
