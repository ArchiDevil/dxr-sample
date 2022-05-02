#pragma once

template<typename T, size_t N>
constexpr size_t Countof(T (&arr)[N])
{
    return N;
}
