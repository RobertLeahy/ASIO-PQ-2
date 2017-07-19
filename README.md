# ASIO PQ

[![Build Status](https://travis-ci.org/RobertLeahy/ASIO-PQ-2.svg?branch=master)](https://travis-ci.org/RobertLeahy/ASIO-PQ-2) [![Build status](https://ci.appveyor.com/api/projects/status/trp2gsq13uk3abww/branch/master?svg=true)](https://ci.appveyor.com/project/RobertLeahy/asio-pq-2/branch/master)

## What is ASIO PQ?

ASIO PQ is a C++14 library which provides composed asynchronous operations against a PostgreSQL server over either TCP/IP or Unix domain sockets.

## How does it work?

1. Create an `asio_pq::connection` object (this is an RAII wrapper around a `PGconn *` which also binds in a `boost::asio::io_service` and provides the `get_io_service()` method
2. Dispatch a connection attempt with `asio_pq::async_connect`
3. On successful completion call `PQsetnonblocking` to enable non-blocking mode
4. Begin a command as usual for [asynchronous command processing](https://www.postgresql.org/docs/current/static/libpq-async.html)
5. Call `asio_pq::async_get_result` to asynchronously obtain an `asio_pq::result` object (this is an RAII wrapper around a `PGresult *`)

### Types

- `connection`
- `result`

### Operations

- `async_connect`
- `async_get_result`
- `cancel`

## Dependencies

- Boost 1.58.0+

Other libraries are depended on, but are header only and are downloaded by CMake:

- Beast
- Catch
- MPark Variant

## Requirements

- Clang 4+ or GCC 6.2.0+ or Microsoft Visual C++ 2017+
- CMake 3.5+
- libpq

## Example

See unit tests.
