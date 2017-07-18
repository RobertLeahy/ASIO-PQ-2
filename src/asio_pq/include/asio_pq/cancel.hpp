/**
 *	\file
 */

#pragma once

#include "connection.hpp"
#include <boost/system/error_code.hpp>

namespace asio_pq {

/**
 *	Cancels any pending operations on a
 *	\ref connection.
 *
 *	Note that this is not a thread safe
 *	operation: Concurrent access to the
 *	\ref connection is not allowed, even
 *	by intermediate completion handlers
 *	which are part of a pending operation
 *	(i.e. even completion handlers which
 *	are implementation details and invisible
 *	to the consumer of this library may not
 *	be running). For more information on how
 *	to resolve this see the documentation of
 *	`boost::asio::io_service::strand` and
 *	`boost::asio::asio_handler_invoke`.
 *
 *	Note that due to inherent race conditions
 *	it is possible this function will complete
 *	successfully and the asynchronous operation
 *	will not be cancelled: If the operation had
 *	already completed and its completion handler
 *	has been enqueued for invocation then it will
 *	not be cancelled.
 *
 *	If a cancellation actually occurs the
 *	underlying connection to the PostgreSQL server
 *	will be in an inconsistent state and should
 *	not be used further.
 *
 *	In the case that cancellation actually occurs
 *	completion handlers will be invoked with
 *	`boost::asio::error::operation_aborted`.
 *
 *	\param [in] conn
 *		The \ref connection.
 *	\param [out] ec
 *		A `boost::system::error_code` object which
 *		shall be set to the result of the operation.
 *		Note that if this object already represents
 *		an error it will be cleared.
 */
void cancel (connection & conn, boost::system::error_code & ec) noexcept;
/**
 *	Cancels any pending operations on a
 *	\ref connection.
 *
 *	Note that this is not a thread safe
 *	operation: Concurrent access to the
 *	\ref connection is not allowed, even
 *	by intermediate completion handlers
 *	which are part of a pending operation
 *	(i.e. even completion handlers which
 *	are implementation details and invisible
 *	to the consumer of this library may not
 *	be running). For more information on how
 *	to resolve this see the documentation of
 *	`boost::asio::io_service::strand` and
 *	`boost::asio::asio_handler_invoke`.
 *
 *	Note that due to inherent race conditions
 *	it is possible this function will complete
 *	successfully and the asynchronous operation
 *	will not be cancelled: If the operation had
 *	already completed and its completion handler
 *	has been enqueued for invocation then it will
 *	not be cancelled.
 *
 *	If a cancellation actually occurs the
 *	underlying connection to the PostgreSQL server
 *	will be in an inconsistent state and should
 *	not be used further.
 *
 *	In the case that cancellation actually occurs
 *	completion handlers will be invoked with
 *	`boost::asio::error::operation_aborted`.
 *
 *	\param [in] conn
 *		The \ref connection.
 */
void cancel (connection & conn);

}
