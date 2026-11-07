// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cctype>
#include <limits>
#include <string_view>

// Implementation detail of ElementMap restore; not a public API.
namespace Data::detail
{
inline int digitValue(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

// Preserve the legacy strtol values for the archive's decimal/hex fields:
// leading whitespace/sign, optional hex prefix, numeric prefix and range clamp.
// No end pointer or errno is exposed; never read outside the supplied view.
inline long parseRestoreLong(std::string_view token, int base)
{
    if (base != 10 && base != 16) {
        return 0;
    }
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
        token.remove_prefix(1);
    }
    bool negative = false;
    if (!token.empty() && (token.front() == '-' || token.front() == '+')) {
        negative = token.front() == '-';
        token.remove_prefix(1);
    }
    if (base == 16 && token.size() >= 3 && token[0] == '0'
        && (token[1] == 'x' || token[1] == 'X') && digitValue(token[2]) >= 0) {
        token.remove_prefix(2);
    }

    const auto maximum = std::numeric_limits<long>::max();
    const auto minimum = std::numeric_limits<long>::min();
    const unsigned long limit = static_cast<unsigned long>(maximum) + (negative ? 1UL : 0UL);
    const auto radix = static_cast<unsigned long>(base);
    const auto cutoff = limit / radix;
    const auto remainder = limit % radix;
    unsigned long value = 0;
    for (char ch : token) {
        const int digit = digitValue(ch);
        if (digit < 0 || digit >= base) {
            break;
        }
        const auto next = static_cast<unsigned long>(digit);
        if (value > cutoff || (value == cutoff && next > remainder)) {
            return negative ? minimum : maximum;
        }
        value = value * radix + next;
    }
    if (negative) {
        // LONG_MIN's magnitude is representable unsigned, but not positive long.
        return value == limit ? minimum : -static_cast<long>(value);
    }
    return static_cast<long>(value);
}
}  // namespace Data::detail
