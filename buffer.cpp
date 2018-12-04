#include "buffer.h"

Buffer::~Buffer()
{
    if (cb_) {
        cb_();
    }
}

Buffer::Buffer(void *ptr, size_t size, bool copy, Buffer::DeleteCb cb)
{
    if (copy) {
        data_=operator new(size);
        size_=size;
        memcpy(data_, ptr, size);
        if (cb) {
            cb_=cb;
        } else {
            cb_=[this]() {
                operator delete(getData());
            };
        }
    } else {
        data_=ptr;
        size_=size;
        cb_=cb;
    }
}

Buffer::DeleteCb Buffer::setDeleteCb(Buffer::DeleteCb cb)
{
    auto res=cb_;
    cb_=cb;
    return res;
}

const void *Buffer::getData() const
{
    return data_;
}

void *Buffer::getData()
{
    return data_;
}

size_t Buffer::getSize() const
{
    return size_;
}

std::tuple<std::string, BufferPtr> Buffer::extractString(const BufferPtr &buf)
{
    auto pchar=static_cast<const char*>(buf->data_);

    size_t len=strnlen(pchar, buf->size_);

    if (len==buf->size_) {
        throw std::invalid_argument("Cannot extract string: buffer does not contain '\0' char");
    }

    auto rest=createSubBuffer(buf, len+1);

    std::string val(pchar);
    return std::make_tuple(val, rest);
}

BufferPtr Buffer::createSubBuffer(const BufferPtr &buf, size_t offset, size_t length)
{
    if (length==std::string::npos) {
        length=buf->size_-offset;
    }

    if (offset > buf->size_ || offset+length > buf->size_) {
        throw std::invalid_argument("Buffer too small");
    }

    void *new_ptr=static_cast<char*>(buf->data_)+offset;

    return BufferPtr(new Buffer(new_ptr, length, false, [buf]() {
        // do nothing, just keep buf alive
    }));
}

BufferPtr Buffer::create(const std::string &str)
{
    BufferPtr buf=BufferPtr(new Buffer(str.length()+1));
    memcpy(buf->getData(), str.c_str(), buf->getSize());
    return buf;
}

Buffer::Buffer(size_t size, Buffer::DeleteCb cb)
    : data_(operator new(size)), size_(size), cb_(cb)
{
    if (!cb_) {
        cb_=[this]() {
            operator delete(getData());
        };
    }
}

size_t size(const Buffers &buf)
{
    size_t res=0;
    for (auto b: buf) {
        res+=b->getSize();
    }
    return res;
}
