/**
 *	\file
 */

#pragma once

#include "detail/socket.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <mpark/variant.hpp>
#include <cassert>

namespace asio_pq {

/**
 *	An RAII wrapper for a pointer to a
 *	`PGconn`.
 */
class connection {
private:
	PGconn * conn_;
	boost::asio::io_service * ios_;
	boost::optional<detail::socket_variant_type> socket_;
	void destroy () noexcept;
	void check () const;
public:
	connection () = delete;
	connection (const connection &) = delete;
	connection (connection &&) noexcept;
	connection & operator = (const connection &) = delete;
	connection & operator = (connection &&) noexcept;
	/**
	 *	Creates a new connection which manages a
	 *	particular pointer.
	 *
	 *	\param [in] ios
	 *		The `boost::asio::io_service` which shall be
	 *		used to dispatch asynchronous operations for
	 *		the associated connection.
	 *	\param [in] conn
	 *		The connection handle to manage.
	 */
	connection (boost::asio::io_service & ios, PGconn * conn) noexcept;
	/**
	 *	Creates a new connection which manages a
	 *	libpq connection handle created by calling
	 *	`PGconnectStart`.
	 *
	 *	\param [in] ios
	 *		The `boost::asio::io_service` which shall be
	 *		used to dispatch asynchronous operations for
	 *		the associated connection.
	 *	\param [in] conninfo
	 *		See the libpq manual entry for `PGconnectStart`.
	 */
	explicit connection (boost::asio::io_service & ios, const char * conninfo);
	/**
	 *	Creates a new connection which manages a
	 *	libpq connection handle created by calling
	 *	`PGconnectStartParams`.
	 *
	 *	\param [in] ios
	 *		The `boost::asio::io_service` which shall be
	 *		used to dispatch asynchronous operations for
	 *		the associated connection.
	 *	\param [in] keywords
	 *		See the libpq manual entry for `PQconnectStartParams`.
	 *	\param [in] values
	 *		See the libpq manual entry for `PQconnectStartParams`.
	 *	\param [in] expand_dbname
	 *		See the libpq manual entry for `PQconnectStartParams`.
	 */
	connection (boost::asio::io_service & ios, const char * const * keywords, const char * const * values, bool expand_dbname);
	/**
	 *	Destroys the managed handle (if any).
	 */
	~connection () noexcept;
	/**
	 *	Retrieves the managed handle and surrenders
	 *	ownership thereof (i.e. if the destructor of
	 *	this object is later called the once-managed
	 *	handle will not be destroyed).
	 *
	 *	\return
	 *		The managed handle (if any).
	 */
	PGconn * release () noexcept;
	/**
	 *	Retrieves the managed handle.
	 *
	 *	\return
	 *		The managed handle (if any).
	 */
	PGconn * get () const noexcept;
	/**
	 *	Retrieves the managed handle.
	 *
	 *	\return
	 *		The managed handle (if any).
	 */
	operator PGconn * () const noexcept;
	/**
	 *	Retrieves the `boost::asio::io_service` associated
	 *	with this object.
	 *
	 *	\return
	 *		A reference to a `boost::asio::io_service`.
	 */
	boost::asio::io_service & get_io_service () const noexcept;
	boost::system::error_code duplicate_socket ();
	template <typename Handler>
	void socket (Handler h) {
		assert(socket_);
		mpark::visit(h, *socket_);
	}
	bool has_socket () const noexcept;
	void cancel (boost::system::error_code &) noexcept;
};

}
