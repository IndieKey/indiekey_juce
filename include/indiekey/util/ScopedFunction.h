//
// Created by Ruurd Adema on 27/03/2024.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

#include <cassert>
#include <functional>

namespace indiekey
{

/**
 * A simple class which executes given function upon destruction.
 * Very suitable for subscriptions which must go out of scope when the owning class gets destructed.
 */
class ScopedFunction
{
public:
    ScopedFunction() = default;

    explicit ScopedFunction (std::function<void()> onDestructionCallback) :
        mOnDestructionCallback (std::move (onDestructionCallback))
    {
    }

    ScopedFunction (const ScopedFunction& other) = delete;
    ScopedFunction (ScopedFunction&& other) noexcept { *this = std::move (other); }

    ~ScopedFunction()
    {
        if (mOnDestructionCallback)
            mOnDestructionCallback();
    }

    explicit operator bool() const { return mOnDestructionCallback != nullptr; }

    ScopedFunction& operator= (const ScopedFunction& other) = delete;

    ScopedFunction& operator= (ScopedFunction&& other) noexcept
    {
        if (mOnDestructionCallback)
            mOnDestructionCallback();

        mOnDestructionCallback = std::move (other.mOnDestructionCallback);
        other.mOnDestructionCallback = nullptr;

        // Apparently a move doesn't reset the std::function
        assert (other.mOnDestructionCallback == nullptr);

        return *this;
    }

    ScopedFunction& operator= (std::function<void()>&& onDestructionCallback) noexcept
    {
        if (mOnDestructionCallback)
            mOnDestructionCallback();

        mOnDestructionCallback = std::move (onDestructionCallback);

        return *this;
    }

    void reset()
    {
        if (mOnDestructionCallback)
        {
            mOnDestructionCallback();
            mOnDestructionCallback = nullptr;
        }
    }

    /**
     * Clears the destruction callback without invoking it.
     * Warning! This basically negates the point of this class, you should probably rarely use it, if ever.
     */
    void neutralize()
    {
        mOnDestructionCallback = nullptr;
    }

private:
    std::function<void()> mOnDestructionCallback;
};

using Defer = ScopedFunction;

}
