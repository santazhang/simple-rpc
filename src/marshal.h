#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "utils.h"

class Marshal {
    Buffer data_;
public:

    Marshal() {}
    Marshal(const std::string& data) {
        data_.write(&data[0], data.size());
    }
    Marshal(const Marshal& m): data_(m.data_) {}

    void write_bytes(const void* p, int n) {
        data_.write(p, n);
    }

    void write_i32(i32 v) {
        write_bytes(&v, sizeof(v));
    }

    void write_i64(i64 v) {
        write_bytes(&v, sizeof(v));
    }

    void write_double(double v) {
        write_bytes(&v, sizeof(v));
    }

    void write_string(const std::string& v) {
        write_i32(v.length());
        write_bytes(&v[0], v.length());
    }

    int read_bytes(void* p, int n) {
        return data_.consume(p, n);
    }

    i32 read_i32() {
        i32 v;
        VERIFY(read_bytes(&v, sizeof(v)) == sizeof(v));
        return v;
    }

    i64 read_i64() {
        i64 v;
        VERIFY(read_bytes(&v, sizeof(v)) == sizeof(v));
        return v;
    }

    double read_double() {
        double v;
        VERIFY(read_bytes(&v, sizeof(v)) == sizeof(v));
        return v;
    }

    std::string read_string() {
        i32 n = read_i32();
        std::string s;
        s.resize(n);
        VERIFY(read_bytes(&s[0], n) == n);
        return s;
    }

    int size() const {
        return data_.size();
    }

    const Marshal& operator =(const Marshal& m) {
        if (&m != this) {
            this->data_ = m.data_;
        }
        return *this;
    }
};

inline Marshal& operator <<(Marshal& m, i32 v) {
    m.write_i32(v);
    return m;
}

inline Marshal& operator <<(Marshal& m, i64 v) {
    m.write_i64(v);
    return m;
}

inline Marshal& operator <<(Marshal& m, double v) {
    m.write_double(v);
    return m;
}

inline Marshal& operator <<(Marshal& m, const std::string& v) {
    m.write_string(v);
    return m;
}

template<class T>
inline Marshal& operator <<(Marshal& m, const std::vector<T>& v) {
    m.write_i32(v.size());
    for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); it++) {
        m << *it;
    }
    return m;
}

template<class K, class V>
inline Marshal& operator <<(Marshal& m, const std::map<K, V>& v) {
    m.write_i32(v.size());
    for (typename std::map<K, V>::const_iterator it = v.begin(); it != v.end(); it++) {
        m << it->first << it->second;
    }
    return m;
}

template<class T1, class T2>
inline Marshal& operator <<(Marshal& m, const std::pair<T1, T2>& v) {
    m << v.first << v.second;
    return m;
}

inline Marshal& operator >>(Marshal& m, i32& v) {
    v = m.read_i32();
    return m;
}

inline Marshal& operator >>(Marshal& m, i64& v) {
    v = m.read_i64();
    return m;
}

inline Marshal& operator >>(Marshal& m, double& v) {
    v = m.read_double();
    return m;
}

inline Marshal& operator >>(Marshal& m, std::string& v) {
    v = m.read_string();
    return m;
}

template<class T>
inline Marshal& operator >>(Marshal& m, std::vector<T>& v) {
    int n = m.read_i32();
    while (n > 0) {
        T t;
        m >> t;
        v.push_back(t);
        n--;
    }
    return m;
}

template<class K, class V>
inline Marshal& operator >>(Marshal& m, std::map<K, V>& v) {
    int n = m.read_i32();
    while (n > 0) {
        K key;
        V val;
        m >> key >> val;
        v[key] = val;
        n--;
    }
    return m;
}

template<class T1, class T2>
inline Marshal& operator >>(Marshal& m, std::pair<T1, T2>& v) {
    m >> v.first >> v.second;
    return m;
}
