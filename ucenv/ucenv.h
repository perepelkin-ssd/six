#pragma once

#include <cassert>
#include <stdexcept>

class DF
{
public:
	enum Type {
		Unset,
		Int,
		Real,
		String,
		Value
	};
	// Values for basic types (int, real, string) are stored as copies.
	// DF data is deleted by operator delete() unless grabbed out (in
	// that case the grabber must deallocate memory).
	// Reassignment is disabled, but it seems no bad to implement

	~DF();
	DF(const std::string &name);
	DF(const std::string &name, int value);
	DF(const std::string &name, double value);
	DF(const std::string &name, const std::string &value);
	
	// create bound to an existing buffer
	DF(const std::string &name, void *data, size_t size);

	std::string getName() const { return name_; }
	const char *getCName() const;
	size_t getSize() const;

	void setValue(int);
	void setValue(double);
	void setValue(const std::string &);

	template<class T>
	void setValue(const T& val)
	{
		if (type_!=Unset) {
			throw std::runtime_error("DF already set: "
				+ std::string(getCName()));
		}
		if (std::is_same<T, int>::value
			|| std::is_same<T, double>::value
			|| std::is_same<T, std::string>::value
		) {
			setValue(val);
		} else {
			throw std::runtime_error("Unsupported type in setValue");
		}
	}

	void *create(size_t size);
	
	template<class T>
	T *create(size_t size)
	{
		return static_cast<T *>(create(size*sizeof(T)));
	}

	template<class T>
	T *create(size_t size, const T &init_val)
	{
		T *res=create<T>(size);
		for (auto i=0u; i<size; i++) {
			res[i]=init_val;
		}
		return res;
	}

	void grabBuffer(const DF &input_df);
	void copy(const DF &);

	int getInt() const;
	double getReal() const;
	std::string getString() const;

	const void *getData() const;
	void *getData();

	template<class T>
	const T *getData() const
	{
		if (type_!=Value) {
			throw std::runtime_error("Not a value");
		}

		return static_cast<const T*>(data_);
	}

	template<class T>
	T *getData()
	{
		if (type_!=Value) {
			throw std::runtime_error("Not a value");
		}

		return static_cast<T*>(data_);
	}

	template<class T>
	T getValue() const
	{
		if (std::is_same<T, int>::value) {
			assert(type_==Int);
			return *static_cast<T*>(data_);
		} else if (std::is_same<T, double>::value) {
			assert(type_==Real);
			return *static_cast<T*>(data_);
		} else {
			throw std::runtime_error("illegal cast");
		}
	}

	bool isSet() const { return type_!=Unset; }
	Type getType() const { return type_; }

	std::tuple<void *, size_t> grabBuffer() const;

private:
	std::string name_;
	mutable void *data_;
	mutable size_t size_;
	mutable Type type_;
};

typedef const DF InputDF;
typedef DF OutputDF;

