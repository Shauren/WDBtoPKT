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

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include <array>
#include <string>
#include <vector>
#include <cstring>

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    ~ByteBufferException() noexcept = default;

    char const* what() const noexcept override { return msg_.c_str(); }

protected:
    std::string & message() noexcept { return msg_; }

private:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(std::size_t pos, std::size_t size, std::size_t valueSize);

    ~ByteBufferPositionException() noexcept = default;
};

class ByteBufferInvalidValueException : public ByteBufferException
{
public:
    ByteBufferInvalidValueException(char const* type, char const* value);

    ~ByteBufferInvalidValueException() noexcept = default;
};

class ByteBuffer
{
    public:
        constexpr static std::size_t DEFAULT_SIZE = 0x1000;
        constexpr static std::uint8_t InitialBitPos = 8;

        // constructor
        ByteBuffer() : _rpos(0), _wpos(0), _bitpos(InitialBitPos), _curbitval(0)
        {
            _storage.reserve(DEFAULT_SIZE);
        }

        // reserve/resize tag
        struct Reserve { };
        struct Resize { };

        ByteBuffer(size_t size, Reserve) : _rpos(0), _wpos(0), _bitpos(InitialBitPos), _curbitval(0)
        {
            _storage.reserve(size);
        }

        ByteBuffer(size_t size, Resize) : _rpos(0), _wpos(size), _bitpos(InitialBitPos), _curbitval(0)
        {
            _storage.resize(size);
        }

        ByteBuffer(ByteBuffer&& buf) noexcept : _rpos(buf._rpos), _wpos(buf._wpos),
            _bitpos(buf._bitpos), _curbitval(buf._curbitval), _storage(buf.Move()) { }

        ByteBuffer(ByteBuffer const& right) = default;

        std::vector<std::uint8_t>&& Move() noexcept
        {
            _rpos = 0;
            _wpos = 0;
            _bitpos = InitialBitPos;
            _curbitval = 0;
            return std::move(_storage);
        }

        ByteBuffer& operator=(ByteBuffer const& right)
        {
            if (this != &right)
            {
                _rpos = right._rpos;
                _wpos = right._wpos;
                _bitpos = right._bitpos;
                _curbitval = right._curbitval;
                _storage = right._storage;
            }

            return *this;
        }

        ByteBuffer& operator=(ByteBuffer&& right) noexcept
        {
            if (this != &right)
            {
                _rpos = right._rpos;
                _wpos = right._wpos;
                _bitpos = right._bitpos;
                _curbitval = right._curbitval;
                _storage = right.Move();
            }

            return *this;
        }

        virtual ~ByteBuffer() = default;

        void clear()
        {
            _rpos = 0;
            _wpos = 0;
            _bitpos = InitialBitPos;
            _curbitval = 0;
            _storage.clear();
        }

        template <typename T>
        void append(T value)
        {
            static_assert(std::is_trivially_copyable<T>::value, "append(T) must be used with trivially copyable types");
            append((std::uint8_t *)&value, sizeof(value));
        }

        bool HasUnfinishedBitPack() const
        {
            return _bitpos != 8;
        }

        void FlushBits()
        {
            if (_bitpos == 8)
                return;

            _bitpos = 8;

            append((std::uint8_t *)&_curbitval, sizeof(std::uint8_t));
            _curbitval = 0;
        }

        void ResetBitPos()
        {
            if (_bitpos > 7)
                return;

            _bitpos = 8;
            _curbitval = 0;
        }

        bool WriteBit(bool bit)
        {
            --_bitpos;
            if (bit)
                _curbitval |= (1 << (_bitpos));

            if (_bitpos == 0)
            {
                _bitpos = 8;
                append((std::uint8_t *)&_curbitval, sizeof(_curbitval));
                _curbitval = 0;
            }

            return bit;
        }

        bool ReadBit()
        {
            ++_bitpos;
            if (_bitpos > 7)
            {
                _curbitval = read<std::uint8_t>();
                _bitpos = 0;
            }

            return ((_curbitval >> (7-_bitpos)) & 1) != 0;
        }

        void WriteBits(std::size_t value, std::int32_t bits)
        {
            for (std::int32_t i = bits - 1; i >= 0; --i)
                WriteBit((value >> i) & 1);
        }

        std::uint32_t ReadBits(std::int32_t bits)
        {
            std::uint32_t value = 0;
            for (std::int32_t i = bits - 1; i >= 0; --i)
                if (ReadBit())
                    value |= (1 << (i));

            return value;
        }

        template <typename T>
        void put(std::size_t pos, T value)
        {
            static_assert(std::is_trivially_copyable<T>::value, "put(size_t, T) must be used with trivially copyable types");
            put(pos, (std::uint8_t *)&value, sizeof(value));
        }

        /**
          * @name   PutBits
          * @brief  Places specified amount of bits of value at specified position in packet.
          *         To ensure all bits are correctly written, only call this method after
          *         bit flush has been performed

          * @param  pos Position to place the value at, in bits. The entire value must fit in the packet
          *             It is advised to obtain the position using bitwpos() function.

          * @param  value Data to write.
          * @param  bitCount Number of bits to store the value on.
        */
        void PutBits(std::size_t pos, std::size_t value, std::uint32_t bitCount);

        ByteBuffer &operator<<(std::uint8_t value)
        {
            append<std::uint8_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::uint16_t value)
        {
            append<std::uint16_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::uint32_t value)
        {
            append<std::uint32_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::uint64_t value)
        {
            append<std::uint64_t>(value);
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer &operator<<(std::int8_t value)
        {
            append<std::int8_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::int16_t value)
        {
            append<std::int16_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::int32_t value)
        {
            append<std::int32_t>(value);
            return *this;
        }

        ByteBuffer &operator<<(std::int64_t value)
        {
            append<std::int64_t>(value);
            return *this;
        }

        // floating points
        ByteBuffer &operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        ByteBuffer &operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        ByteBuffer &operator<<(const std::string &value)
        {
            if (size_t len = value.length())
                append((std::uint8_t const*)value.c_str(), len);
            append<std::uint8_t>(0);
            return *this;
        }

        ByteBuffer &operator<<(const char *str)
        {
            if (size_t len = (str ? strlen(str) : 0))
                append((std::uint8_t const*)str, len);
            append<std::uint8_t>(0);
            return *this;
        }

        ByteBuffer &operator>>(bool &value)
        {
            value = read<char>() > 0 ? true : false;
            return *this;
        }

        ByteBuffer &operator>>(std::uint8_t &value)
        {
            value = read<std::uint8_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::uint16_t &value)
        {
            value = read<std::uint16_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::uint32_t &value)
        {
            value = read<std::uint32_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::uint64_t &value)
        {
            value = read<std::uint64_t>();
            return *this;
        }

        //signed as in 2e complement
        ByteBuffer &operator>>(std::int8_t &value)
        {
            value = read<std::int8_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::int16_t &value)
        {
            value = read<std::int16_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::int32_t &value)
        {
            value = read<std::int32_t>();
            return *this;
        }

        ByteBuffer &operator>>(std::int64_t &value)
        {
            value = read<std::int64_t>();
            return *this;
        }

        ByteBuffer &operator>>(float &value);
        ByteBuffer &operator>>(double &value);

        ByteBuffer& operator>>(std::string& value)
        {
            value = ReadCString(true);
            return *this;
        }

        std::uint8_t& operator[](size_t const pos)
        {
            if (pos >= size())
                throw ByteBufferPositionException(pos, 1, size());
            return _storage[pos];
        }

        std::uint8_t const& operator[](size_t const pos) const
        {
            if (pos >= size())
                throw ByteBufferPositionException(pos, 1, size());
            return _storage[pos];
        }

        size_t rpos() const { return _rpos; }

        size_t rpos(size_t rpos_)
        {
            _rpos = rpos_;
            return _rpos;
        }

        void rfinish()
        {
            _rpos = wpos();
        }

        size_t wpos() const { return _wpos; }

        size_t wpos(size_t wpos_)
        {
            _wpos = wpos_;
            return _wpos;
        }

        /// Returns position of last written bit
        size_t bitwpos() const { return _wpos * 8 + 8 - _bitpos; }

        size_t bitwpos(size_t newPos)
        {
            _wpos = newPos / 8;
            _bitpos = 8 - (newPos % 8);
            return _wpos * 8 + 8 - _bitpos;
        }

        template<typename T>
        void read_skip() { read_skip(sizeof(T)); }

        void read_skip(size_t skip)
        {
            if (_rpos + skip > size())
                throw ByteBufferPositionException(_rpos, skip, size());

            ResetBitPos();
            _rpos += skip;
        }

        template <typename T, typename Underlying = T>
        T read()
        {
            ResetBitPos();
            T r = read<T, Underlying>(_rpos);
            _rpos += sizeof(Underlying);
            return r;
        }

        template <typename T, typename Underlying = T>
        T read(size_t pos) const
        {
            if (pos + sizeof(Underlying) > size())
                throw ByteBufferPositionException(pos, sizeof(Underlying), size());
            Underlying val;
            std::memcpy(&val, &_storage[pos], sizeof(Underlying));
            return static_cast<T>(val);
        }

        template<class T>
        void read(T* dest, size_t count)
        {
            static_assert(std::is_trivially_copyable<T>::value, "read(T*, size_t) must be used with trivially copyable types");
            return read(reinterpret_cast<std::uint8_t*>(dest), count * sizeof(T));
        }

        void read(std::uint8_t *dest, size_t len)
        {
            if (_rpos + len > size())
               throw ByteBufferPositionException(_rpos, len, size());

            ResetBitPos();
            std::memcpy(dest, &_storage[_rpos], len);
            _rpos += len;
        }

        template <size_t Size>
        void read(std::array<std::uint8_t, Size>& arr)
        {
            read(arr.data(), Size);
        }

        void ReadPackedUint64(std::uint64_t& guid)
        {
            guid = 0;
            ReadPackedUint64(read<std::uint8_t>(), guid);
        }

        void ReadPackedUint64(std::uint8_t mask, std::uint64_t& value)
        {
            for (std::uint32_t i = 0; i < 8; ++i)
                if (mask & (std::uint8_t(1) << i))
                    value |= (std::uint64_t(read<std::uint8_t>()) << (i * 8));
        }

        //! Method for writing strings that have their length sent separately in packet
        //! without null-terminating the string
        void WriteString(std::string const& str)
        {
            if (size_t len = str.length())
                append(str.c_str(), len);
        }

        void WriteString(char const* str, size_t len)
        {
            if (len)
                append(str, len);
        }

        std::string ReadCString(bool requireValidUtf8 = true);

        std::string ReadString(std::uint32_t length, bool requireValidUtf8 = true);

        std::uint32_t ReadPackedTime();

        std::uint8_t* contents()
        {
            if (_storage.empty())
                throw ByteBufferException();
            return _storage.data();
        }

        std::uint8_t const* contents() const
        {
            if (_storage.empty())
                throw ByteBufferException();
            return _storage.data();
        }

        size_t size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        void resize(size_t newsize)
        {
            _storage.resize(newsize, 0);
            _rpos = 0;
            _wpos = size();
        }

        void reserve(size_t ressize)
        {
            if (ressize > size())
                _storage.reserve(ressize);
        }

        void shrink_to_fit()
        {
            _storage.shrink_to_fit();
        }

        void append(const char *src, size_t cnt)
        {
            return append((const std::uint8_t *)src, cnt);
        }

        template<class T>
        void append(const T *src, size_t cnt)
        {
            return append((const std::uint8_t *)src, cnt * sizeof(T));
        }

        void append(std::uint8_t const* src, size_t cnt);

        void append(ByteBuffer const& buffer)
        {
            if (!buffer.empty())
                append(buffer.contents(), buffer.size());
        }

        template <size_t Size>
        void append(std::array<std::uint8_t, Size> const& arr)
        {
            append(arr.data(), Size);
        }

        // can be used in SMSG_MONSTER_MOVE opcode
        void appendPackXYZ(float x, float y, float z)
        {
            std::uint32_t packed = 0;
            packed |= ((int)(x / 0.25f) & 0x7FF);
            packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
            packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
            *this << packed;
        }

        void AppendPackedUint64(std::uint64_t guid)
        {
            std::uint8_t mask = 0;
            size_t pos = wpos();
            *this << std::uint8_t(mask);

            std::uint8_t packed[8];
            if (size_t packedSize = PackUint64(guid, &mask, packed))
                append(packed, packedSize);

            put<std::uint8_t>(pos, mask);
        }

        static size_t PackUint64(std::uint64_t value, std::uint8_t* mask, std::uint8_t* result)
        {
            size_t resultSize = 0;
            *mask = 0;
            memset(result, 0, 8);

            for (std::uint8_t i = 0; value != 0; ++i)
            {
                if (value & 0xFF)
                {
                    *mask |= std::uint8_t(1 << i);
                    result[resultSize++] = std::uint8_t(value & 0xFF);
                }

                value >>= 8;
            }

            return resultSize;
        }

        void AppendPackedTime(time_t time);

        void put(size_t pos, std::uint8_t const* src, size_t cnt);

    protected:
        size_t _rpos, _wpos, _bitpos;
        std::uint8_t _curbitval;
        std::vector<std::uint8_t> _storage;
};

/// @todo Make a ByteBuffer.cpp and move all this inlining to it.
template<> inline std::string ByteBuffer::read<std::string>()
{
    std::string tmp;
    *this >> tmp;
    return tmp;
}

template<>
inline void ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;
}

template<>
inline void ByteBuffer::read_skip<char const*>()
{
    read_skip<char*>();
}

template<>
inline void ByteBuffer::read_skip<std::string>()
{
    read_skip<char*>();
}

#endif
