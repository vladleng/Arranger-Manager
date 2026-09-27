#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace arranger
{
enum class Status { unmarked, todo, wip, draft, review, done, blocked };
enum class Priority { none, p0, p1, p2, p3 };

struct Tag
{
    Status status = Status::unmarked;
    Priority priority = Priority::none;
    std::string note;
    bool valid = false;
};

inline std::string trim(std::string_view text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return std::string(text.substr(first, last - first + 1));
}

inline std::string upper(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

inline Status parseStatus(std::string_view text)
{
    const auto code = upper(trim(text));
    if (code == "TODO") return Status::todo;
    if (code == "WIP") return Status::wip;
    if (code == "DRAFT") return Status::draft;
    if (code == "REVIEW") return Status::review;
    if (code == "DONE") return Status::done;
    if (code == "BLOCKED") return Status::blocked;
    return Status::unmarked;
}

inline Priority parsePriority(std::string_view text)
{
    const auto code = upper(trim(text));
    if (code == "P0") return Priority::p0;
    if (code == "P1") return Priority::p1;
    if (code == "P2") return Priority::p2;
    if (code == "P3") return Priority::p3;
    return Priority::none;
}

inline const char* label(Status status)
{
    switch (status)
    {
        case Status::todo: return "TODO";
        case Status::wip: return "WIP";
        case Status::draft: return "DRAFT";
        case Status::review: return "REVIEW";
        case Status::done: return "DONE";
        case Status::blocked: return "BLOCKED";
        default: return "UNMARKED";
    }
}

inline const char* label(Priority priority)
{
    switch (priority)
    {
        case Priority::p0: return "P0";
        case Priority::p1: return "P1";
        case Priority::p2: return "P2";
        case Priority::p3: return "P3";
        default: return "--";
    }
}

// STATUS | P1 | optional note. Legacy GROUP:STATUS remains readable.
// Unknown names are ordinary DAW events and are not counted as arrangement tags.
inline Tag parse(std::string_view name)
{
    Tag result;
    const auto pipe = name.find('|');
    if (pipe == std::string_view::npos)
    {
        result.status = parseStatus(name);
        if (result.status != Status::unmarked)
        {
            result.valid = true;
            return result;
        }

        const auto colon = name.rfind(':');
        if (colon == std::string_view::npos || colon == 0) return result;
        result.status = parseStatus(name.substr(colon + 1));
        result.valid = result.status != Status::unmarked;
        return result;
    }

    result.status = parseStatus(name.substr(0, pipe));
    if (result.status == Status::unmarked) return result;

    const auto second = name.find('|', pipe + 1);
    const auto priorityText = name.substr(pipe + 1, second == std::string_view::npos ? second : second - pipe - 1);
    result.priority = parsePriority(priorityText);
    if (result.priority == Priority::none) return result;
    if (second != std::string_view::npos) result.note = trim(name.substr(second + 1));
    result.valid = true;
    return result;
}
}
