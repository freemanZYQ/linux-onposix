/*
 * StreamSocketClientDescriptor.hpp
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

#ifndef STREAMSOCKETCLIENTDESCRIPTOR_HPP_
#define STREAMSOCKETCLIENTDESCRIPTOR_HPP_

#include "PosixDescriptor.hpp"

namespace onposix {

/**
 * \brief Class for a connection-oriented client.
 */
class StreamSocketClientDescriptor: public PosixDescriptor {
public:
	virtual ~StreamSocketClientDescriptor(){}
	StreamSocketClientDescriptor(const std::string& name);
	StreamSocketClientDescriptor(const std::string& address, const uint16_t port);
};

} /* onposix */

#endif /* STREAMSOCKETCLIENTDESCRIPTOR_HPP_ */
