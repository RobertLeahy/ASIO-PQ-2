/**
 *	\file
 */

#pragma once

#include "connection.hpp"
#include "detail/op.hpp"
#include "detail/wrapper.hpp"
#include "error.hpp"
#include "result.hpp"
#include <beast/core/async_result.hpp>
#include <beast/core/handler_ptr.hpp>
#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <libpq-fe.h>
#include <cassert>
#include <memory>
#include <utility>

namespace asio_pq {

namespace detail {

using async_get_result_signature = void (boost::system::error_code, result);

template <typename Handler>
class async_get_result_fail_wrapper : public wrapper<Handler> {
private:
	using base = wrapper<Handler>;
	boost::system::error_code ec_;
public:
	async_get_result_fail_wrapper (Handler h, boost::system::error_code ec)
		:	base(std::move(h)),
			ec_(ec)
	{	}
	void operator () () {
		base::handler()(ec_, result{});
	}
};

template <typename Handler>
class async_get_result_success_wrapper {
private:
	class state : public result {
	public:
		state (const Handler &, result r)
			:	result(std::move(r))
		{	}
	};
	using pointer = beast::handler_ptr<state, Handler>;
	pointer ptr_;
public:
	async_get_result_success_wrapper () = delete;
	async_get_result_success_wrapper (const async_get_result_success_wrapper &) = default;
	async_get_result_success_wrapper (async_get_result_success_wrapper &&) = default;
	async_get_result_success_wrapper & operator = (const async_get_result_success_wrapper &) = default;
	async_get_result_success_wrapper & operator = (async_get_result_success_wrapper &&) = default;
	template <typename DeducedHandler>
	async_get_result_success_wrapper (DeducedHandler && h, result r)
		:	ptr_(std::forward<DeducedHandler>(h), std::move(r))
	{	}
	void operator () () {
		result r(std::move(*ptr_));
		ptr_.invoke(boost::system::error_code{}, std::move(r));
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_get_result_success_wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->ptr_.handler()));
	}
	friend bool asio_handler_is_continuation (async_get_result_success_wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->ptr_.handler()));
	}
	friend void * asio_handler_allocate (std::size_t num, async_get_result_success_wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->ptr_.handler()));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, async_get_result_success_wrapper * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->ptr_.handler()));
	}
};

template <typename Handler>
class async_get_result_op {
private:
	class state {
	public:
		state () = delete;
		state (const state &) = delete;
		state (state &&) = delete;
		state & operator = (const state &) = delete;
		state & operator = (state &&) = delete;
		explicit state (const Handler &, asio_pq::connection & conn, int flush)
			:	strand(conn.get_io_service()),
				connection(conn),
				read(false),
				write(false),
				flush(flush)
		{	}
		boost::asio::io_service::strand strand;
		asio_pq::connection & connection;
		bool read;
		bool write;
		int flush;
		boost::optional<boost::system::error_code> error_code;
		asio_pq::result result;
	};
	using pointer = beast::handler_ptr<state, Handler>;
	pointer ptr_;
	void complete () {
		assert(!ptr_->read);
		assert(!ptr_->write);
		assert(ptr_->error_code);
		auto ec = *ptr_->error_code;
		auto result = std::move(ptr_->result);
		ptr_.invoke(ec, std::move(result));
	}
	bool complete_if () {
		if (!ptr_->error_code) return false;
		complete();
		return true;
	}
	void upcall () {
		if (ptr_->read || ptr_->write) {
			ptr_->connection.socket([&] (auto & socket) {	socket.cancel();	});
			return;
		}
		complete();
	}
	void fail (boost::system::error_code ec) {
		if (!ptr_->error_code) ptr_->error_code = ec;
		upcall();
	}
	void success (result r) {
		ptr_->error_code = boost::in_place();
		ptr_->result = std::move(r);
		upcall();
	}
	void read () {
		if (ptr_->read) return;
		ptr_->read = true;
		ptr_->connection.socket([&] (auto & socket) {
			state & s = *ptr_;
			detail::async_readable(
				socket,
				s.strand.wrap(
					detail::make_read_wrapper(
						std::move(*this)
					)
				)
			);
		});
	}
	void write () {
		if (ptr_->write) return;
		ptr_->write = true;
		ptr_->connection.socket([&] (auto & socket) {
			state & s = *ptr_;
			detail::async_writable(
				socket,
				s.strand.wrap(
					detail::make_write_wrapper(
						std::move(*this)
					)
				)
			);
		});
	}
	void dispatch () {
		if (ptr_->flush == 1) write();
		read();
	}
	bool flush () {
		if (ptr_->flush == 0) return true;
		ptr_->flush = PQflush(ptr_->connection);
		if (ptr_->flush == -1) {
			fail(make_error_code(error::flush_failed));
			return false;
		}
		return true;
	}
	bool consume () {
		if (PQconsumeInput(ptr_->connection) == 0) {
			fail(make_error_code(error::consume_failed));
			return false;
		}
		if (PQisBusy(ptr_->connection) == 0) {
			result r(PQgetResult(ptr_->connection));
			success(std::move(r));
			return false;
		}
		return true;
	}
public:
	async_get_result_op () = delete;
	async_get_result_op (const async_get_result_op &) = default;
	async_get_result_op (async_get_result_op &&) = default;
	async_get_result_op & operator = (const async_get_result_op &) = default;
	async_get_result_op & operator = (async_get_result_op &&) = default;
	template <typename DeducedHandler>
	async_get_result_op (connection & conn, int flush, DeducedHandler && h)
		:	ptr_(std::forward<DeducedHandler>(h), conn, flush)
	{	}
	void begin () {
		assert(ptr_->connection);
		assert(!ptr_->read);
		assert(!ptr_->write);
		assert(ptr_->flush != -1);
		assert(!ptr_->error_code);
		assert(!ptr_->result);
		dispatch();
	}
	void read (boost::system::error_code ec) {
		ptr_->read = false;
		if (complete_if()) return;
		//	If it becomes read-ready, call PQconsumeInput,
		//	then call PQflush again.
		if (consume() && flush()) dispatch();
	}
	void write (boost::system::error_code ec) {
		ptr_->write = false;
		if (complete_if()) return;
		//	If it becomes write-ready, call PQflush again.
		if (flush()) dispatch();
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_get_result_op * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->ptr_.handler()));
	}
	friend bool asio_handler_is_continuation (async_get_result_op * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->ptr_.handler()));
	}
	friend void * asio_handler_allocate (std::size_t num, async_get_result_op * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->ptr_.handler()));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, async_get_result_op * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->ptr_.handler()));
	}
};

template <typename Handler>
void async_get_result_fail (
	boost::asio::io_service & ios,
	boost::system::error_code ec,
	Handler h
) {
	ios.post(
		async_get_result_fail_wrapper<Handler>(
			std::move(h),
			ec
		)
	);
}

template <typename Handler>
void async_get_result_success (
	boost::asio::io_service & ios,
	result r,
	Handler h
) {
	ios.post(
		async_get_result_success_wrapper<Handler>(
			std::move(h),
			std::move(r)
		)
	);
}

}

/**
 *	Asynchronously calls `PQflush` and `PQconsumeInput` until
 *	`PQisBusy` indicates `PQgetResult` will return a `PGresult *`
 *	without blocking at which time a \ref result wrapping the
 *	`PGresult *` will be asynchronously passed to the completion
 *	handler.
 *
 *	\tparam CompletionToken
 *		The type of completion token an instance
 *		of which shall be used to notify the caller
 *		of completion.
 *
 *	\param [in] ios
 *		A `boost::asio::io_service` which shall be used
 *		to dispatch asynchronous operations.
 *	\param [in] conn
 *		The \ref connection which has a pending command.
 *		It must be the case that `!!conn` is \em true or
 *		the behavior is undefined.
 *	\param [in] token
 *		The completion token which shall be used to notify
 *		the caller of completion. Two parameters are provided:
 *		An instance of `boost::system::error_code` representing
 *		the result of the operation and a \ref result containing
 *		the `PGresult *` representing the result (if applicable).
 *
 *	\return
 *		Whatever is appropriate given \em CompletionToken.
 */
template <typename CompletionToken>
auto async_get_result (
	boost::asio::io_service & ios,
	connection & conn,
	CompletionToken && token
) {
	assert(conn);
	beast::async_completion<CompletionToken, detail::async_get_result_signature> init(token);
	auto ec = conn.duplicate_socket(ios);
	if (ec) {
		detail::async_get_result_fail(ios, ec, std::move(init.completion_handler));
		return init.result.get();
	}
	int flush = PQflush(conn);
	switch (flush) {
	case -1:
		detail::async_get_result_fail(
			ios,
			make_error_code(error::flush_failed),
			std::move(init.completion_handler)
		);
		return init.result.get();
	case 1:
	default:
		break;
	case 0:
		if (PQconsumeInput(conn) == 0) {
			detail::async_get_result_fail(
				ios,
				make_error_code(error::consume_failed),
				std::move(init.completion_handler)
			);
			return init.result.get();
		}
		if (PQisBusy(conn) == 0) {
			result r(PQgetResult(conn));
			detail::async_get_result_success(
				ios,
				std::move(r),
				std::move(init.completion_handler)
			);
			return init.result.get();
		}
		break;
	}
	detail::async_get_result_op<
		beast::handler_type<CompletionToken, detail::async_get_result_signature>
	> op(
		conn,
		flush,
		std::move(init.completion_handler)
	);
	op.begin();
	return init.result.get();
}

}
