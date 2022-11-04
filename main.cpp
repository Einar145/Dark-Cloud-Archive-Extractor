#include <thread>
#include <chrono>
#include "archive.h"
#include "img.h"

#define SLEEP_TIME 2
#define WAIT_EXIT(ec) \
std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME)); \
exit(ec)

void condition_fail_exit(bool condition, const char* msg)
{
	if (condition)
	{
		std::cout << msg << "\n";
		WAIT_EXIT(1);
	}
}

int main(int argc, char** argv)
{

	// thanks to c++20 endian enum class we can check endianness at compile time
#if __cplusplus >= 202002L
    #include <bit>
	// if you try to compile with big endian processor
	static_assert(std::endian::native == std::endian::little, "program works only on little endian systems");
#endif

	condition_fail_exit(argc != 2, "invalid number of arguments...");
	fs::path filepath(argv[1]);
	condition_fail_exit(!fs::exists(filepath), "file does not exist...");
	condition_fail_exit(fs::is_empty(filepath), "file is empty...");

	std::string path = filepath.string();
	std::ifstream file(path, std::ios::binary);
	
	int im = extract_img(path.data());
	bool im_err = img_status::has_error(im);
	if (!im_err)
	{
		std::cout << "extracted img(" << img_status::type_string(im) << ") \"" << filepath.filename().string() << "\"\n";
		WAIT_EXIT(0);
	}

	int arch = extract_archive(path.data());
	bool arch_err = arch_error::has_error(arch);
	if (!arch_err)
	{
		std::cout << "extracted archive \"" << filepath.filename().string() << "\"\n";
		WAIT_EXIT(0);
	}

	if (im_err && arch_err)
	{
		std::cout << "could not extract... img[" << img_status::error_string(im) << "][" << img_status::type_string(im) << "] ";
		std::cout << "arch[" << arch_error::to_string(arch) << "]\n";
		WAIT_EXIT(1);
	}
}