#ifndef TD_MANAGER_H
#define TD_MANAGER_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <thread>
#include <vector>

class TdManager {
  public:
    using Object = td::td_api::object_ptr<td::td_api::Object>;
    using UpdateCallback = std::function<void(Object)>;

    TdManager();
    ~TdManager();

    void send(td::td_api::object_ptr<td::td_api::Function> function, UpdateCallback callback = nullptr);
    void setUpdateCallback(UpdateCallback callback);

  private:
    void run();
    void processResponse(td::ClientManager::Response response);
    void processUpdate(Object update);

    std::uint64_t nextQueryId();

    std::unique_ptr<td::ClientManager> client_manager_;
    std::int32_t client_id_;
    std::atomic<bool> running_;
    std::thread worker_thread_;

    UpdateCallback update_callback_;
    std::map<std::uint64_t, UpdateCallback> handlers_;
    std::mutex handlers_mutex_;
};

#endif
