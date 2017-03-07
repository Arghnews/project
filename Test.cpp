#include <string>
#include <iostream>
#include <sstream>

#include "Compress.hpp"

int main() {
    std::cout << "Hi\n";
    std::string s = "Hi there bill I'm going ouit on a walk right now I'll see you later matey pi\nasdasfgasfjkasfjsafjksafsafsafsafasfasfasfasdafhwhfiuafhuiwhiuvhcxvhxczivzicvczivihzvuzc asdjakdjwuarfakufhwauhf wu fhwaiufhwaiufahwai hfwaiu hfwaiufhwa fhuiwau hwa";
    std::cout << "Size of s:" << s.size() << "\n";
    std::string compressed = compress_string(s, 1);
    std::cout << "Size of compressed:" << compressed.size() << "\n";
    std::cout << compressed << "\n";

    std::string decomp = decompress_string(compressed);
    std::cout << "Decompressed\n";
    std::cout << decomp << "\n";

    
}
