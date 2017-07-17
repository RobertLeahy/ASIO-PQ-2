/**
 *	\file
 */

#pragma once

#include "connection.hpp"
#include "detail/op.hpp"
#include "detail/wrapper.hpp"
#include "error.hpp"
#include <beast/core/async_result.hpp>
#include <beast/core/handler_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <libpq-fe.h>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace asio_pq {

namespace detail {

using async_connect_signature = void (boost::system::error_code);

template <typename Handler>
class async_connect_op {
//	TODO (?): If simultaneous reads and writes
//	during connect ever become a thing:
//
//	-	Don't use std::move in either read()
//		or write(), whichever is called first
//		when both are called
//	-	Dispatch read and write asynchronous
//		operations through a strand
private:
	class state {
	public:
		state () = default;
		state (const state &) = default;
		state (state &&) = default;
		state & operator = (const state &) = default;
		state & operator = (state &&) = default;
		connection & handle;
		boost::optional<boost::system::error_code> result;
		bool read;
		bool write;
		state (const Handler &, connection & handle)
			:	handle(handle),
				read(false),
				write(false)
		{	}
	};
	using pointer = beast::handler_ptr<state, Handler>;
	pointer ptr_;
	void begin_fail (boost::system::error_code ec) {
		assert(!ptr_->result);
		assert(!ptr_->read);
		assert(!ptr_->write);
		ptr_->result = ec;
		boost::asio::io_service & ios = ptr_->handle.get_io_service();
		ios.post(std::move(*this));
	}
	void complete () {
		assert(ptr_->result);
		assert(!ptr_->read);
		assert(!ptr_->write);
		boost::system::error_code ec = *ptr_->result;
		ptr_.invoke(ec);
	}
	void complete_if () {
		if (ptr_->read || ptr_->write) return;
		complete();
	}
	void fail (boost::system::error_code ec) {
		assert(!ptr_->result);
		ptr_->result = ec;
		complete_if();
	}
	bool check () {
		if (!ptr_->result) return true;
		complete();
		return false;
	}
	void read () {
		ptr_->handle.socket([&] (auto & socket) {
			this->ptr_->read = true;
			detail::async_readable(
				socket,
				detail::make_read_wrapper(std::move(*this))
			);
		});
	}
	void write () {
		ptr_->handle.socket([&] (auto & socket) {
			this->ptr_->write = true;
			detail::async_writable(
				socket,
				detail::make_write_wrapper(std::move(*this))
			);
		});
	}
	void succeed () {
		assert(!ptr_->result);
		ptr_->result = boost::in_place();
		complete_if();
	}
	void impl (boost::system::error_code ec) {
		if (!check()) return;
		if (ec) {
			fail(ec);
			return;
		}
		PostgresPollingStatusType status = PQconnectPoll(ptr_->handle);
		switch (status) {
		case PGRES_POLLING_WRITING:
			write();
			break;
		case PGRES_POLLING_READING:
			read();
			break;
		case PGRES_POLLING_FAILED:
		default:
			fail(make_error_code(error::polling_failed));
			break;
		case PGRES_POLLING_OK:
			succeed();
			break;
		}
	}
public:
	async_connect_op () = delete;
	async_connect_op (const async_connect_op &) = default;
	async_connect_op (async_connect_op &&) = default;
	async_connect_op & operator = (const async_connect_op &) = default;
	async_connect_op & operator = (async_connect_op &&) = default;
	async_connect_op (Handler h, connection & handle)
		:	ptr_(std::move(h), handle)
	{	}
	void read (boost::system::error_code ec) {
		ptr_->read = false;
		impl(ec);
	}
	void write (boost::system::error_code ec) {
		ptr_->write = false;
		impl(ec);
	}
	void begin () {
		if (PQstatus(ptr_->handle) == CONNECTION_BAD) {
			begin_fail(make_error_code(error::connection_bad));
			return;
		}
		auto ec = ptr_->handle.duplicate_socket();
		if (ec) {
			ptr_->result = ec;
			boost::asio::io_service & ios = ptr_->handle.get_io_service();
			ios.post(std::move(*this));
			return;
		}
		//	If you have yet to call PQconnectPoll, i.e.,
		//	just after the call to PQconnectStart, behave
		//	as if it last returned PGRES_POLLING_WRITING.
		write();
	}
	void operator () () {
		complete();
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_connect_op * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->ptr_.handler()));
	}
	friend bool asio_handler_is_continuation (async_connect_op * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->ptr_.handler()));
	}
	friend void * asio_handler_allocate (std::size_t num, async_connect_op * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->ptr_.handler()));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, async_connect_op * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->ptr_.handler()));
	}
};

}

/**
 *	Asynchronously connects to a PostgreSQL server.
 *
 *	\tparam CompletionToken
 *		The type of completion token an instance
 *		of which shall be used to notify the caller
 *		of completion.
 *
 *	\param [in] conn
 *		A \ref connection object wrapping a libpq connection
 *		handle. The reference to this object must remain valid
 *		for the lifetime of the asynchronous operation or the
 *		behavior is undefined.
 *	\param [in] token
 *		The completion token which shall be used to notify
 *		the caller of completion. Two parameters are provided:
 *		An instance of `boost::system::error_code` representing
 *		the result of the operation and a \ref connection containing
 *		the `PGconn *` representing the connection (if applicable).
 *
 *	\return
 *		Whatever is appropriate given \em CompletionToken.
 */
template <typename CompletionToken>
auto async_connect (
	connection & conn,
	CompletionToken && token
) {
	beast::async_completion<CompletionToken, detail::async_connect_signature> init(token);
	detail::async_connect_op<
		beast::handler_type<CompletionToken, detail::async_connect_signature>
	> op(
		std::move(init.completion_handler),
		conn
	);
	op.begin();
	return init.result.get();
}

}
