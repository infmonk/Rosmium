#ifndef OSMIUM_INDEX_DETAIL_CREATE_MAP_WITH_FD_HPP
#define OSMIUM_INDEX_DETAIL_CREATE_MAP_WITH_FD_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2015 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace osmium {

    namespace index {

        namespace detail {

            template <typename T>
            inline T* create_map_with_fd(const std::vector<std::string>& config) {
                if (config.size() == 1) {
                    return new T();
                }
                assert(config.size() > 1);
                const std::string& filename = config[1];
                int fd = ::open(filename.c_str(), O_CREAT | O_RDWR, 0644);
                if (fd == -1) {
                    throw std::runtime_error(std::string("can't open file '") + filename + "': " + strerror(errno));
                }
                return new T(fd);
            }

        } // namespace detail

    } // namespace index

} // namespace osmium

#endif // OSMIUM_INDEX_DETAIL_CREATE_MAP_WITH_FD_HPP