#include "tdManager.h"
#include <atomic>

TdManager::TdManager()
    : client_manager_(std::make_unique<td::ClientManager>()), running_(true) {
    client_id_ = client_manager_->create_client_id();
    worker_thread_ = std::thread(&TdManager::run, this);
}

TdManager::~TdManager() {
    running_ = false;
    send(td::td_api::make_object<td::td_api::close>());
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void TdManager::send(td::td_api::object_ptr<td::td_api::Function> function, UpdateCallback callback) {
    auto query_id = nextQueryId();
    if (callback) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.emplace(query_id, std::move(callback));
    }
    if (client_manager_) {
        client_manager_->send(client_id_, query_id, std::move(function));
    }
}

void TdManager::setUpdateCallback(UpdateCallback callback) {
    std::lock_guard<std::mutex> lock(update_mutex_);
    update_callback_ = std::move(callback);
}

void TdManager::run() {
    while (running_) {
        auto response = client_manager_->receive(10.0);
        if (response.object) {
            processResponse(std::move(response));
        }
    }
}

void TdManager::processResponse(td::ClientManager::Response response) {
    if (response.request_id == 0) {
        processUpdate(std::move(response.object));
    } else {
        UpdateCallback handler;
        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            auto it = handlers_.find(response.request_id);
            if (it != handlers_.end()) {
                handler = std::move(it->second);
                handlers_.erase(it);
            }
        }
        if (handler) {
            handler(std::move(response.object));
        }
    }
}

void TdManager::processUpdate(Object update) {
    if (update->get_id() == td::td_api::updateAuthorizationState::ID) {
        auto* auth_update = static_cast<td::td_api::updateAuthorizationState*>(update.get());
        if (auth_update->authorization_state_->get_id() == td::td_api::authorizationStateClosed::ID) {
            running_ = false;
        }
    }

    std::lock_guard<std::mutex> lock(update_mutex_);
    if (update_callback_) {
        update_callback_(std::move(update));
    }
}

std::uint64_t TdManager::nextQueryId() {
    static std::atomic<std::uint64_t> current_query_id_{0};
    return ++current_query_id_;
}
