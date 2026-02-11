#include "opf/common.hpp"
#include <cstdlib> // For rand()
#include <new>     // For std::bad_alloc
#include <span>

namespace base64 {

    static constexpr size_t BITS_PER_BYTE = 8;
    static constexpr size_t BITS_PER_B64_UNIT = 6;
    static constexpr uint8_t B64_MASK = 0x3F;   // 00111111 (6 bits)
    static constexpr uint8_t BYTE_MASK = 0xFF;  // 11111111 (8 bits)
    static constexpr char PADDING_CHAR = '=';

    static constexpr std::string_view CHARSET = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static constexpr char CHARSET_SIZE = 64;
    static constexpr char TABLE_SIZE = 256; // 64 chars + padding + invalid chars
    auto build_decode_table() -> std::array<int, TABLE_SIZE>
    {
        std::array<int, TABLE_SIZE> table;
        table.fill(-1);
        for (int i = 0; i < CHARSET_SIZE; ++i)
        {
            table[static_cast<uint8_t>(CHARSET[i])] = i;
        }
        return table;
    }
    static auto DECODE_TABLE = build_decode_table();
        
    auto encode(std::span<const std::uint8_t> data) -> std::string
    {
        std::string out;
        int accumulator = 0;
        int bits_in_accumulator = -BITS_PER_B64_UNIT;

        for (const uint8_t byte : data) 
        {
            accumulator = (accumulator << BITS_PER_BYTE) | byte;
            bits_in_accumulator += BITS_PER_BYTE;
            while (bits_in_accumulator >= 0) {
                out.push_back(CHARSET[(accumulator >> bits_in_accumulator) & B64_MASK]);
                bits_in_accumulator -= BITS_PER_B64_UNIT;
            }
        }

        if (bits_in_accumulator > -BITS_PER_B64_UNIT) 
        {
            out.push_back(CHARSET[((accumulator << BITS_PER_B64_UNIT) >> (bits_in_accumulator + BITS_PER_B64_UNIT)) & B64_MASK]);
        }

        while (out.size() % 4 != 0)
        {
            out.push_back(PADDING_CHAR);
        }
        return out;
    }

    auto decode(std::string_view data) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> out;
        int accumulator = 0;
        int bits_in_accumulator = -BITS_PER_BYTE;

        for (const char c : data) {
            const int value = DECODE_TABLE[static_cast<uint8_t>(c)];
            if (value == -1)
            {
                break; // break padding or invalid char
            }

            accumulator = (accumulator << BITS_PER_B64_UNIT) | value;
            bits_in_accumulator += BITS_PER_B64_UNIT;

            if (bits_in_accumulator >= 0) 
            {
                out.push_back(static_cast<std::uint8_t>((accumulator >> bits_in_accumulator) & BYTE_MASK));
                bits_in_accumulator -= BITS_PER_BYTE;
            }
        }
        return out;
    }
}

namespace opf
{

    auto AllocIntArray(int n, int default_value = 0) -> std::vector<int>
    {
        try
        {
            return std::vector<int>(n, default_value);
        }
        catch (const std::bad_alloc &e)
        {
            Error(MSG1, "AllocIntArray");
        }
    }

    auto AllocFloatArray(int n, float default_value = 0.0F) -> std::vector<float>
    {
        try
        {
            return std::vector<float>(n, default_value);
        }
        catch (const std::bad_alloc &e)
        {
            Error(MSG1, "AllocFloatArray");
        }
    }
} // namespace opf