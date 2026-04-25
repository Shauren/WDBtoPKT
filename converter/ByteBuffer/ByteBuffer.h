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

#ifndef TRINITYCORE_BYTE_BUFFER_H
#define TRINITYCORE_BYTE_BUFFER_H

#include <array>
#include <concepts>
#include <string>
#include <vector>
#include <cstring>

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    explicit ByteBufferException() = default;
    explicit ByteBufferException(std::string&& message) noexcept : msg_(std::move(message)) { }

    char const* what() const noexcept override { return msg_.c_str(); }

protected:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(size_t pos, size_t size, size_t valueSize);
};

class ByteBufferInvalidValueException : public ByteBufferException
{
public:
    ByteBufferInvalidValueException(char const* type, std::string_view value);
};

template <typename T>
concept ByteBufferNumeric = std::same_as<T, std::int8_t> || std::same_as<T, std::uint8_t>
    || std::same_as<T, std::int16_t> || std::same_as<T, std::uint16_t>
    || std::same_as<T, std::int32_t> || std::same_as<T, std::uint32_t>
    || std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t>
    || std::same_as<T, float> || std::same_as<T, double>
    || std::same_as<T, char> || std::is_enum_v<T>;

class ByteBuffer
{
    public:
        constexpr static std::size_t DEFAULT_SIZE = 0x1000;
        constexpr static std::uint8_t InitialBitPos = 8;

        // reserve/resize tag
        struct Reserve { };
        struct Resize { };

        // constructor
        explicit ByteBuffer() : ByteBuffer(DEFAULT_SIZE, Reserve{}) { }

        explicit ByteBuffer(size_t size, Reserve) : _rpos(0), _wpos(0), _bitpos(InitialBitPos), _curbitval(0)
        {
            _storage.reserve(size);
        }

        explicit ByteBuffer(size_t size, Resize) : _rpos(0), _wpos(size), _bitpos(InitialBitPos), _curbitval(0)
        {
            _storage.resize(size);
        }

        ByteBuffer(ByteBuffer const& right) = default;

        ByteBuffer(ByteBuffer&& buf) noexcept : _rpos(buf._rpos), _wpos(buf._wpos),
            _bitpos(buf._bitpos), _curbitval(buf._curbitval), _storage(std::move(buf).Release()) { }

        explicit ByteBuffer(std::vector<std::uint8_t>&& buffer) noexcept : _rpos(0), _wpos(buffer.size()),
            _bitpos(InitialBitPos), _curbitval(0), _storage(std::move(buffer)) { }

        std::vector<std::uint8_t>&& Release() && noexcept
        {
            _rpos = 0;
            _wpos = 0;
            _bitpos = InitialBitPos;
            _curbitval = 0;
            return std::move(_storage);
        }

        ByteBuffer& operator=(ByteBuffer const& right) = default;

        ByteBuffer& operator=(ByteBuffer&& right) noexcept
        {
            if (this != &right)
            {
                _rpos = right._rpos;
                _wpos = right._wpos;
                _bitpos = right._bitpos;
                _curbitval = right._curbitval;
                _storage = std::move(right).Release();
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

        template <ByteBufferNumeric T>
        void append(T value)
        {
            append(reinterpret_cast<std::uint8_t const*>(&value), sizeof(value));
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

            append(&_curbitval, sizeof(std::uint8_t));
            _curbitval = 0;
        }

        void ResetBitPos()
        {
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
                append(&_curbitval, sizeof(_curbitval));
                _curbitval = 0;
            }

            return bit;
        }

        bool ReadBit()
        {
            if (_bitpos >= 8)
            {
                read(&_curbitval, 1);
                _bitpos = 0;
            }

            return ((_curbitval >> (8 - ++_bitpos)) & 1) != 0;
        }

        void WriteBits(std::uint64_t value, std::int32_t bits)
        {
            // remove bits that don't fit
            value &= (UINT64_C(1) << bits) - 1;

            if (bits > std::int32_t(_bitpos))
            {
                // first write to fill bit buffer
                _curbitval |= value >> (bits - _bitpos);
                bits -= _bitpos;
                _bitpos = 8; // required "unneccessary" write to avoid double flushing
                append(&_curbitval, sizeof(_curbitval));

                // then append as many full bytes as possible
                while (bits >= 8)
                {
                    bits -= 8;
                    append<std::uint8_t>(value >> bits);
                }

                // store remaining bits in the bit buffer
                _bitpos = 8 - bits;
                _curbitval = (value & ((UINT64_C(1) << bits) - 1)) << _bitpos;
            }
            else
            {
                // entire value fits in the bit buffer
                _bitpos -= bits;
                _curbitval |= value << _bitpos;

                if (_bitpos == 0)
                {
                    _bitpos = 8;
                    append(&_curbitval, sizeof(_curbitval));
                    _curbitval = 0;
                }
            }
        }

        std::uint32_t ReadBits(std::int32_t bits)
        {
            std::uint32_t value = 0;
            if (bits > 8 - std::int32_t(_bitpos))
            {
                // first retrieve whatever is left in the bit buffer
                std::int32_t bitsInBuffer = 8 - _bitpos;
                value = (_curbitval & ((UINT64_C(1) << bitsInBuffer) - 1)) << (bits - bitsInBuffer);
                bits -= bitsInBuffer;

                // then read as many full bytes as possible
                while (bits >= 8)
                {
                    bits -= 8;
                    value |= read<std::uint8_t>() << bits;
                }

                // and finally any remaining bits
                if (bits)
                {
                    read(&_curbitval, 1);
                    value |= (_curbitval >> (8 - bits)) & ((UINT64_C(1) << bits) - 1);
                    _bitpos = bits;
                }
            }
            else
            {
                // entire value is in the bit buffer
                value = (_curbitval >> (8 - _bitpos - bits)) & ((UINT64_C(1) << bits) - 1);
                _bitpos += bits;
            }

            return value;
        }

        template <ByteBufferNumeric T>
        void put(std::size_t pos, T value)
        {
            put(pos, reinterpret_cast<std::uint8_t const*>(&value), sizeof(value));
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

        ByteBuffer& operator<<(bool) = delete;  // prevent implicit conversions to int32

        ByteBuffer& operator<<(char value)
        {
            append<char>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::uint8_t value)
        {
            append<std::uint8_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::uint16_t value)
        {
            append<std::uint16_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::uint32_t value)
        {
            append<std::uint32_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::uint64_t value)
        {
            append<std::uint64_t>(value);
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer& operator<<(std::int8_t value)
        {
            append<std::int8_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::int16_t value)
        {
            append<std::int16_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::int32_t value)
        {
            append<std::int32_t>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::int64_t value)
        {
            append<std::int64_t>(value);
            return *this;
        }

        // floating points
        ByteBuffer& operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        ByteBuffer& operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        ByteBuffer& operator<<(std::string_view value)
        {
            if (size_t len = value.length())
                append(reinterpret_cast<std::uint8_t const*>(value.data()), len);
            append(static_cast<std::uint8_t>(0));
            return *this;
        }

        ByteBuffer& operator<<(std::string const& str)
        {
            return operator<<(std::string_view(str));
        }

        ByteBuffer& operator<<(char const* str)
        {
            return operator<<(std::string_view(str ? str : ""));
        }

        ByteBuffer& operator>>(bool&) = delete;

        ByteBuffer& operator>>(char& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::uint8_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::uint16_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::uint32_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::uint64_t& value)
        {
            read(&value, 1);
            return *this;
        }

        //signed as in 2e complement
        ByteBuffer& operator>>(std::int8_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::int16_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::int32_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(std::int64_t& value)
        {
            read(&value, 1);
            return *this;
        }

        ByteBuffer& operator>>(float& value);
        ByteBuffer& operator>>(double& value);

        ByteBuffer& operator>>(std::string& value)
        {
            value = ReadCString();
            return *this;
        }

        std::uint8_t& operator[](size_t const pos)
        {
            if (pos >= _storage.size())
                OnInvalidPosition(pos, 1);
            return _storage[pos];
        }

        std::uint8_t const& operator[](size_t const pos) const
        {
            if (pos >= _storage.size())
                OnInvalidPosition(pos, 1);
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

        template <ByteBufferNumeric T>
        void read_skip() { read_skip(sizeof(T)); }

        void read_skip(size_t skip)
        {
            if (_rpos + skip > _storage.size())
                OnInvalidPosition(_rpos, skip);

            ResetBitPos();
            _rpos += skip;
        }

        template <ByteBufferNumeric T>
        T read()
        {
            ResetBitPos();
            T r = read<T>(_rpos);
            _rpos += sizeof(T);
            return r;
        }

        template <ByteBufferNumeric T>
        T read(size_t pos) const
        {
            if (pos + sizeof(T) > _storage.size())
                OnInvalidPosition(pos, sizeof(T));

            T val;
            std::memcpy(&val, &_storage[pos], sizeof(T));
            return val;
        }

        template <ByteBufferNumeric T>
        void read(T* dest, size_t count)
        {
            static_assert(std::is_trivially_copyable_v<T>, "read(T*, size_t) must be used with trivially copyable types");
            read(reinterpret_cast<std::uint8_t*>(dest), count * sizeof(T));
        }

        void read(std::uint8_t* dest, size_t len)
        {
            if (_rpos + len > _storage.size())
                OnInvalidPosition(_rpos, len);

            ResetBitPos();
            std::memcpy(dest, &_storage[_rpos], len);
            _rpos += len;
        }

        template <ByteBufferNumeric T, size_t Size>
        void read(std::array<T, Size>& arr)
        {
            read(arr.data(), Size);
        }

        //! Method for writing strings that have their length sent separately in packet
        //! without null-terminating the string
        void WriteString(std::string const& str)
        {
            if (size_t len = str.length())
                append(str.c_str(), len);
        }

        void WriteString(std::string_view str)
        {
            if (size_t len = str.length())
                append(str.data(), len);
        }

        void WriteString(char const* str, size_t len)
        {
            if (len)
                append(str, len);
        }

        void ReadSkipCString() { (void)ReadCString(); }

        std::string_view ReadCString();

        std::string_view ReadString(std::uint32_t length);

        std::uint8_t* data() { return _storage.data(); }
        std::uint8_t const* data() const { return _storage.data(); }

        size_t size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        void resize(size_t newsize)
        {
            _storage.resize(newsize, 0);
            _rpos = 0;
            _wpos = _storage.size();
        }

        void reserve(size_t ressize)
        {
            if (ressize > _storage.size())
                _storage.reserve(ressize);
        }

        void shrink_to_fit()
        {
            _storage.shrink_to_fit();
        }

        template <ByteBufferNumeric T>
        void append(T const* src, size_t cnt)
        {
            for (size_t i = 0; i < cnt; ++i)
                this->append<T>(src[i]);
        }

        void append(std::uint8_t const* src, size_t cnt);

        void append(ByteBuffer const& buffer)
        {
            if (!buffer.empty())
                append(buffer.data(), buffer.size());
        }

        template <ByteBufferNumeric T, std::size_t Size>
        void append(std::array<T, Size> const& arr)
        {
            this->append(arr.data(), Size);
        }

        void put(size_t pos, std::uint8_t const* src, size_t cnt);

        [[noreturn]] void OnInvalidPosition(size_t pos, size_t valueSize) const;

    protected:
        size_t _rpos, _wpos;
        std::uint8_t _bitpos;
        std::uint8_t _curbitval;
        std::vector<std::uint8_t> _storage;
};

extern template char ByteBuffer::read<char>();
extern template std::uint8_t ByteBuffer::read<std::uint8_t>();
extern template std::uint16_t ByteBuffer::read<std::uint16_t>();
extern template std::uint32_t ByteBuffer::read<std::uint32_t>();
extern template std::uint64_t ByteBuffer::read<std::uint64_t>();
extern template std::int8_t ByteBuffer::read<std::int8_t>();
extern template std::int16_t ByteBuffer::read<std::int16_t>();
extern template std::int32_t ByteBuffer::read<std::int32_t>();
extern template std::int64_t ByteBuffer::read<std::int64_t>();
extern template float ByteBuffer::read<float>();
extern template double ByteBuffer::read<double>();

template <typename T>
concept HasByteBufferShiftOperators = requires(ByteBuffer& data, T const& value) { { data << value } -> std::convertible_to<ByteBuffer&>; }
                                   && requires(ByteBuffer& data, T& value)       { { data >> value } -> std::convertible_to<ByteBuffer&>; };

#endif
