
#include "ByteBuffer/ByteBuffer.h"
#include <fmt/core.h>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <cstdio>

using namespace fmt::literals;

#pragma pack(push, 1)

namespace WDB
{
    struct FileHeader
    {
        std::array<char, 4> Magic = { };
        std::uint32_t Build = 0;
        std::array<char, 4> Locale = { };
        std::uint32_t RecordSize = 0;
        std::uint32_t RecordVersion = 0;
        std::uint32_t CacheVersion = 0;
    };
}

namespace PKT
{
    struct FileHeader
    {
        std::array<char, 3> Signature = { 'P', 'K', 'T' };
        std::uint16_t FormatVersion = 0x301;
        std::uint8_t SnifferId = 0;
        std::uint32_t Build = 0;
        std::array<char, 4> Locale = { };
        std::array<std::uint8_t, 40> SessionKey = { };
        std::uint32_t SniffStartUnixtime = 0;
        std::uint32_t SniffStartTicks = 0;
        std::uint32_t OptionalDataSize = 0;
    };

    struct PacketHeader
    {
        std::uint32_t Direction = 0x47534d53;
        std::uint32_t ConnectionId = 0;
        std::uint32_t ArrivalTicks = 0;
        std::uint32_t OptionalDataSize = 0;
        std::uint32_t Length = 0;
    };
}

#pragma pack(pop)

std::uint32_t GetOpcodeValue(std::array<char, 4> wdbMagic, std::uint32_t build)
{
    switch (build)
    {
        // 9.0.1
        case 36216:
        case 36228:
        case 36230:
        case 36247:
        case 36272:
        case 36322:
        case 36372:
        case 36492:
        case 36577:
        // 9.0.2
        case 36639:
        case 36665:
        case 36671:
        case 36710:
        case 36734:
        case 36751:
        case 36753:
        case 36839:
        case 36949:
        case 37106:
        case 37142:
        case 37176:
        case 37474:
            if (wdbMagic == std::array{ 'B', 'O', 'M', 'W' })
                return 0x26CE; // SMSG_QUERY_CREATURE_RESPONSE
            else if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
                return 0x26CF; // SMSG_QUERY_GAME_OBJECT_RESPONSE
            else if (wdbMagic == std::array{ 'C', 'P', 'N', 'W' })
                return 0x26D2; // SMSG_QUERY_NPC_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'X', 'T', 'P', 'W' })
                return 0x26D3; // SMSG_QUERY_PAGE_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'T', 'S', 'Q', 'W' })
                return 0x2A95; // SMSG_QUERY_QUEST_INFO_RESPONSE
            else
                throw std::invalid_argument(fmt::format("Unsupported WDB header {}{}{}{}", wdbMagic[0], wdbMagic[1], wdbMagic[2], wdbMagic[3]));
        // 9.0.5
        case 37503:
        case 37862:
        case 37864:
        case 37893:
        case 37899:
        case 37988:
        case 38134:
        case 38556:
            if (wdbMagic == std::array{ 'B', 'O', 'M', 'W' })
                return 0x26CE; // SMSG_QUERY_CREATURE_RESPONSE
            else if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
                return 0x26CF; // SMSG_QUERY_GAME_OBJECT_RESPONSE
            else if (wdbMagic == std::array{ 'C', 'P', 'N', 'W' })
                return 0x26D2; // SMSG_QUERY_NPC_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'X', 'T', 'P', 'W' })
                return 0x26D3; // SMSG_QUERY_PAGE_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'T', 'S', 'Q', 'W' })
                return 0x2A96; // SMSG_QUERY_QUEST_INFO_RESPONSE
            else
                throw std::invalid_argument(fmt::format("Unsupported WDB header {}{}{}{}", wdbMagic[0], wdbMagic[1], wdbMagic[2], wdbMagic[3]));
        // 9.1.0
        case 39185:
        case 39226:
        case 39229:
        case 39262:
        case 39282:
        case 39289:
        case 39291:
        case 39318:
        case 39335:
        case 39427:
        case 39497:
        case 39498:
        case 39584:
        case 39617:
        case 39653:
        case 39804:
        case 40000:
        case 40120:
        case 40443:
        case 40593:
        case 40725:
        // 9.1.5
        case 40772:
        case 40871:
        case 40906:
        case 40944:
        case 40966:
        case 41031:
        case 41079:
        case 41288:
        case 41323:
        case 41359:
        case 41488:
        case 41793:
            if (wdbMagic == std::array{ 'B', 'O', 'M', 'W' })
                return 0x2914; // SMSG_QUERY_CREATURE_RESPONSE
            else if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
                return 0x2915; // SMSG_QUERY_GAME_OBJECT_RESPONSE
            else if (wdbMagic == std::array{ 'C', 'P', 'N', 'W' })
                return 0x2916; // SMSG_QUERY_NPC_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'X', 'T', 'P', 'W' })
                return 0x2917; // SMSG_QUERY_PAGE_TEXT_RESPONSE
            else if (wdbMagic == std::array{ 'T', 'S', 'Q', 'W' })
                return 0x2A96; // SMSG_QUERY_QUEST_INFO_RESPONSE
            else
                throw std::invalid_argument(fmt::format("Unsupported WDB header {}{}{}{}", wdbMagic[0], wdbMagic[1], wdbMagic[2], wdbMagic[3]));
        default:
            throw std::invalid_argument(fmt::format("Unsupported client build {}", build));
    }
}

void ProcessWDBRecord(ByteBuffer& wdb, std::array<char, 4> wdbMagic, std::uint32_t build, std::int32_t id, std::uint32_t recordSize, ByteBuffer& pkt)
{
    PKT::PacketHeader header;

    // create a wrapper packet
    std::size_t headerPos = pkt.wpos();

    pkt.append(header);

    std::size_t pktPos = pkt.wpos();

    pkt.append<std::uint32_t>(GetOpcodeValue(wdbMagic, build));

    pkt.append<std::int32_t>(id);
    if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
        pkt.append<std::uint16_t>(0); // empty guid mask

    pkt.WriteBit(true);
    pkt.FlushBits();

    if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
        pkt.append<std::uint32_t>(1); // data size - doesnt actually matter to fill it properly, WPP is only checking != 0
    else if (wdbMagic == std::array{ 'C', 'P', 'N', 'W' })
        pkt.append<std::uint32_t>(64); // data size
    else if (wdbMagic == std::array{ 'X', 'T', 'P', 'W' })
        pkt.append<std::uint32_t>(1); // page count

    pkt.append(wdb.contents() + wdb.rpos(), recordSize);
    wdb.rpos(wdb.rpos() + recordSize);

    reinterpret_cast<PKT::PacketHeader*>(pkt.contents() + headerPos)->Length = static_cast<std::uint32_t>(pkt.wpos() - pktPos);
}

std::size_t ProcessWDB(ByteBuffer& wdb, ByteBuffer& pkt)
{
    WDB::FileHeader header;
    wdb.read(header.Magic.data(), header.Magic.size());
    wdb >> header.Build;
    wdb.read(header.Locale.data(), header.Locale.size());
    wdb >> header.RecordSize;
    wdb >> header.RecordVersion;
    wdb >> header.CacheVersion;

    PKT::FileHeader pktHeader;
    pktHeader.Build = header.Build;
    std::reverse_copy(header.Locale.begin(), header.Locale.end(), pktHeader.Locale.begin());

    pkt.append(pktHeader);

    std::size_t processedRecords = 0;

    while (wdb.rpos() + 8 < wdb.size())
    {
        std::int32_t id = wdb.read<std::int32_t>();
        std::uint32_t recordSize = wdb.read<std::uint32_t>();
        if (!recordSize)
            continue;

        ProcessWDBRecord(wdb, header.Magic, header.Build, id, recordSize, pkt);
        ++processedRecords;
    }

    return processedRecords;
}

int main(int argc, char const* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        FILE* inFile = nullptr;
        if (fopen_s(&inFile, argv[i], "rb") || !inFile)
            continue;

        std::filesystem::path inPath(argv[i]);
        std::uintmax_t size = std::filesystem::file_size(inPath);

        ByteBuffer data(size, ByteBuffer::Resize{ });

        fread(data.contents(), size, 1, inFile);

        ByteBuffer pkt;
        try
        {
            std::size_t processedRecords = ProcessWDB(data, pkt);
            if (processedRecords > 0)
            {
                FILE* out = nullptr;
                if (!fopen_s(&out, inPath.replace_filename(inPath.filename().replace_extension("pkt")).string().c_str(), "wb") && out)
                {
                    fwrite(pkt.contents(), pkt.size(), 1, out);
                    fclose(out);
                }
            }
        }
        catch (std::exception const& ex)
        {
            fmt::print("Caught exception when processing {path}: {exception}", fmt::arg("path", inPath.filename().string()), fmt::arg("exception", ex.what()));
        }

        fclose(inFile);
    }

    return 0;
}
