/**
 *	\file
 */

#pragma once

#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace asio_pq {
namespace detail {

template <typename Handler>
class wrapper {
private:
	Handler h_;
protected:
	Handler & handler () noexcept {
		return h_;
	}
public:
	wrapper () = delete;
	explicit wrapper (Handler h) noexcept(
		std::is_nothrow_move_constructible<Handler>::value
	)	:	h_(std::move(h))
	{	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->h_));
	}
	friend bool asio_handler_is_continuation (wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->h_));
	}
	friend void * asio_handler_allocate (std::size_t num, wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->h_));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->h_));
	}
};

template <typename Handler>
class read_wrapper : public wrapper<Handler> {
private:
	using base = wrapper<Handler>;
public:
	using base::base;
	void operator () (boost::system::error_code ec) {
		base::handler().read(ec);
	}
};

template <typename Handler>
read_wrapper<Handler> make_read_wrapper (Handler h) noexcept(std::is_nothrow_move_constructible<Handler>::value) {
	return read_wrapper<Handler>(std::move(h));
}

template <typename Handler>
class write_wrapper : public wrapper<Handler> {
private:
	using base = wrapper<Handler>;
public:
	using base::base;
	void operator () (boost::system::error_code ec) {
		base::handler().write(ec);
	}
};

template <typename Handler>
write_wrapper<Handler> make_write_wrapper (Handler h) noexcept(std::is_nothrow_move_constructible<Handler>::value) {
	return write_wrapper<Handler>(std::move(h));
}

}
}
