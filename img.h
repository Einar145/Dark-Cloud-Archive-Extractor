#pragma once
#include <iostream>
#include "utils.h"

#define IMG_HEADER_SIZE 0x10
#define IMG_IM2_ENTRY_SIZE 0x30
#define IMG_IM3_ENTRY_SIZE 0x40

// we use this struct for both IM2 and IM3. they are the same size
// the only difference is where the file count is stored
#pragma pack(1)
struct IMG_header {
    char sig[4];        // 0x0
    u32 im2_file_count; // 0x4 // use this field for im2
    u32 im3_file_count; // 0x8 // use this field for im3
    u32 ukw1;           // 0xC
}; // 16 bytes

#pragma pack(1)
struct IMG_im2_file_entry {
    char name[32];  // 0x0
    u32 offset;     // 0x20
    u32 ukw0;       // 0x24
    u32 ukw1;       // 0x28
    u32 ukw2;       // 0x2C
}; // 48 bytes

#pragma pack(1)
struct IMG_im3_file_entry
{
    char name[32];      // 0x0
    u32 ukw0;           // 0x20
    u32 offset;         // 0x24
    u32 ukw1;           // 0x28
    char pad[8];        // 0x2C
    u32 file_length;    // 0x34
    u32 ukw2;           // 0x38
    u32 ukw3;           // 0x3C
}; // 64 bytes

struct img_status {
    enum {
        // img error
        no_error        = 1 << 0,
        bad_signature   = 1 << 1,
        bad_offset      = 1 << 2,

        // img type
        IMG = 1 << 3,
        IM2 = 1 << 4,
        IM3 = 1 << 5,
    };

    static const char* type_string(int e)
    {
        if (e & img_status::IMG) return "IMG";
        if (e & img_status::IM2) return "IM2";
        if (e & img_status::IM3) return "IM3";
        return "Unknown type";
    }

    static const char* error_string(int e)
    {
        if (e & img_status::no_error) return "no error";
        if (e & img_status::bad_signature) return "bad signature";
        if (e & img_status::bad_offset) return "bad offset";
        return "Unknown error";
    }
    static bool has_error(int e) {
        return !(e & img_status::no_error);
    }
};

constexpr std::array<std::array<char, 4>, 3> img_signatures = { "IMG", "IM2", "IM3" };

// bool is_img_file(const char* filename)
// {
//     if (fs::is_empty(filename)) return false;
//     auto sig = get_file_signature<4>(filename);
//     if (sig == img_signatures[0] || 
//         sig == img_signatures[1] || 
//         sig == img_signatures[2]) return true;
//     return false;
// }


// these functions its expected that we have checked the signature and that we are at offset IMG_HEADER_SIZE
int extract_im2_img(std::ifstream& file, const std::string& filename, int file_num, int fsize,  bool mkdir = true)
{
    // allocate variable number of file metadata
    auto entries = std::make_unique<IMG_im2_file_entry[]>(file_num);

    // read the metadata file entries
    for (int i = 0; i < file_num; i++)
        file.read((char*)&entries[i], sizeof(IMG_im2_file_entry));

    std::string path = mkdir ? fs::path(filename).stem().string() : "";

    // where the start offset should be
    int start_offs = (IMG_IM2_ENTRY_SIZE * file_num) + IMG_HEADER_SIZE;
    
    int last = start_offs;

    for (int i = 0; i < file_num; i++)
    {
        int offs = entries[i].offset;
        
        // get offset from entry 1 ahead to know how large the file size is
        // if there are no more entries, we use length of file archive
        int size = (i >= file_num - 1 ? fsize : entries[i + 1].offset) - offs;
        
        // if size has gone past the boundry
        if(size > fsize - start_offs)
            return img_status::bad_offset;
        
        // if current offset is larger than the last
        if(offs < last)
            return img_status::bad_offset;

        file.seekg(offs);

        // read file
        auto fdat = std::make_unique<u8[]>(size);
        file.read((char*)fdat.get(), size);

        // output to new file with .tm2 extension
        write_file(path, std::string(entries[i].name) + ".tm2", std::move(fdat), size);

        last = offs;
    }
    return img_status::no_error;
}
int extract_im3_img(std::ifstream& file, const std::string& filename, int file_num, int fsize, bool mkdir = true)
{
    std::string path = mkdir ? fs::path(filename).stem().string(): "";

    // where the start offset should be
    int start_offs = (IMG_IM3_ENTRY_SIZE * file_num) + IMG_HEADER_SIZE;
    
    for (int i = 0; i < file_num; i++)
    {
        // read file meta data
        IMG_im3_file_entry entry;
        file.read((char*)&entry, sizeof(entry));
        
        int size = entry.file_length;
        int savep = file.tellg();

        // if size has gone past the boundry
        if(size > fsize-start_offs)
            return img_status::bad_offset; 
        
        if(entry.offset < file.tellg())
            return img_status::bad_offset;

        file.seekg(entry.offset);

        // read file
        auto fdat = std::make_unique<u8[]>(size);
        file.read((char*)fdat.get(), size);

        file.seekg(savep);

        // write the file and mkdir if enabled
        write_file(path, std::string(entry.name) + ".tm2", std::move(fdat), size);
    }
    return img_status::no_error;
}

// these functions will check the signature and put the offset at IMG_HEADER_SIZE
int extract_im2_img(const char* filename, bool mkdir = true)
{
    std::ifstream file(filename, std::ios::in | std::ios::ate);

    int fsize = file.tellg();
    file.seekg(std::ios::beg);

    IMG_header head;
    file.read((char*)&head, sizeof(head));

    std::array<char, 4> sig{ head.sig[0], head.sig[1], head.sig[2], head.sig[3] };

    bool b_img = sig == img_signatures[0];
    bool b_im2 = sig == img_signatures[1];

    // not img signature or im2 signature
    if(!b_img || !b_im2)
        return img_status::bad_signature;

    return extract_im2_img(file, filename, head.im2_file_count, fsize, mkdir) | (b_img ? img_status::IMG : img_status::IM2);
}
int extract_im3_img(const char* filename, bool mkdir = true)
{
    std::ifstream file(filename, std::ios::in | std::ios::ate);
    int fsize = file.tellg();

    file.seekg(std::ios::beg);
    IMG_header head;

    file.read((char*)&head, sizeof(head));
    
    std::array<char, 4> sig{ head.sig[0], head.sig[1], head.sig[2], head.sig[3] };
    
    if (sig != img_signatures[2])
        return img_status::bad_signature;

    return extract_im3_img(file, filename, head.im3_file_count, fsize, mkdir) | img_status::IM3;
}
int extract_img(const char* filename, bool mkdir = true)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    int asize = file.tellg();

    file.seekg(std::ios::beg);

    IMG_header head;
    file.read((char*)&head, sizeof(head));

    std::array<char, 4> sig{ head.sig[0], head.sig[1], head.sig[2], head.sig[3] };

    // IMG is same as IM2?
    if (sig == img_signatures[0]) {
        return extract_im2_img(file, filename, head.im2_file_count, asize, mkdir) | img_status::IMG;
    }
    if (sig == img_signatures[1]) {
        return extract_im2_img(file, filename, head.im2_file_count, asize, mkdir) | img_status::IM2;
    }
    if (sig == img_signatures[2]) {
        return extract_im3_img(file, filename, head.im3_file_count, asize, mkdir) | img_status::IM3;
    }

    return img_status::bad_signature;
}