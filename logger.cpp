#include "logger.h"

#include <iostream>

Logger::~Logger()
{
	flush();
	delete buffer_;
}

Logger::Logger(const std::string &path, const std::string &prefix,
		size_t buf_lines)
	: path_(path), prefix_(prefix), buf_lines_(buf_lines),
		buffer_(new std::vector<std::string>()), base_num_(0)
{
	std::lock_guard<std::mutex> flk(fm_);

	fclose(fopen(path_.c_str(), "w"));
}

void Logger::log(const std::string &s)
{
	std::vector<std::string> *to_write=nullptr;
	size_t old_basenum;
	{
		std::lock_guard<std::mutex> lk(m_);

		buffer_->push_back(s);

		if (buffer_->size()>=buf_lines_) {
			to_write=buffer_;
			buffer_=new std::vector<std::string>();
			old_basenum=base_num_;
			base_num_+=to_write->size();
		}
	}

	if (!to_write) {
		return;
	}

	write(to_write, old_basenum);
}

void Logger::flush()
{
	std::vector<std::string> *to_write=nullptr;
	size_t old_basenum;
	{
		std::lock_guard<std::mutex> lk(m_);

		to_write=buffer_;
		buffer_=new std::vector<std::string>();
		old_basenum=base_num_;
		base_num_+=to_write->size();
	}

	write(to_write, old_basenum);
}

void Logger::write(std::vector<std::string> *to_write, size_t basenum)
{
	std::lock_guard<std::mutex> flk(fm_);

	FILE *f=fopen(path_.c_str(), "a");
	for (auto s : *to_write) {
		fprintf(f, "%s:%d\t%s\n", prefix_.c_str(), (int)basenum++,
			s.c_str());
	}
	fclose(f);

	delete to_write;
}

