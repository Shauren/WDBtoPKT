/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ByteBuffer.h"
#include <algorithm>
#include <format>
#include <cmath>

ByteBufferPositionException::ByteBufferPositionException(size_t pos, size_t size, size_t valueSize)
    : ByteBufferException(std::format("Attempted to get value with size: {} in ByteBuffer (pos: {} size: {})", valueSize, pos, size))
{
}

ByteBufferInvalidValueException::ByteBufferInvalidValueException(char const* type, std::string_view value)
    : ByteBufferException(std::format("Invalid {} value ({}) found in ByteBuffer", type, value))
{
}

ByteBuffer& ByteBuffer::operator>>(float& value)
{
    read(&value, 1);
    if (!std::isfinite(value))
        throw ByteBufferInvalidValueException("float", "infinity");
    return *this;
}

ByteBuffer& ByteBuffer::operator>>(double& value)
{
    read(&value, 1);
    if (!std::isfinite(value))
        throw ByteBufferInvalidValueException("double", "infinity");
    return *this;
}

std::string_view ByteBuffer::ReadCString()
{
    if (_rpos >= size())
        throw ByteBufferPositionException(_rpos, 1, size());

    ResetBitPos();

    char const* begin = reinterpret_cast<char const*>(_storage.data()) + _rpos;
    char const* end = reinterpret_cast<char const*>(_storage.data()) + size();
    char const* stringEnd = std::ranges::find(begin, end, '\0');
    if (stringEnd == end)
        throw ByteBufferPositionException(size(), 1, size());

    std::string_view value(begin, stringEnd);
    _rpos += value.length() + 1;
    return value;
}

std::string_view ByteBuffer::ReadString(std::uint32_t length)
{
    if (_rpos + length > size())
        throw ByteBufferPositionException(_rpos, length, size());

    ResetBitPos();
    if (!length)
        return {};

    std::string_view value(reinterpret_cast<char const*>(&_storage[_rpos]), length);
    _rpos += length;
    return value;
}

void ByteBuffer::append(std::uint8_t const* src, size_t cnt)
{
    FlushBits();

    size_t const newSize = _wpos + cnt;
    if (_storage.capacity() < newSize) // custom memory allocation rules
    {
        if (newSize < 100)
            _storage.reserve(300);
        else if (newSize < 750)
            _storage.reserve(2500);
        else if (newSize < 6000)
            _storage.reserve(10000);
        else
            _storage.reserve(400000);
    }

    if (_storage.size() < newSize)
        _storage.resize(newSize);
    std::memcpy(&_storage[_wpos], src, cnt);
    _wpos = newSize;
}

void ByteBuffer::put(size_t pos, std::uint8_t const* src, size_t cnt)
{
    std::memcpy(&_storage[pos], src, cnt);
}

void ByteBuffer::PutBits(std::size_t pos, std::size_t value, std::uint32_t bitCount)
{
    for (std::uint32_t i = 0; i < bitCount; ++i)
    {
        std::size_t wp = (pos + i) / 8;
        std::size_t bit = (pos + i) % 8;
        if ((value >> (bitCount - i - 1)) & 1)
            _storage[wp] |= 1 << (7 - bit);
        else
            _storage[wp] &= ~(1 << (7 - bit));
    }
}

void ByteBuffer::OnInvalidPosition(size_t pos, size_t valueSize) const
{
    throw ByteBufferPositionException(pos, _storage.size(), valueSize);
}

template char ByteBuffer::read<char>();
template std::uint8_t ByteBuffer::read<std::uint8_t>();
template std::uint16_t ByteBuffer::read<std::uint16_t>();
template std::uint32_t ByteBuffer::read<std::uint32_t>();
template std::uint64_t ByteBuffer::read<std::uint64_t>();
template std::int8_t ByteBuffer::read<int8_t>();
template std::int16_t ByteBuffer::read<std::int16_t>();
template std::int32_t ByteBuffer::read<std::int32_t>();
template std::int64_t ByteBuffer::read<std::int64_t>();
template float ByteBuffer::read<float>();
template double ByteBuffer::read<double>();
