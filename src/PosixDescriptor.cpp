/*
 * Descriptor.cpp
 *
 * Copyright (C) 2012 Evidence Srl - www.evidence.eu.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "PosixDescriptor.hpp"

namespace onposix {

/**
 * \brief Function so start an asynchronous operation
 *
 * This method allows to start an asynchronous operation, either read or write.
 * @param read_operation Specifies if it is a read (true) or write (false) operation
 * @param handler Function to be run at the end of the operation. 
 * This function will have as arguments a Buffer* where data is stored and the
 * number of bytes actually transferred.
 * @param buff Buffer where data must be copied to (for a read operation) or where
 * data is contained (for a write operation)
 * @param size Amount of bytes to be transferred
 */
void PosixDescriptor::AsyncThread::startAsyncOperation (bool read_operation, 
    void (*handler) (Buffer* b, size_t size),
	Buffer* buff, size_t size)
{
	size_ = size;
	buff_handler_ = handler;
	buff_buffer_ = buff;
	if (read_operation)
		async_operation_ = READ_BUFFER;
	else
		async_operation_ = WRITE_BUFFER;

	// This will call run() on a different thread:
	start();
}



/**
 * \brief Function so start an asynchronous operation
 *
 * This method allows to start an asynchronous operation, either read or write.
 * @param read_operation Specifies if it is a read (true) or write (false) operation
 * @param handler Function to be run at the end of the operation. 
 * This function will have as arguments a void* where data is stored and the
 * number of bytes actually transferred.
 * @param buff void* where data must be copied to (for a read operation) or where
 * data is contained (for a write operation)
 * @param size Amount of bytes to be transferred
 */
void PosixDescriptor::AsyncThread::startAsyncOperation (bool read_operation,
    void (*handler) (void* b, size_t size),
	void* buff, size_t size)
{
	size_ = size;
	void_handler_ = handler;
	void_buffer_ = buff;
	if (read_operation)
		async_operation_ = READ_VOID;
	else
		async_operation_ = WRITE_VOID;

	// This will call run() on a different thread:
	start();
}

/**
 * \brief Function run on the separate thread.
 *
 * This is the function automatically called by start() which, in turn, is called by
 * startAsyncOperation().
 * The function is run on a different thread. It performs the read/write operation
 * and invokes the handler.
 * @exception It throws runtime_error in case no operation has been scheduled
 */
void PosixDescriptor::AsyncThread::run()
{
	int n;
	
	if (async_operation_ == READ_BUFFER)
		n = des_->__read(buff_buffer_->getBuffer(), size_);
	else if (async_operation_ == READ_VOID)
		n = des_->__read(void_buffer_, size_);
	else if (async_operation_ == WRITE_BUFFER)
		n = des_->__write(buff_buffer_->getBuffer(), size_);
	else if (async_operation_ == WRITE_VOID)
		n = des_->__write(void_buffer_, size_);
	else {
		DEBUG(ERROR, "Handler called without operation!");
		throw std::runtime_error ("Async error");
	}
	des_->lock_.unlock();

	if ((async_operation_ == READ_BUFFER) ||
	    (async_operation_ == WRITE_BUFFER))
		buff_handler_(buff_buffer_, n);
	else
		void_handler_(void_buffer_, n);
	async_operation_ = NONE;
}


/**
 * \brief Low-level read
 *
 * This method is private because it is meant to be used through the other read()
 * methods.
 * Note: it can block the caller, because it continues reading until the given
 * number of bytes have been read.
 * @param buffer Pointer to the buffer where read bytes must be stored
 * @param size Number of bytes to be read
 * @exception runtime_error if the ::read() returns an error
 * @return The number of actually read bytes or -1 in case of error
 */
int PosixDescriptor::__read (void* buffer, size_t size)
{
	size_t remaining = size;
	while (remaining > 0) {
		ssize_t ret = ::read (fd_, ((char*)buffer)+(size-remaining),
		    remaining);
		if (ret == 0)
			// End of file reached
			break;
		else if (ret < 0) {
			throw std::runtime_error ("Read error");
			return -1;
		}
		remaining -= ret;
	}
	return (size-remaining);
}


/**
 * \brief Method to read from the descriptor and fill a buffer.
 *
 * Note: this method may block current thread if data is not available.
 * The buffer is filled with the read data.
 * @param b Pointer to the buffer to be filled
 * @param size Number of bytes that must be read
 * @return -1 in case of error; the number of bytes read otherwise
 */
int PosixDescriptor::read (Buffer* b, size_t size)
{
	if (b->getSize() == 0 || size > b->getSize()) {
		DEBUG(ERROR, "Buffer size not enough!");
		return -1;
	}
	lock_.lock();
	int ret = __read(b->getBuffer(), size);
	lock_.unlock();
	return ret;
}

/**
 * \brief Method to read from the descriptor.
 *
 * Note: this method may block current thread if data is not available.
 * The buffer is filled with the read data.
 * @param p Pointer to the memory space to be filled
 * @param size Number of bytes that must be read
 * @return -1 in case of error; the number of bytes read otherwise
 */
int PosixDescriptor::read (void* p, size_t size)
{
	lock_.lock();
	int ret = __read(p, size);
	lock_.unlock();
	return ret;
}




/**
 * \brief Low-level write
 *
 * This method is private because it is meant to be used through the other
 * write() methods.
 * Note: it can block the caller, because it continues writing until the
 * given number of bytes have been written.
 * @param buffer Pointer to the buffer containing bytes to be written
 * @param size Number of bytes to be written
 * @exception runtime_error if the ::write() returns 0 or an error
 * @return The number of actually written bytes or -1 in case of error
 */
int PosixDescriptor::__write (const void* buffer, size_t size)
{
	size_t remaining = size;
	while (remaining > 0) {
		ssize_t ret = ::write (fd_,
		    ((char*)buffer)+(size-remaining), remaining);
		if (ret == 0)
			// Cannot write more
			break;
		else if (ret < 0) {
			throw std::runtime_error ("Write error");
			return -1;
		}
		remaining -= ret;
	}
	return (size-remaining);
}


/**
 * \brief Method to write data in a buffer to the descriptor.
 *
 * Note: this method may block current thread if data cannot be written.
 * @param b Pointer to the buffer to be filled
 * @param size Number of bytes that must be written
 * @return -1 in case of error; the number of bytes read otherwise
 */
int PosixDescriptor::write (Buffer* b, size_t size)
{
	if (b->getSize() == 0 || size > b->getSize()) {
		DEBUG(ERROR, "Buffer size not enough!");
		return -1;
	}
	return __write(reinterpret_cast<const void*> (b->getBuffer()),
	    size);
}

/**
 * \brief Method to write to the descriptor.
 *
 * Note: this method may block current thread if data cannot be written.
 * @param p Pointer to the memory space containing data
 * @param size Number of bytes that must be written
 * @return -1 in case of error; the number of bytes read otherwise
 */
int PosixDescriptor::write (const void* p, size_t size)
{
	return __write(p, size);
}


/**
 * \brief Method to write a string to the descriptor.
 *
 * Note: this method may block current thread if data cannot be written.
 * @param string to be written
 * @return -1 in case of error; the number of bytes read otherwise
 */
int PosixDescriptor::write (const std::string& s)
{
	return __write(reinterpret_cast<const void*> (s.c_str()), s.size());
}

} /* onposix */
