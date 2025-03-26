//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#pragma once

#include <string>
#include <stdexcept>

namespace indiekey
{

class License
{
public:
    enum class Type
    {
        Undefined,
        Perpetual,
        Subscription,
        Trial,
        Alpha,
        Beta,
    };

    /**
     * Determines which side of given types is more valuable.
     * @param lhs Left hand side
     * @param rhs Right hand side
     * @returns A negative value means that the left hand side is less valuable than the right hand side. A positive
     * value means that the left hand side is more valuable than the right hand side. A value of zero means that both
     * sides are equally valuable.
     */
    static int compareTypeValue (Type lhs, Type rhs)
    {
        auto getTypeValue = [] (Type type) {
            switch (type)
            {
            case Type::Perpetual:
                return 5;
            case Type::Subscription:
                return 4;
            case Type::Trial:
                return 3;
            case Type::Beta:
                return 2;
            case Type::Alpha:
                return 1;
            case Type::Undefined:
            default:
                return 0;
            }
        };

        return getTypeValue (lhs) - getTypeValue (rhs);
    }

    static Type typeFromString (const std::string& string)
    {
        if (string == "Undefined")
            return Type::Undefined;
        else if (string == "Perpetual")
            return Type::Perpetual;
        else if (string == "Trial")
            return Type::Trial;
        else if (string == "Subscription")
            return Type::Subscription;
        else if (string == "Alpha")
            return Type::Alpha;
        else if (string == "Beta")
            return Type::Beta;

        throw std::runtime_error ("Unknown license type: " + string);
    }

    static const char* typeToString (Type licenseType)
    {
        // Note: these names are used for signature verification and must match the server side.
        switch (licenseType)
        {
        case Type::Perpetual:
            return "Perpetual";
        case Type::Trial:
            return "Trial";
        case Type::Subscription:
            return "Subscription";
        case Type::Alpha:
            return "Alpha";
        case Type::Beta:
            return "Beta";
        case Type::Undefined:
            return "Undefined";
        }

        throw std::runtime_error ("Unknown license type: " + std::to_string (static_cast<int> (licenseType)));
    }
};

} // namespace indiekey
