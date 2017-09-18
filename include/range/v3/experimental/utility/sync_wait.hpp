// Range v3 library
//
//  Copyright Eric Niebler 2017
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// With thanks to Lewis Baker and Gor Nishanov
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_EXPERIMENTAL_UTILITY_SYNC_WAIT_HPP
#define RANGES_V3_EXPERIMENTAL_UTILITY_SYNC_WAIT_HPP

#include <range/v3/detail/config.hpp>
#if RANGES_CXX_COROUTINES >= RANGES_CXX_COROUTINES_TS1
#include <condition_variable>
#include <exception>
#include <experimental/coroutine>
#include <mutex>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/invoke.hpp> // for reference_wrapper
#include <range/v3/utility/swap.hpp>

namespace ranges
{
    inline namespace v3
    {
        namespace experimental
        {
            /// \cond
            namespace detail
            {
                struct sync_wait_promise_base
                {
                    std::exception_ptr eptr_;
                    void unhandled_exception() noexcept
                    {
                        eptr_ = std::current_exception();
                    }
                    void get() const
                    {
                        if(eptr_)
                            std::rethrow_exception(eptr_);
                    }
                };
                struct sync_wait_void_promise : sync_wait_promise_base
                {
                    void return_void() const noexcept {}
                };
                template<typename T>
                struct sync_wait_value_promise : sync_wait_promise_base
                {
                    using value_type = meta::if_<std::is_reference<T>, reference_wrapper<T>, T>;
                    bool is_set_{false};
                    alignas(value_type) char buffer_[sizeof(value_type)];
                    template<typename U>
                    void return_value(U &&u)
                    {
                        RANGES_EXPECT(!is_set_);
                        ::new((void *)&buffer_) value_type(static_cast<U &&>(u));
                        is_set_ = true;
                    }
                    ~sync_wait_value_promise()
                    {
                        if(is_set_)
                            static_cast<value_type *>((void *)&buffer_)->~value_type();
                    }
                    T &&get()
                    {
                        this->sync_wait_promise_base::get();
                        return static_cast<T &&>(*static_cast<value_type *>((void *)&buffer_));
                    }
                };

                struct sync_wait_fn
                {
                    template<typename T, CONCEPT_REQUIRES_(Awaitable<T>())>
                    co_result_t<T> operator()(T &&t) const
                    {
                        struct _
                        {
                            // In this awaitable's final_suspend, signal a condition variable.
                            // Then in the awaitable's wait() function, wait on the condition
                            // variable.
                            struct promise_type
                              : meta::if_<
                                    std::is_void<co_result_t<T>>,
                                    sync_wait_void_promise,
                                    sync_wait_value_promise<co_result_t<T>>>
                            {
                                std::mutex mtx_;
                                std::condition_variable cnd_;
                                bool done_{false};

                                struct final_suspend_result : std::experimental::suspend_always
                                {
                                    void await_suspend(
                                        std::experimental::coroutine_handle<promise_type> h)
                                        const noexcept
                                    {
                                        auto &promise = h.promise();
                                        {
                                            std::lock_guard<std::mutex> g(promise.mtx_);
                                            promise.done_ = true;
                                        }
                                        promise.cnd_.notify_all();
                                    }
                                };
                                std::experimental::suspend_never initial_suspend() const
                                    noexcept
                                {
                                    return {};
                                }
                                final_suspend_result final_suspend() noexcept
                                {
                                    return {};
                                }
                                auto get_return_object() noexcept -> _
                                {
                                    return _{*this};
                                }
                            };
                            std::experimental::coroutine_handle<promise_type> coro_;

                            explicit _(promise_type &p) noexcept
                              : coro_(std::experimental::coroutine_handle<
                                      promise_type>::from_promise(p))
                            {
                            }
                            _(_ &&that) noexcept
                              : coro_(ranges::exchange(that.coro_, {}))
                            {
                            }
                            ~_()
                            {
                                if(coro_)
                                    coro_.destroy();
                            }
                            co_result_t<T> wait()
                            {
                                auto &promise = coro_.promise();
                                std::unique_lock<std::mutex> lock(promise.mtx_);
                                promise.cnd_.wait(lock, [&promise] { return promise.done_; });
                                return promise.get();
                            }
                        };

                        return [](T &t) -> _ { co_return co_await t; }(t).wait();
                    }
                };
            } // namespace detail
            /// \endcond

            RANGES_INLINE_VARIABLE(detail::sync_wait_fn, sync_wait)

        } // namespace experimental
    } // namespace v3
} // namespace ranges

#endif // RANGES_CXX_COROUTINES >= RANGES_CXX_COROUTINES_TS1
#endif // RANGES_V3_EXPERIMENTAL_UTILITY_SYNC_WAIT_HPP