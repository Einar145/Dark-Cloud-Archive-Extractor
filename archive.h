#pragma once
#include <iostream>
#include "utils.h"

#define ARCH_ENTRY_SIZE 0x50


#pragma pack(1)
struct ARCH_file_entry {
    char name[64];          // 0x0
    u32 metadata_length;    // 0x40
    i32 file_length;        // 0x44
    i32 end_offset;         // 0x48
    u32 unknown1;           // 0x4C

    bool end() {
        return name[0] == '\0' || file_length == -1 || end_offset == -1;
    }
}; // 180 bytes

struct arch_error {
    enum {
        none                = 0,
        bad_arch_size       = 1,
        bad_offset_field    = 2,
        bad_length_field    = 3,
        bad_format          = 4,
    };

    static const char* to_string(int e)
    {
        switch (e)
        {
            case arch_error::none: return "none";
            case arch_error::bad_arch_size: return "bad archive size";
            case arch_error::bad_offset_field: return "bad offset field";
            case arch_error::bad_length_field: return "bad length field";
            case arch_error::bad_format: return "bad format";
            default: return "unknown";
        }
    }
    static bool has_error(int e) {
        return e != arch_error::none;
    }
};

// returns arch_error
int extract_archive(const char* filename, bool create_directory = true)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    int asize = file.tellg();

    // we need at least two entries (first and ending)
    if (asize <= ARCH_ENTRY_SIZE * 2) 
        return arch_error::bad_arch_size;

    // read where the last entry would be
    ARCH_file_entry last;
    file.seekg(asize - ARCH_ENTRY_SIZE);
    file.read((char*)&last, sizeof(last));
    
    // if the end is a valid terminating node
    if (!last.end()) 
        return arch_error::bad_format;

    std::string path = create_directory ? fs::path(filename).stem().string() : "";
    
    file.seekg(std::ios::beg);

    while (true)
    {
        // save position
        int startp = file.tellg();

        ARCH_file_entry entry;
        file.read((char*)&entry, sizeof(entry));

        // calculate next offset
        int next_offs = startp + entry.end_offset;
        int next_length = entry.file_length;

        // if we have reached the end
        if (entry.end()) break;

        // next offset is less than current position or
        // next offset goes past file length
        if(next_offs < file.tellg() || next_offs > (asize - ARCH_ENTRY_SIZE))
            return arch_error::bad_offset_field;
        
        // length is negative or
        // length goes past file length
        if(next_length < 0 || next_length > (asize - file.tellg()))
            return arch_error::bad_length_field;

        // allocate memory to write into new file
        auto fdat = std::make_unique<u8[]>(next_length);
        file.read((char*)fdat.get(), next_length);

        // create and write to the new file (and directory if enabled)
        write_file(path, entry.name, std::move(fdat), next_length);
        
        // std::ofstream outfile(path + entry.name, std::ios::binary);
        // outfile.write((char*)fdat.get(), entry.file_length);

        file.seekg(next_offs);
    }

    return arch_error::none;
}