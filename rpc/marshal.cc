#include "marshal.h"

using namespace std;

namespace rpc {

/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const int Chunk::min_size = 8192;

Marshal::~Marshal() {
    for (list<Chunk*>::iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        delete *it;
    }
}

Marshal::Bookmark* Marshal::set_bookmark(int size) {
    verify(write_counter_ == 0);

    // invariant: head of chunk list is not fully read (otherwise it's a waste of memory)
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    Bookmark* bmark = new Bookmark();
    bmark->size_ = size;
    bmark->ptr_ = new char*[bmark->size_];

    for (int i = 0; i < bmark->size_; i++) {
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            chunk_.push_back(new Chunk);
        }
        bmark->ptr_[i] = chunk_.back()->set_bookmark();
    }

    return bmark;
}

void Marshal::write_bookmark(Bookmark* bmark, const void* ptr) {
    char* pc = (char *) ptr;
    verify(bmark != NULL && bmark->ptr_ != NULL && bmark->size_ >= 0);
    for (int i = 0; i < bmark->size_; i++) {
        *(bmark->ptr_[i]) = pc[i];
    }
}

int Marshal::write(const void* p, int n) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    if (chunk_.empty() || chunk_.back()->fully_written()) {
        chunk_.push_back(new Chunk(p, n));
    } else {
        int n_write = chunk_.back()->write(p, n);

        // otherwise the above fully_written() will return true
        assert(n_write > 0);

        if (n_write < n) {
            const char* pc = (const char *) p;
            chunk_.push_back(new Chunk(pc + n_write, n - n_write));
        }
    }

    write_counter_ += n;
    return n;
}

int Marshal::read(void* p, int n) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    char* pc = (char *) p;
    int n_read = 0;
    while (!chunk_.empty() && n_read < n) {
        int r = chunk_.front()->read(pc + n_read, n - n_read);
        if (chunk_.front()->fully_read()) {
            // remove fully read chunks, avoid unnecessary mem usage
            delete chunk_.front();
            chunk_.pop_front();
        }
        if (r == 0) {
            // currently there's no content for us to read, so stop.
            break;
        }
        n_read += r;
    }

    verify(n_read <= n);
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_read;
}

int Marshal::peek(void* p, int n) const {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    char* pc = (char *) p;
    int n_peek = 0;

    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        int r = (*it)->peek(pc + n_peek, n - n_peek);
        if (r == 0) {
            // no more data to peek, so stop
            break;
        }
        n_peek += r;
        if (n_peek == n) {
            // read enough data, so stop
            break;
        }
    }

    assert(n_peek <= n);
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_peek;
}

int Marshal::write_to_fd(int fd) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    int n_write = 0;
    while (!chunk_.empty()) {
        int r = chunk_.front()->write_to_fd(fd);
        if (chunk_.front()->fully_read()) {
            // remove useless chunks when they are fully read
            delete chunk_.front();
            chunk_.pop_front();
        }
        if (r <= 0) {
            break;
        }
        n_write += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_write;
}

string Marshal::dump() const {
    string s;
    s.reserve(this->content_size());
    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        s += string((*it)->content_ptr(), (*it)->content_size());
    }
    assert(s.length() == this->content_size());
    return s;
}

int Marshal::read_from_marshal(Marshal& m, int n) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());
    assert(m.chunk_.empty() || !m.chunk_.front()->fully_read());

    int n_read = 0;
    while (n_read < n) {
        if (m.chunk_.empty()) {
            // nothing more to read
            break;
        }
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            int head_size = m.chunk_.front()->content_size();
            if (head_size < n - n_read) {

                // speed up: directly transfer chunk pointer, avoid memory copying
                chunk_.push_back(m.chunk_.front());
                m.chunk_.pop_front();
                n_read += head_size;

                // skip read_from_chunk operations
                continue;

            } else {
                chunk_.push_back(new Chunk);
            }
        }
        int r = chunk_.back()->read_from_chunk(*m.chunk_.front(), n - n_read);

        if (m.chunk_.front()->fully_read()) {
            // remove useless chunks when they are fully read
            delete m.chunk_.front();
            m.chunk_.pop_front();
        }
        if (r == 0) {
            // no more data to read
            break;
        }
        n_read += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());
    assert(m.chunk_.empty() || !m.chunk_.front()->fully_read());

    return n_read;
}

int Marshal::read_from_fd(int fd) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    int n_read = 0;
    for (;;) {
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            chunk_.push_back(new Chunk);
        }
        int r = chunk_.back()->read_from_fd(fd);
        if (r <= 0) {
            break;
        }
        n_read += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_read;
}

bool Marshal::content_size_gt(int size) const {

    int size_visited = 0;
    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        size_visited += (*it)->content_size();
        if (size_visited > size) {
            return true;
        }
    }

    return size_visited > size;
}

int Marshal::content_size() const {
    int size = 0;
    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        size += (*it)->content_size();
    }
    return size;
}

}
