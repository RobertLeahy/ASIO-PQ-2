/**
 *	\file
 */

#pragma once

#include "wrapper.hpp"
#include <beast/core/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace asio_pq {
namespace detail {

template <typename Handler>
class readable_writable_wrapper : public wrapper<Handler> {
private:
	using base = wrapper<Handler>;
public:
	using base::base;
	void operator () (boost::system::error_code ec, std::size_t num) noexcept(
		noexcept(std::declval<Handler>()(ec))
	) {
		assert(num == 0);
		base::handler()(ec);
	}
};

using async_readable_signature = void (boost::system::error_code);

template <typename AsyncReadStream, typename CompletionToken>
beast::async_return_type<CompletionToken, async_readable_signature> async_readable (
	AsyncReadStream & stream,
	CompletionToken && token
) {
	beast::async_completion<CompletionToken, async_readable_signature> init(token);
	readable_writable_wrapper<
		beast::handler_type<CompletionToken, async_readable_signature>
	> wrapper(std::move(init.completion_handler));
	stream.async_read_some(
		boost::asio::null_buffers{},
		std::move(wrapper)
	);
	return init.result.get();
}

using async_writable_signature = async_readable_signature;

template <typename AsyncWriteStream, typename CompletionToken>
beast::async_return_type<CompletionToken, async_writable_signature> async_writable (
	AsyncWriteStream & stream,
	CompletionToken && token
) {
	beast::async_completion<CompletionToken, async_writable_signature> init(token);
	readable_writable_wrapper<
		beast::handler_type<CompletionToken, async_writable_signature>
	> wrapper(std::move(init.completion_handler));
	stream.async_write_some(
		boost::asio::null_buffers{},
		std::move(wrapper)
	);
	return init.result.get();
}

}
}
