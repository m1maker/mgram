#include "tdManager.h"

TdManager::TdManager()
    : client_manager_(std::make_unique<td::ClientManager>()), client_id_(client_manager_->create_client_id()),
      running_(true), worker_thread_(&TdManager::run, this) {}

TdManager::~TdManager() {
    running_ = false;
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
    client_manager_->send(client_id_, query_id, std::move(function));
}

void TdManager::setUpdateCallback(UpdateCallback callback) {
    update_callback_ = std::move(callback);
}

void TdManager::run() {
    while (running_) {
        auto response = client_manager_->receive(1.0);
        if (response.object) {
            processResponse(std::move(response));
        }
    }
}

void TdManager::processResponse(td::ClientManager::Response response) {
    if (response.request_id == 0) {
        processUpdate(std::move(response.object));
    } else {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find(response.request_id);
        if (it != handlers_.end()) {
            it->second(std::move(response.object));
            handlers_.erase(it);
        }
    }
}

void TdManager::processUpdate(Object update) {
    if (update_callback_) {
        update_callback_(std::move(update));
    }
}

std::uint64_t TdManager::nextQueryId() {
    static std::atomic<std::uint64_t> current_query_id_{0};
    return ++current_query_id_;
}
