#pragma once
#include <cstdint>
#include <filesystem>
namespace fs = std::filesystem;

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

#include <array>
#include <fstream>

// template<int N>
// std::array<char, N> get_file_signature(const char* filepath)
// {
// 	std::ifstream file(filepath, std::ios::binary);
// 	std::array<char, N> sig;
// 	file.read(sig.data(), sig.size());
// 	return sig;
// }

// template<int N>
// std::array<char, N> get_file_signature(std::ifstream& file)
// {
// 	int savep = file.tellg();
// 	std::array<char, N> sig;
// 	file.seekg(std::ios::beg);
// 	file.read(sig.data(), sig.size());
// 	file.seekg(savep);
// 	return sig;
// }
void write_file(const std::string& path, const std::string& name, std::unique_ptr<u8[]> data, int size)
{
	if(!path.empty() && !fs::is_directory(path))
		fs::create_directory(path);
	std::ofstream outfile(path + "/" + name, std::ios::binary);
	outfile.write((char*)data.get(), size);
}