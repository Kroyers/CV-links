#ifndef STP_PROTOCOL_COMPRESSION_H
#define STP_PROTOCOL_COMPRESSION_H

#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <stdint.h>

#include <stack>
#include <deque>
#include <queue>
#include <string.h>
//#include "Interface.h"

/// Type used to store and retrieve codes.
using CodeType = std::uint16_t;

class huffman{
	public:
		uint16_t compressed_total;
		char* compress(char* input, uint32_t input_size, uint32_t& package_size);
		char* decompress(char* package_buffer, uint32_t& original_size);
};

class lzw{
    public:   
        uint16_t mtu;
        uint16_t compressed_total;
        lzw(uint16_t mtu);
        uint8_t* compression_buf;
        uint16_t compression_buf_sz = 0;
      uint32_t decompress(char* compressed_buffer, uint32_t compressed_len, char* output_buffer, uint32_t output_buffer_len);
        uint32_t compress(char* input_buffer, uint32_t input_len, char* compressed_buffer, uint32_t compressed_buffer_len);
};


#endif
