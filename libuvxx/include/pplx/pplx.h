/***
* ==++==
*
* Copyright (c) Microsoft Corporation. All rights reserved. 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* pplx.h
*
* Parallel Patterns Library
*
* For the latest on this and related APIs, please see http://casablanca.codeplex.com.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once

#ifndef _PPLX_H
#define _PPLX_H


#ifndef _MS_WINDOWS
#if defined(WIN32) || defined(__cplusplus_winrt)
#define _MS_WINDOWS
#endif
#endif // _MS_WINDOWS

#ifdef _NO_PPLXIMP
#define _PPLXIMP
#else
#ifdef _PPLX_EXPORT
#define _PPLXIMP __declspec(dllexport)
#else
#define _PPLXIMP __declspec(dllimport)
#endif
#endif

#include "compat/linux_compat.h"
#include "pplx/pplxlinux.h"

// Common implementation across all the non-concrt versions
#include "pplx/pplxcancellation_token.h"
#include <functional>

// conditional expression is constant
#pragma warning(push)
#pragma warning(disable: 4127)

#pragma pack(push,_CRT_PACKING)

/// <summary>
///     The <c>pplx</c> namespace provides classes and functions that give you access to the Concurrency Runtime,
///     a concurrent programming framework for C++. For more information, see <see cref="Concurrency Runtime"/>.
/// </summary>
/**/
namespace uvxx { namespace pplx
{

/// <summary>
/// Sets the ambient scheduler to be used by the PPL constructs.
/// </summary>
void set_ambient_scheduler(const std::shared_ptr<uvxx::pplx::scheduler_interface>& _Scheduler);

/// <summary>
/// Gets the ambient scheduler to be used by the PPL constructs
/// </summary>
std::shared_ptr<uvxx::pplx::scheduler_interface> get_ambient_scheduler();

uvxx::pplx::scheduler_interface* get_ambient_scheduler_ptr();

namespace details
{
    //
    // An internal exception that is used for cancellation. Users do not "see" this exception except through the
    // resulting stack unwind. This exception should never be intercepted by user code. It is intended
    // for use by the runtime only.
    //
    class _Interruption_exception : public std::exception
    {
    public:
        _Interruption_exception(){}
    };

    template<typename _T>
    struct _AutoDeleter
    {
        _AutoDeleter(_T *_PPtr) : _Ptr(_PPtr) {}
        ~_AutoDeleter () { delete _Ptr; } 
        _T *_Ptr;
    };

    struct _TaskProcHandle
    {
        _TaskProcHandle()
        {
        }

        virtual ~_TaskProcHandle() {}
        virtual void invoke() const = 0;

        static void _RunChoreBridge(void * _Parameter)
        {
            auto _PTaskHandle = static_cast<_TaskProcHandle *>(_Parameter);
            _AutoDeleter<_TaskProcHandle> _AutoDeleter(_PTaskHandle);
            _PTaskHandle->invoke();
        }
    };

    enum _TaskInliningMode
    {
        // Disable inline scheduling
        _NoInline = 0,
        // Let runtime decide whether to do inline scheduling or not
        _DefaultAutoInline = 16,
        // Always do inline scheduling
        _ForceInline = -1,
    };

    // This is an abstraction that is built on top of the scheduler to provide these additional functionalities
    // - Ability to wait on a work item
    // - Ability to cancel a work item
    // - Ability to inline work on invocation of RunAndWait
    class _TaskCollectionImpl 
    {
    public:

        typedef _TaskProcHandle _TaskProcHandle_t;

        _TaskCollectionImpl(const scheduler_ptr& _PScheduler) 
            : _M_pScheduler(_PScheduler)
        {
            _M_pScheduler = _PScheduler;
        }

        void _ScheduleTask(_TaskProcHandle_t* _PTaskHandle, _TaskInliningMode _InliningMode)
        {
            if (_InliningMode == _ForceInline)
            {
                _TaskProcHandle_t::_RunChoreBridge(_PTaskHandle);
            }
            else
            {
                _M_pScheduler->schedule(_TaskProcHandle_t::_RunChoreBridge, _PTaskHandle);
            }
        }

        void _Cancel()
        {
            // No cancellation support
        }

        void _RunAndWait()
        {
            // No inlining support yet
            _Wait();
        }

        void _Wait()
        {
            _M_Completed.wait();
        }

        void _Complete()
        {
            _M_Completed.set();
        }

        scheduler_ptr _GetScheduler() const
        {
            return _M_pScheduler;
        }

        // Fire and forget
        static void _RunTask(TaskProc_t _Proc, void * _Parameter, _TaskInliningMode _InliningMode)
        {
            if (_InliningMode == _ForceInline)
            {
                _Proc(_Parameter);
            }
            else
            {
                // Schedule the work on the ambient scheduler
                get_ambient_scheduler_ptr()->schedule(_Proc, _Parameter);
            }
        }

        static bool _Is_cancellation_requested()
        {
            // We do not yet have the ability to determine the current task. So return false always
            return false;
        }
    private:

        extensibility::event_t _M_Completed;
        scheduler_ptr _M_pScheduler;
    };

    // For create_async lambdas that return a (non-task) result, we oversubscriber the current task for the duration of the
    // lambda.
    struct _Task_generator_oversubscriber  {};

    typedef _TaskCollectionImpl _TaskCollection_t;
    typedef _TaskInliningMode _TaskInliningMode_t;
    typedef _Task_generator_oversubscriber _Task_generator_oversubscriber_t;

} // namespace details

} }// namespace uvxx { namespace pplx

#pragma pack(pop)
#pragma warning(pop)

#endif // _PPLX_H