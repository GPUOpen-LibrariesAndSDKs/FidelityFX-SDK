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

#pragma once

#include "../misc/helpers.h"
#include "../misc/threadsafe_queue.h"

#include <functional>
#include <queue>
#include <memory>
#include <thread>
#include <vector>

namespace cauldron
{
    typedef std::function<void(void*)> TaskFunc;

    struct TaskCompletionCallback;

    /**
     * @enum TaskPriority
     *
     * Selects which queue a task is dispatched to. High-priority tasks are serviced by a small pool of
     * reserved worker threads so that low-cost, latency-sensitive work is not starved by long-running
     * normal tasks. Inherit adopts the priority of the task currently executing on the enqueuing thread
     * (or Normal when enqueued from the main thread), so tasks spawned by a running task propagate its
     * priority without every call site having to specify it.
     *
     * @ingroup CauldronCore
     */
    enum class TaskPriority : uint8_t
    {
        Normal = 0,     ///< Default queue, serviced by the general worker pool.
        High,           ///< High-priority queue for low-cost, latency-sensitive work. Serviced by all workers (reserved workers exclusively, normal workers hi-first). Reserved workers guarantee a throughput floor even when the normal pool is saturated; they do not restrict which workers can consume this queue.
        Inherit         ///< Resolved at enqueue time to the priority of the currently running task (Normal if none).
    };

    /**
     * @struct Task
     *
     * Used to dispatch work to the thread pool managed by the task manager.
     *
     * @ingroup CauldronCore
     */
    struct Task
    {
        TaskFunc                pTaskFunction;                    ///< The task to execute
        void*                   pTaskParam;                       ///< Parameters (in the form of a void pointer) to pass to the task (NOTE** calling code is responsible for the memory backing parameter pointer).
        TaskCompletionCallback* pTaskCompletionCallback;          ///< If this task is part of a larger group of tasks that require post-completion synchronization, they will be associated with a task sync primitive
        TaskPriority            Priority = TaskPriority::Inherit; ///< Which queue to dispatch to. Defaults to Inherit (adopts the running task's priority, or Normal from the main thread).

        Task(TaskFunc pTaskFunction, void* pTaskParam = nullptr, TaskCompletionCallback* pCompletionCallback = nullptr, TaskPriority priority = TaskPriority::Inherit) :
            pTaskFunction(pTaskFunction), pTaskParam(pTaskParam), pTaskCompletionCallback(pCompletionCallback), Priority(priority) {}

    private:
        friend class TaskManager;
        Task() = default;
    };

    /**
     * @struct TaskCompletionCallback
     *
     * Used to schedule a task to run after all associated tasks have run.
     * Ensures we don't have to Wait() on tasks to complete so that we can fully
     * use all our threads at all times (as long as work is available)
     *
     * @ingroup CauldronCore
     */
    struct TaskCompletionCallback
    {
        Task                    CompletionTask;     ///< The task to execute once the task count reaches 0. Always runs inline on the worker that decrements the count to zero — it is never requeued. Priority is always Inherit: propagation to tasks spawned by the completion task is controlled by explicit priorities at those spawn sites, not here.
        std::atomic_uint        TaskCount;          ///< Number of tasks this callback is paired with. Count will tick down upon completion of each dependent task.

        TaskCompletionCallback(Task completionTask, uint32_t taskCount = 1) :
            CompletionTask(completionTask), TaskCount(taskCount)
        {
            // Completion tasks always inherit the priority context of the worker that fires them.
            // Any priority set on the task by the caller is ignored — control propagation at spawn
            // sites inside the completion function instead.
            CompletionTask.Priority = TaskPriority::Inherit;
        }

    private:
        TaskCompletionCallback() = delete;
    };

    /**
     * @class TaskManager
     *
     * The TaskManager instance manages our thread pool. Currently, only loading of content is handled
     * asynchronously (the main loop is single threaded).
     *
     * @ingroup CauldronCore
     */
    class TaskManager
    {
    public:

        /**
         * @brief   Constructor with default behavior.
         */
        TaskManager();

        /**
         * @brief   Destructor with default behavior.
         */
        virtual ~TaskManager();

        /**
         * @brief   Initialization function for the TaskManager. Dictates the size of our thread pool.
         */
        int32_t Init(uint32_t threadPoolSize);

        /**
         * @brief   Shuts down the task manager and joins all threads. Called from framework shut down procedures.
         */
        void Shutdown();

        /**
         * @brief   Enqueues a task for execution.
         */
        void AddTask(Task& newTask);

        /**
         * @brief   Enqueues multiple tasks for execution.
         */
        void AddTaskList(std::queue<Task>& newTaskList);

    private:

        // No Copy, No Move
        NO_COPY(TaskManager);
        NO_MOVE(TaskManager);

        // Worker loop. Reserved (hiPriOnly) workers only ever service the high-priority queue;
        // normal workers service the high-priority queue first, then fall back to the normal queue.
        void TaskExecutor(bool hiPriOnly);

        bool                        m_ShuttingDown = false;
        std::vector<std::thread>    m_ThreadPool = {};
        std::queue<Task>            m_TaskQueue = {};       // Normal-priority work
        std::queue<Task>            m_HiTaskQueue = {};     // High-priority work (serviced by reserved workers, and by normal workers when free)
        std::mutex                  m_CriticalSection;
        std::condition_variable     m_NormalCondition;      // Wakes normal workers (either queue has work)
        std::condition_variable     m_HiCondition;          // Wakes reserved workers (high-priority queue has work)
    };

} // namespace cauldron
