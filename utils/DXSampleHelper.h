#pragma once

#include "comdef.h"

#include <exception>
#include <string>

class ComError : std::exception
{
public:
    ComError(std::wstring error)
        : _error(std::move(error))
    {
    }

    const std::wstring& Error() const
    {
        return _error;
    }

private:
    std::wstring _error;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;

    _com_error err(hr);
    throw ComError(err.ErrorMessage());
}
