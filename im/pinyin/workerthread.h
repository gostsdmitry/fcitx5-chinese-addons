/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _PINYIN_WORKERTHREAD_H_
#define _PINYIN_WORKERTHREAD_H_

#include <condition_variable>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <queue>

class TaskToken : public fcitx::TrackableObject<TaskToken> {};

class WorkerThread {
public:
    WorkerThread(fcitx::EventDispatcher &dispatcher);
    ~WorkerThread();

    template <typename Ret, typename OnDone>
    FCITX_NODISCARD std::unique_ptr<TaskToken>
    addTask(std::packaged_task<Ret()> task, OnDone onDone) {
        std::future<Ret> future = task.get_future();
        std::function<void()> taskFunction =
            [task = std::make_shared<decltype(task)>(
                 std::move(task))]() mutable { (*task)(); };
        std::function<void()> callback = [onDone = std::move(onDone),
                                          future = future.share()]() mutable {
            onDone(future);
        };

        return addTaskImpl(std::move(taskFunction), std::move(callback));
    }

private:
    std::unique_ptr<TaskToken> addTaskImpl(std::function<void()> task,
                                           std::function<void()> onDone);
    static void runThread(WorkerThread *self) { self->run(); }
    void run();

    struct Task {
        std::function<void()> task;
        std::function<void()> callback;
        fcitx::TrackableObjectReference<TaskToken> context;
    };

    fcitx::EventDispatcher &dispatcher_;
    std::mutex mutex_;
    std::queue<Task, std::list<Task>> queue_;
    bool exit_ = false;
    std::condition_variable condition_;

    // Must be the last member
    std::thread thread_;
};

#endif