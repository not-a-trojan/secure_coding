#pragma once

template<typename T>
struct WriteOnly
{
    WriteOnly(T* pointer) : m_pointer{pointer} {}

    void operator=(const T& t) { *m_pointer = t; }

private:
    T* m_pointer;
};
