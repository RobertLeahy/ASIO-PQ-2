#include <asio_pq/connection.hpp>

#include <libpq-fe.h>
#include <utility>

namespace asio_pq {

void connection::destroy () noexcept {
	if (!conn_) return;
	PQfinish(conn_);
	conn_ = nullptr;
}

connection::connection () noexcept : conn_(nullptr) {	}

connection::connection (connection && other) noexcept : conn_(other.conn_) {
	other.conn_ = nullptr;
}

connection & connection::operator = (connection && rhs) noexcept {
	destroy();
	using std::swap;
	swap(conn_, rhs.conn_);
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

}
