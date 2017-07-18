#include <asio_pq/connection.hpp>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <cassert>
#include <new>
#include <utility>

namespace asio_pq {

void connection::destroy () noexcept {
	if (!conn_) return;
	PQfinish(conn_);
	conn_ = nullptr;
	ios_ = nullptr;
	socket_ = boost::none;
}

void connection::check () const {
	if (!conn_) throw std::bad_alloc{};
}

connection::connection (connection && other) noexcept
	:	conn_(other.conn_),
		ios_(other.ios_),
		socket_(std::move(other.socket_))
{
	other.conn_ = nullptr;
	other.ios_ = nullptr;
}

connection & connection::operator = (connection && rhs) noexcept {
	destroy();
	using std::swap;
	swap(conn_, rhs.conn_);
	swap(ios_, rhs.ios_);
	swap(socket_, rhs.socket_);
	return *this;
}

connection::connection (boost::asio::io_service & ios, PGconn * conn) noexcept
	:	conn_(conn),
		ios_(&ios)
{	}

connection::connection (boost::asio::io_service & ios, const char * conninfo)
	:	conn_(PQconnectStart(conninfo)),
		ios_(&ios)
{
	check();
}

connection::connection (boost::asio::io_service & ios, const char * const * keywords, const char * const * values, bool expand_dbname)
	:	conn_(PQconnectStartParams(keywords, values, int(expand_dbname))),
		ios_(&ios)
{
	check();
}

connection::~connection () noexcept {
	destroy();
}

PGconn * connection::release () noexcept {
	PGconn * retr = nullptr;
	using std::swap;
	swap(retr, conn_);
	ios_ = nullptr;
	socket_ = boost::none;
	return retr;
}

PGconn * connection::get() const noexcept {
	return conn_;
}

connection::operator PGconn * () const noexcept {
	return get();
}

boost::asio::io_service & connection::get_io_service () const noexcept {
	assert(ios_);
	return *ios_;
}

boost::system::error_code connection::duplicate_socket () {
	boost::system::error_code ec;
	if (socket_) return ec;
	auto v = detail::socket(get_io_service(), get(), ec);
	if (!ec) socket_ = std::move(v);
	return ec;
}

bool connection::has_socket () const noexcept {
	return bool(socket_);
}

void connection::cancel (boost::system::error_code & ec) noexcept {
	ec.clear();
	socket([&] (auto & socket) noexcept {	socket.cancel(ec);	});
	if (!ec) socket_ = boost::none;
}

}
