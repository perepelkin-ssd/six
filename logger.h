#pragma once

#include <mutex>
#include <string>
#include <vector>

class Logger
{
public:
	~Logger();
	Logger(const std::string &path, const std::string &prefix="",
		size_t buf_lines=10);

	void log(const std::string &);

	void flush();
private:
	std::mutex m_;
	std::mutex fm_;
	std::string path_;
	std::string prefix_;
	size_t buf_lines_;
	std::vector<std::string> *buffer_;
	size_t base_num_;
	
	void write(std::vector<std::string> *, size_t);
};
