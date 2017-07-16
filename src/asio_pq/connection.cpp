#include <asio_pq/connection.hpp>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <cassert>
#include <utility>

namespace asio_pq {

void connection::destroy () noexcept {
	if (!conn_) return;
	PQfinish(conn_);
	conn_ = nullptr;
	socket_ = boost::none;
}

connection::connection () noexcept : conn_(nullptr) {	}

connection::connection (connection && other) noexcept
	:	conn_(other.conn_),
		socket_(std::move(other.socket_))
{
	other.conn_ = nullptr;
}

connection & connection::operator = (connection && rhs) noexcept {
	destroy();
	using std::swap;
	swap(conn_, rhs.conn_);
	swap(socket_, rhs.socket_);
	return *this;
}

connection::connection (PGconn * conn) noexcept : conn_(conn) {	}

connection::~connection () noexcept {
	destroy();
}

PGconn * connection::release () noexcept {
	PGconn * retr = nullptr;
	using std::swap;
	swap(retr, conn_);
	return retr;
}

PGconn * connection::get() const noexcept {
	return conn_;
}

connection::operator PGconn * () const noexcept {
	return get();
}

connection::operator bool () const noexcept {
	return bool(get());
}

boost::system::error_code connection::duplicate_socket (boost::asio::io_service & ios) {
	boost::system::error_code ec;
	if (socket_) return ec;
	auto v = detail::socket(ios, get(), ec);
	if (!ec) socket_ = std::move(v);
	return ec;
}

bool connection::has_socket () const noexcept {
	return bool(socket_);
}

boost::asio::io_service & connection::get_io_service () noexcept {
	assert(socket_);
	return mpark::visit([&] (auto & socket) noexcept -> boost::asio::io_service & {	return socket.get_io_service();	}, *socket_);
}

}
