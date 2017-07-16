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

using async_connect_signature = void (boost::system::error_code, connection);

template <typename Handler>
class async_connect_fail_wrapper : public wrapper<Handler> {
private:
	using base = wrapper<Handler>;
	boost::system::error_code ec_;
public:
	async_connect_fail_wrapper (Handler h, boost::system::error_code ec)
		:	base(std::move(h)),
			ec_(ec)
	{	}
	void operator () () {
		base::handler()(ec_, connection{});
	}
};

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
		asio_pq::connection handle;
		boost::asio::io_service & io_service;
		boost::optional<boost::system::error_code> result;
		bool read;
		bool write;
		state (const Handler &, asio_pq::connection handle, boost::asio::io_service & ios)
			:	handle(std::move(handle)),
				io_service(ios),
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
		boost::asio::io_service & ios = ptr_->io_service;
		ios.post(std::move(*this));
	}
	void complete () {
		assert(ptr_->result);
		assert(!ptr_->read);
		assert(!ptr_->write);
		boost::system::error_code ec = *ptr_->result;
		asio_pq::connection h(std::move(ptr_->handle));
		ptr_.invoke(ec, std::move(h));
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
	async_connect_op (Handler h, asio_pq::connection handle, boost::asio::io_service & ios)
		:	ptr_(std::move(h), std::move(handle), ios)
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
		boost::asio::io_service & ios = ptr_->io_service;
		auto ec = ptr_->handle.duplicate_socket(ios);
		if (ec) {
			ptr_->result = ec;
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

template <typename CompletionHandler>
void async_connect_fail (
	boost::asio::io_service & ios,
	boost::system::error_code ec,
	CompletionHandler h
) {
	ios.post(
		async_connect_fail_wrapper<CompletionHandler>(
			std::move(h),
			ec
		)
	);
}

template <typename CompletionToken>
auto async_connect (
	boost::asio::io_service & ios,
	connection h,
	CompletionToken && token
) {
	beast::async_completion<CompletionToken, async_connect_signature> init(token);
	if (h) {
		async_connect_op<
			beast::handler_type<CompletionToken, async_connect_signature>
		> op(
			std::move(init.completion_handler),
			std::move(h),
			ios
		);
		op.begin();
	} else {
		detail::async_connect_fail(
			ios,
			make_error_code(boost::system::errc::not_enough_memory),
			std::move(init.completion_handler)
		);
	}
	return init.result.get();
}

}

/**
 *	Asynchronously connects to a PostgreSQL server.
 *
 *	\tparam CompletionToken
 *		The type of completion token an instance
 *		of which shall be used to notify the caller
 *		of completion.
 *
 *	\param [in] ios
 *		A `boost::asio::io_service` which shall be used
 *		to dispatch asynchronous operations.
 *	\param [in] conninfo
 *		See the libpq manual entry for `PQconnectStart`.
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
	boost::asio::io_service & ios,
	const char * conninfo,
	CompletionToken && token
) {
	connection h(PQconnectStart(conninfo));
	return detail::async_connect(
		ios,
		std::move(h),
		std::forward<CompletionToken>(token)
	);
}
/**
 *	Asynchronously connects to a PostgreSQL server.
 *
 *	\tparam CompletionToken
 *		The type of completion token an instance
 *		of which shall be used to notify the caller
 *		of completion.
 *
 *	\param [in] ios
 *		A `boost::asio::io_service` which shall be used
 *		to dispatch asynchronous operations.
 *	\param [in] keywords
 *		See the libpq manual entry for `PQconnectStartParams`.
 *	\param [in] values
 *		See the libpq manual entry for `PQconnectStartParams`.
 *	\param [in] expand_dbname
 *		See the libpq manual entry for `PQconnectStartParams`.
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
	boost::asio::io_service & ios,
	const char * const * keywords,
	const char * const * values,
	bool expand_dbname,
	CompletionToken && token
) {
	int edbn(expand_dbname);
	connection h(PQconnectStartParams(keywords, values, expand_dbname));
	return detail::async_connect(
		ios,
		std::move(h),
		std::forward<CompletionToken>(token)
	);
}

}
