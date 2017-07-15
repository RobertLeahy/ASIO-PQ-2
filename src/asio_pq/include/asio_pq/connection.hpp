/**
 *	\file
 */

#pragma once

#include <libpq-fe.h>

namespace asio_pq {

/**
 *	An RAII wrapper for a pointer to a
 *	`PGconn`.
 */
class connection {
private:
	PGconn * conn_;
	void destroy () noexcept;
public:
	/**
	 *	Creates a new connection which does no manage
	 *	a pointer.
	 */
	connection () noexcept;
	connection (const connection &) = delete;
	connection (connection &&) noexcept;
	connection & operator = (const connection &) = delete;
	connection & operator = (connection &&) noexcept;
	/**
	 *	Creates a new connection which manages a
	 *	particular pointer.
	 *
	 *	\param [in] conn
	 *		The connection handle to manage.
	 */
	explicit connection (PGconn * conn) noexcept;
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
	 *	Determines whether or not this object manages
	 *	a handle.
	 *
	 *	\return
	 *		\em true if this object manages a
	 *		handle, \em false otherwise.
	 */
	explicit operator bool () const noexcept;
};

}
