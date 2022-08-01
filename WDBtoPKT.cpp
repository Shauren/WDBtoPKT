
#include "ByteBuffer/ByteBuffer.h"
#include <msclr/marshal_cppstd.h>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <cstdio>

using namespace std::string_literals;

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

std::uint32_t GetOpcodeValueFromWPP(std::array<char, 4> wdbMagic)
{
    using namespace WowPacketParser::Enums;

    Opcode opcode{};

    if (wdbMagic == std::array{ 'B', 'O', 'M', 'W' })
        opcode = Opcode::SMSG_QUERY_CREATURE_RESPONSE;
    else if (wdbMagic == std::array{ 'B', 'O', 'G', 'W' })
        opcode = Opcode::SMSG_QUERY_GAME_OBJECT_RESPONSE;
    else if (wdbMagic == std::array{ 'C', 'P', 'N', 'W' })
        opcode = Opcode::SMSG_QUERY_NPC_TEXT_RESPONSE;
    else if (wdbMagic == std::array{ 'X', 'T', 'P', 'W' })
        opcode = Opcode::SMSG_QUERY_PAGE_TEXT_RESPONSE;
    else if (wdbMagic == std::array{ 'T', 'S', 'Q', 'W' })
        opcode = Opcode::SMSG_QUERY_QUEST_INFO_RESPONSE;
    else
        throw std::invalid_argument("Unsupported WDB header "s + wdbMagic[0] + wdbMagic[1] + wdbMagic[2] + wdbMagic[3]);

    return Version::Opcodes::GetOpcode(opcode, Direction::ServerToClient);
}

void ProcessWDBRecord(ByteBuffer& wdb, std::array<char, 4> wdbMagic, std::uint32_t build, std::int32_t id, std::uint32_t recordSize, ByteBuffer& pkt)
{
    PKT::PacketHeader header;

    // create a wrapper packet
    std::size_t headerPos = pkt.wpos();

    pkt.append(header);

    std::size_t pktPos = pkt.wpos();

    pkt.append<std::uint32_t>(GetOpcodeValueFromWPP(wdbMagic));

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

    WowPacketParser::Misc::ClientVersion::SetVersion(WowPacketParser::Enums::ClientVersionBuild(header.Build));

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

namespace WDBtoPKT
{
public ref class WDBtoPKT
{
public:
    static void Run(array<System::String^>^ args)
    {
        for (int i = 0; i < args->Length; ++i)
        {
            FILE* inFile = nullptr;
            std::string inPathString = msclr::interop::marshal_as<std::string>(args[i]->ToString());
            if (fopen_s(&inFile, inPathString.c_str(), "rb") || !inFile)
                continue;

            std::filesystem::path inPath(inPathString);
            std::uintmax_t size = std::filesystem::file_size(inPath);

            ByteBuffer data(size, ByteBuffer::Resize{ });

            fread(data.contents(), size, 1, inFile);

            try
            {
                ByteBuffer pkt;
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
                printf("Caught exception when processing %s: %s", inPath.filename().string().c_str(), ex.what());
            }

            fclose(inFile);
        }
    }
};
}
