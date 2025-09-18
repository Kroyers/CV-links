#include "Compression.h"
#include "huf.h"
#define MAX(a,b) (((a)>(b))?(a):(b))
#define word_t char
lzw::lzw(uint16_t mtu) {
    this->compression_buf_sz = mtu * 2;
    //initialize compression buffer
    compression_buf = new uint8_t[compression_buf_sz];
    for (int i = 0; i < compression_buf_sz ; i++) {
        *(compression_buf + i) = 0;
    }
    compressed_total = 0;
}

namespace globals {

/// Dictionary Maximum Size (when reached, the dictionary will be reset)
const CodeType dms {std::numeric_limits<CodeType>::max()};

} // namespace globals

///
/// @brief Helper hash functor, to be used on character containers.
///
struct HashCharVector {

    ///
    /// @param [in]     vc
    /// @returns Hash of the vector of characters.
    ///
    std::size_t operator () (const std::vector<char> &vc) const
    {
        return std::hash<std::string>()(std::string(vc.cbegin(), vc.cend()));
    }
};

///
/// @brief Helper operator intended to simplify code.
/// @param vc   original vector
/// @param c    element to be appended
/// @returns vector resulting from appending `c` to `vc`
///
std::vector<char> operator + (std::vector<char> vc, char c)
{
    vc.push_back(c);
    return vc;
}

uint32_t lzw::compress(char* input_buffer, uint32_t input_len, char* compressed_buffer, uint32_t compressed_buffer_len)
{
    uint32_t num_compressed_bytes = 0;
    std::unordered_map<std::vector<char>, CodeType, HashCharVector> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[{static_cast<char> (c)}] = dictionary_size;
        }
    };

    reset_dictionary();

    std::vector<char> s; // String
    char c;

    //while (is.get(c))
    for (uint32_t i = 0; i < input_len; i++)
    {
        c = *(input_buffer + i);
        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        s.push_back(c);

        if (dictionary.count(s) == 0)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[s] = dictionary_size;
            s.pop_back();
            CodeType compressed = dictionary.at(s);
            if (num_compressed_bytes + 2 > compressed_buffer_len)
            {
                throw std::runtime_error("Buffer overflow! Compression buffer too small");
            }
            *(compressed_buffer + num_compressed_bytes) = compressed >> 8;
            CodeType low = compressed << 8;
            low = low >> 8;
            *(compressed_buffer + num_compressed_bytes + 1) = low;
            num_compressed_bytes += 2;
            s = {c};
        }
    }

    if (!s.empty())
    {
    	CodeType compressed = dictionary.at(s);
        if (num_compressed_bytes + 2 > compressed_buffer_len)
        {
            throw std::runtime_error("Buffer overflow! Compression buffer too small");
        }
        *(compressed_buffer + num_compressed_bytes) = compressed >> 8;
        *(compressed_buffer + num_compressed_bytes + 1) = (compressed << 8) >> 8;
        num_compressed_bytes += 2;
    }
    return num_compressed_bytes;
}



uint32_t lzw::decompress(char* compressed_buffer, uint32_t compressed_len, char* output_buffer, uint32_t output_buffer_len)
{
    uint32_t num_decompressed_bytes = 0;
    std::vector<std::vector<char>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();
        dictionary.reserve(globals::dms);

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
            dictionary.push_back({static_cast<char> (c)});
    };

    reset_dictionary();

    std::vector<char> s; // String
    CodeType k; // Key

    for (uint32_t i = 0; i < compressed_len; i+=2)
    {
        k = ((uint8_t)compressed_buffer[i] << 8) + (uint8_t)compressed_buffer[i+1];

        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        if (k > dictionary.size())
        {
            throw std::runtime_error("invalid compressed code");
        }

        if (k == dictionary.size())
            dictionary.push_back(s + s.front());
        else
        if (!s.empty())
            dictionary.push_back(s + dictionary.at(k).front());

        uint16_t decompressed = dictionary.at(k).front();
        if (num_decompressed_bytes + 2 > output_buffer_len)
        {
            throw std::runtime_error("Buffer overflow! Output buffer too small");
        }
        memcpy(output_buffer + num_decompressed_bytes, &dictionary.at(k).front(), dictionary.at(k).size());
        num_decompressed_bytes += dictionary.at(k).size();
        s = dictionary.at(k);
    }
    return num_decompressed_bytes;
}

//---------------------------HUFFMAN ENCODING---------------------------


char* huffman::compress(char* input, uint32_t input_size, uint32_t& package_size)
{
    // build Huffman tree and two arrays representing the tree (for spatial locality in memory)
    char* huffman_array;
    bool* terminator_array;
    Node<char>* tree;
    uint16_t hufman_array_size = generate_array_tree_representation(input, input_size, huffman_array, terminator_array, tree);

    // generate encoding dictionary from tree
    encoding_dict<char> encoding_dict;
    build_inverse_mapping(tree, encoding_dict);

    // compress
    char* compressed;
    long compressed_size = encode(input, input_size, compressed, encoding_dict);

    // package
    package_size = sizeof(input_size) + sizeof(hufman_array_size) + 2 * hufman_array_size + compressed_size;
    char* package_buffer = new char[package_size];
    memcpy(package_buffer, &input_size, sizeof(input_size));
    uint32_t array_size_offset = sizeof(input_size);
    memcpy(package_buffer + array_size_offset, &hufman_array_size, sizeof(hufman_array_size));
    uint32_t huffman_array_offset = array_size_offset + sizeof(hufman_array_size);
    memcpy(package_buffer + huffman_array_offset, huffman_array, hufman_array_size);
    uint32_t terminator_array_offset = huffman_array_offset + hufman_array_size;
    memcpy(package_buffer + terminator_array_offset, terminator_array, hufman_array_size);
    uint32_t compressed_offset = terminator_array_offset + hufman_array_size;
    memcpy(package_buffer + compressed_offset, compressed, compressed_size);
    return package_buffer;
}

char* huffman::decompress(char* package_buffer, uint32_t& original_size)
{
    // depackage data from package buffer
    original_size = *((uint32_t*)package_buffer);
    uint16_t original_array_size = *(package_buffer + sizeof(original_size));

    uint32_t huffman_array_offset = sizeof(original_size) + sizeof(original_array_size);
    char* huffman_array = package_buffer + huffman_array_offset;
    uint32_t terminator_array_offset = huffman_array_offset + original_array_size;
    bool* terminator_array = (bool*) (package_buffer + terminator_array_offset);
    uint32_t compressed_offset = terminator_array_offset + original_array_size;
    char* compressed = package_buffer + compressed_offset;

    char* decompressed;
    decode(compressed, original_size, decompressed, huffman_array, terminator_array);
    return decompressed;
}
