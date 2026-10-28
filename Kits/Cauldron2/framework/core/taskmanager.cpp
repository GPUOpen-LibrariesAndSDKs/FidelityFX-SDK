// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "taskmanager.h"
#include "contentmanager.h"
#include "framework.h"
#include "../misc/assert.h"

#include <functional>

namespace cauldron
{
    // Priority of the task currently executing on this worker thread. Used to resolve
    // TaskPriority::Inherit so that tasks spawned by a running task (e.g. a completion task
    // that dispatches the next stage of a load) adopt that task's priority. Defaults to
    // Normal on the main thread and any other non-worker thread.
    static thread_local TaskPriority t_currentPriority = TaskPriority::Normal;

    TaskManager::TaskManager()
    {
    }

    TaskManager::~TaskManager()
    {
    }

    int32_t TaskManager::Init(uint32_t threadPoolSize)
    {
        // Reserve a small number of workers dedicated to the high-priority queue so that low-cost,
        // latency-sensitive tasks are never starved by long-running normal tasks. These are added on
        // top of the normal pool; they idle on a condition variable when there is no high-priority work.
        const uint32_t hiPriThreads = (threadPoolSize > 2) ? 2u : 1u;

        for (uint32_t i = 0; i < threadPoolSize; ++i)
            m_ThreadPool.emplace_back([this]() { this->TaskExecutor(false); });

        for (uint32_t i = 0; i < hiPriThreads; ++i)
            m_ThreadPool.emplace_back([this]() { this->TaskExecutor(true); });

        return 0;
    }

    void TaskManager::Shutdown()
    {
        // Before shutting down, ensure no loading is going on in the background, as it can hang
        while (GetContentManager()->IsCurrentlyLoading()) {}

        // Flag all threads to shutdown
        {
            std::unique_lock<std::mutex> lock(m_CriticalSection);
            m_ShuttingDown = true;
            m_NormalCondition.notify_all();
            m_HiCondition.notify_all();
        }

        // Wait for all threads to be done
        std::vector<std::thread>::iterator iter = m_ThreadPool.begin();
        while (iter != m_ThreadPool.end())
        {
            iter->join();
            iter = m_ThreadPool.erase(iter);
        }
    }

    void TaskManager::AddTask(Task& newTask)
    {
        // Resolve Inherit to the priority of the task currently running on this thread (Normal on the main thread).
        if (newTask.Priority == TaskPriority::Inherit)
            newTask.Priority = t_currentPriority;

        std::unique_lock<std::mutex> lock(m_CriticalSection);
        if (newTask.Priority == TaskPriority::High)
        {
            m_HiTaskQueue.push(std::move(newTask));

            // Wake a reserved worker, and let an idle normal worker help too
            m_HiCondition.notify_one();
            m_NormalCondition.notify_one();
        }
        else
        {
            m_TaskQueue.push(std::move(newTask));

            // Wake a single normal worker to pick up the task
            m_NormalCondition.notify_one();
        }
    }

    void TaskManager::AddTaskList(std::queue<Task>& newTaskList)
    {
        std::unique_lock<std::mutex> lock(m_CriticalSection);

        bool pushedHi     = false;
        bool pushedNormal = false;
        while (newTaskList.size())
        {
            Task& task = newTaskList.front();

            // Resolve Inherit to the priority of the task currently running on this thread (Normal on the main thread).
            if (task.Priority == TaskPriority::Inherit)
                task.Priority = t_currentPriority;

            if (task.Priority == TaskPriority::High)
            {
                m_HiTaskQueue.push(std::move(task));
                pushedHi = true;
            }
            else
            {
                m_TaskQueue.push(std::move(task));
                pushedNormal = true;
            }
            newTaskList.pop();
        }

        // Wake up all threads that can service the work we just queued
        if (pushedHi)
            m_HiCondition.notify_all();
        if (pushedHi || pushedNormal)
            m_NormalCondition.notify_all();
    }

    // Runs for each thread and executes any waiting tasks when available.
    // Reserved (hiPriOnly) workers only service the high-priority queue; normal workers
    // service the high-priority queue first, then fall back to the normal queue.
    void TaskManager::TaskExecutor(bool hiPriOnly)
    {
        while (true)
        {
            Task taskToExecute;
            {
                // Try to unload a task to execute
                std::unique_lock<std::mutex> lock(m_CriticalSection);
                std::condition_variable& queueCondition = hiPriOnly ? m_HiCondition : m_NormalCondition;

                // Sleep until work this worker can service is available, or we are shutting down
                queueCondition.wait(lock, [this, hiPriOnly] {
                    if (m_ShuttingDown)
                        return true;
                    if (!m_HiTaskQueue.empty())
                        return true;
                    return !hiPriOnly && !m_TaskQueue.empty();
                });

                if (m_ShuttingDown)
                    break;

                // High-priority queue first (both worker kinds); normal workers fall back to the normal queue
                if (!m_HiTaskQueue.empty())
                {
                    taskToExecute = m_HiTaskQueue.front();
                    m_HiTaskQueue.pop();
                }
                else if (!hiPriOnly && !m_TaskQueue.empty())
                {
                    taskToExecute = m_TaskQueue.front();
                    m_TaskQueue.pop();
                }
                else
                {
                    continue;   // spurious wake-up, nothing to do
                }
            }

            while (taskToExecute.pTaskFunction)
            {
                // Update the propagated priority for tasks this task enqueues with Inherit. Dequeued
                // tasks always carry a concrete priority (resolved at enqueue). Completion tasks always
                // carry Inherit (enforced by TaskCompletionCallback), so they continue propagating
                // whatever the group's priority was.
                if (taskToExecute.Priority != TaskPriority::Inherit)
                    t_currentPriority = taskToExecute.Priority;

                // Execute the task
                taskToExecute.pTaskFunction(taskToExecute.pTaskParam);

                // When we are done, if there was a completion callback, tick it down and execute if needed
                if (taskToExecute.pTaskCompletionCallback)
                {
                    // If this was the last task on which we were waiting, execute the completion task now
                    if (--taskToExecute.pTaskCompletionCallback->TaskCount == 0)
                    {
                        auto callbackMemPtr = taskToExecute.pTaskCompletionCallback;
                        taskToExecute = taskToExecute.pTaskCompletionCallback->CompletionTask;
                        delete callbackMemPtr;
                        continue;
                    }
                }

                // No completion task to run, fetch another task or sleep
                break;
            }

            // Reset so that any Inherit tasks enqueued outside of a running task context (e.g. in a
            // destructor triggered by taskToExecute going out of scope) don't accidentally inherit this
            // task's priority on the next pick-up.
            t_currentPriority = TaskPriority::Normal;
        }
    }

} // namespace cauldron
