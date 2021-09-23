#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//* \details This function accepts a substring (aka a segment) of bytes,
//* possibly out-of-order, from the logical stream, and assembles any newly
//* contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (_storage.empty()) {
        if (index + data.size() >= _output.bytes_written()) {
            size_t begin = std::max(index, _output.bytes_written());
            size_t end = std::min(index + data.size(), _output.bytes_written() + _capacity);
            _storage.emplace(_storage.begin(), begin, std::move(data.substr(begin - index, end - begin)));
            _unassembled_bytes += end - begin;
        }
    } else {
        auto it = _storage.begin();
        if (index < it->first && index + data.size() >= _output.bytes_written()) {
            size_t begin = std::max(index, _output.bytes_written());
            size_t end = std::min(index + data.size(), it->first);
            if (end - begin != 0) {
                it = _storage.emplace(it, begin, std::move(data.substr(begin - index, end - begin)));
                _unassembled_bytes += end - begin;
            }
        }
        while (it + 1 < _storage.end()) {
            auto nit = it + 1;
            if (index < nit->first && index + data.size() >= it->first + it->second.size()) {
                size_t begin = std::max(index, it->first + it->second.size());
                size_t end = std::min(index + data.size(), nit->first);
                if (end - begin != 0) {
                    it = _storage.emplace(nit, begin, std::move(data.substr(begin - index, end - begin)));
                    _unassembled_bytes += end - begin;
                }
            }
            ++it;
        }
        if (!_eof && index + data.size() > it->first + it->second.size()) {
            size_t begin = std::max(index, it->first + it->second.size());
            size_t end = std::min(index + data.size(), _output.bytes_written() + _capacity);
            if (end - begin != 0) {
                it = _storage.emplace(_storage.end(), begin, std::move(data.substr(begin - index, end - begin)));
                _unassembled_bytes += end - begin;
            }
        }
    }

    auto it = _storage.begin();
    while (it < _storage.end() && it->first == _output.bytes_written()) {
        auto assembled_bytes = _output.write(it->second);
        if (assembled_bytes == it->second.size()) {
            auto tmp = it->second;
            it = _storage.erase(it);
            _unassembled_bytes -= assembled_bytes;
        } else if (assembled_bytes != 0) {
            it->first += assembled_bytes;
            it->second = std::move(it->second.substr(assembled_bytes, std::string::npos));
            _unassembled_bytes -= assembled_bytes;
            break;
        } else {
            break;
        }
    }

    if (eof)
        _eof = true;
    if (_eof && empty())
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
