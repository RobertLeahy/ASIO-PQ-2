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
class handle {
private:
	PGconn * conn_;
	void destroy () noexcept;
public:
	/**
	 *	Creates a new handle which does no manage
	 *	a pointer.
	 */
	handle () noexcept;
	handle (const handle &) = delete;
	handle (handle &&) noexcept;
	handle & operator = (const handle &) = delete;
	handle & operator = (handle &&) noexcept;
	/**
	 *	Creates a new handle which manages a
	 *	particular pointer.
	 *
	 *	\param [in] conn
	 *		The connection handle to manage.
	 */
	explicit handle (PGconn * conn) noexcept;
	/**
	 *	Destroys the managed handle (if any).
	 */
	~handle () noexcept;
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
