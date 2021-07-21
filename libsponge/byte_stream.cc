#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : size(capacity) { DUMMY_CODE(capacity); }

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    if (is_input_ended)
        return 0;
    if (data.size() + buffer.size() > size) {
        size_t written_size = size - buffer.size();
        buffer += data.substr(0, written_size);
        return written_size;
    }
    buffer += data;
    return data.size();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    size_t actual_len = (len > buffer.size()) ? buffer.size() : len;
    return buffer.substr(0, actual_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    DUMMY_CODE(len);

    size_t actual_len = (len > buffer.size()) ? buffer.size() : len;
    buffer = buffer.substr(actual_len);
    bytes_pop_cnt += actual_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    size_t actual_len = (len > buffer.size()) ? buffer.size() : len;
    auto ret = buffer.substr(0, actual_len);
    buffer = buffer.substr(actual_len);
    bytes_pop_cnt += actual_len;
    return ret;
}

void ByteStream::end_input() { is_input_ended = true; }

bool ByteStream::input_ended() const { return is_input_ended; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.empty(); }

bool ByteStream::eof() const { return is_input_ended && buffer.empty(); }

size_t ByteStream::bytes_written() const { return bytes_pop_cnt + buffer.size(); }

size_t ByteStream::bytes_read() const { return bytes_pop_cnt; }

size_t ByteStream::remaining_capacity() const { return size - buffer.size(); }
