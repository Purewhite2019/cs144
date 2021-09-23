#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t write_cnt = min(data.size(), _capacity - _storage.size());
    _storage += data.substr(0, write_cnt);
    return write_cnt;
}

//* \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return _storage.substr(0, len); }

//* \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    _storage = _storage.substr(len, string::npos);
    _read_cnt += len;
}

//* Read (i.e., copy and then pop) the next "len" bytes of the stream
//* \param[in] len bytes will be popped and returned
//* \returns a string
std::string ByteStream::read(const size_t len) {
    auto result = peek_output(len);
    pop_output(len);
    return result;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _storage.size(); }

bool ByteStream::buffer_empty() const { return _storage.size() == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _read_cnt + _storage.size(); }

size_t ByteStream::bytes_read() const { return _read_cnt; }

size_t ByteStream::remaining_capacity() const { return _capacity - _storage.size(); }
