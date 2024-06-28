
class RingBuffer {
    construct new(size, value) {
        _size = size
        _buffer = []
        for(i in 0..._size) _buffer.add(value)
        _index = 0
    }

    push(value) {
        _buffer[_index] = value
        _index = (_index + 1) % _size
    }

    peek() { _buffer[_index - 1] }

    [index] {
        return _buffer[(_index + index) % _size]
    }

    size { _size }

    toString { _buffer.toString }
}