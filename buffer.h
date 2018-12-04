#ifndef BUFFER_H
#define BUFFER_H

#include <cassert>
#include <deque>
#include <functional>
#include <memory>
#include <memory.h>
#include <tuple>

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

/// An abstraction for a non-continuous memory
/// buffer to send. Buffers is a sequence of memory Buffer-s,
/// each located in a random memory area. Buffers act like a
/// smart pointer, see Buffer's description for details.
typedef std::deque<BufferPtr> Buffers;

size_t size(const Buffers &buf);

/// A continuous buffer in memory. This class is designed to handle
/// incoming and outgoing messages in an efficient way, i.e. without
/// any copying of the memory extent. The Buffer object acts as a
/// smart pointer (data deletion is presumed by default but can be
/// redefined in case of attaching to a buffer that will be deallocated
/// elsewhere; in that case user is responsible for keeping memory
/// alive during BUffer's life time).
class Buffer
{
public:
    typedef std::function<void ()> DeleteCb;

    virtual ~Buffer();

    /// Allocate size bytes of memory for new buffer. The memory is
    /// allocated with operator new(size).
    /// If cb is nullptr, a callback is generated which deallocates
    /// memory with operator delete(getData()).
    /// Otherwise caller must free the memory.
    Buffer(size_t size, DeleteCb cb=nullptr);

    /// Create a Buffer object, which is attached to the address ptr
    /// The memory at ptr must be a valid allocated memory of at least
    /// size bytes.
    /// If copy is true, new buffer is allocated, as if Buffer(size, cb)
    /// has been called and memory is copied from the original buffer.
    /// If cb is nullptr and copy is true, then a callback will be
    /// generated, which deallocates memory with operator delete(getData()).
    /// If cb is not nullptr, and copy is true, then caller must free
    /// the memory
	/// If copy is false, then nothing happens on deletion unless cb
	/// is provided
    Buffer(void* ptr, size_t size, bool copy=false, DeleteCb cb=nullptr);

	/// Create a bffer via copy (new memory extent is allocated).
	/// Same as BUffer(ptr, size, true, cb), except that it accepts
	/// const void * pointer.
	Buffer(const void *ptr, size_t size, DeleteCb cb=nullptr);

    /// Set new callback, which will be called in destructor. Previous
    /// callback is returned
    virtual DeleteCb setDeleteCb(DeleteCb cb);

    /// Get size of the buffer
    virtual size_t getSize() const;

    /// Returns: pointer to data
    virtual void *getData();

    /// Returns: const pointer to data
    virtual const void *getData() const;

    /// Interpret buffer as a type and return a reference. Buffer size must
    /// be equal to sizeof type.
    template<class T>
    T& val()
    {
        assert (size_==sizeof(T));
        return *static_cast<T*>(data_);
    }

    /// Same as val(), but const ref is returned
    template<class T>
    const T& val() const
    {
        assert (size_==sizeof(T));
        return *static_cast<T*>(data_);
    }

    /// Same as extractString, but the only return value is
    /// the string extracted. The BufferPtr is modified to be the tail.
    static std::string popString(BufferPtr &buf)
    {
        std::string res;
        std::tie(res, buf)=extractString(buf);
        return res;
    }

    /// Same as extract, but the only return value is the extracted
    /// one. The BufferPtr is modified to be the tail.
    template<class T>
    static T pop(BufferPtr &buf)
    {
        T res;
        std::tie(res, buf)=extract<T>(buf);
        return res;
    }

    /// Create a buffer of given size
    static BufferPtr create(const std::string &str);

    /// Create a buffer, containing a copy of a simple type
    /// (which must be valid to create via memcpy.
    template<class T>
    static BufferPtr create(const T& val)
    {
        BufferPtr buf(new Buffer(sizeof(val)));
        memcpy(buf->getData(), &val, sizeof(val));
        return buf;
    }

    /// Create new buffer object as a part of original buffer. The new buffer keeps a pointer
    /// to original buffer, thus preventing memory from deallocation.
    static BufferPtr createSubBuffer(const BufferPtr &buf, size_t offset, size_t length=std::string::npos);

    /// "pop_front" a string from buffer. Buffer must start with c-style '\0'-terminated
    /// string buffer. The rest of the buffer is returned as a new BufferPtr object, which
    /// is attached to the same memory extent (with different offset). The buffer created
    /// keeps a pointer to original buffer, thus preventing memory from deallocation.
    static std::tuple<std::string, BufferPtr> extractString(const BufferPtr &buf);

    /// Same as extractString, but the type extracted is a simple type specified.
    template<class T>
    static std::tuple<T, BufferPtr> extract(const BufferPtr &buf)
    {
        if(buf->size_<sizeof(T)) {
            throw std::invalid_argument("Not enough data to extract value");
        }

        BufferPtr rest=Buffer::createSubBuffer(buf, sizeof(T));

        T val=*static_cast<T*>(buf->data_);
        return std::make_tuple(val, rest);
    }

	/// Same as extractString, but an arbitrary item, which supports
	/// delerialization from buffer, is constructed
	/// The item must implement T(Buffer &buf) constructor, which
	/// deserializes itself, and modifies buf to point to the tail
	template<class T>
	static std::tuple<T, BufferPtr> extractItem(const BufferPtr &buf)
	{
		BufferPtr b(buf);
		T item(b);
		return std::tuple<T, BufferPtr>(item, b);
	}

	/// Same as extractItem, but the item is returned.
	/// The BufferPtr is modified to be the tail
	template<class T>
	static T popItem(BufferPtr &buf)
	{
		T res;
		std::tie(res, buf)=extractItem<T>(buf);
		return res;
	}

private:
    void *data_;
    size_t size_;
    DeleteCb cb_;
};

#endif // BUFFER_H
