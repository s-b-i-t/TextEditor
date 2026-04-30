#include "Model.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <string>


Model::Model()
{
    EnsureNonEmpty();
}

static void SetErr(std::string *errMsg, const std::string &msg)
{
    if (errMsg)
        *errMsg = msg;
}

bool Model::LoadFromFile(const std::string &filename, std::string *errMsg)
{
    lines.clear();

    std::fstream file(filename);
    if (!file.is_open())
    {
        // Treat missing/unopenable files as an empty new buffer, but report the reason.
        SetErr(errMsg, std::string("Could not open file: ") + std::strerror(errno));
        EnsureNonEmpty();
        SetCursor(0, 0);
        return false;
    }

    std::string line;
    while (std::getline(file, line))
        lines.push_back(NormalizeLine(line));

    EnsureNonEmpty();
    SetCursor(0, 0);
    return true;
}

bool Model::SaveToFile(const std::string &filename, std::string *errMsg) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        SetErr(errMsg, std::string("Could not save file: ") + std::strerror(errno));
        return false;
    }

    for (const auto &line : lines)
        file << line << '\n';

    return true;
}

void Model::EnsureNonEmpty()
{
    if (lines.empty())
        lines.push_back("");

    cursorY = std::clamp(cursorY, 0, (int)lines.size() - 1);
    cursorX = std::clamp(cursorX, 0, (int)lines[cursorY].size());
}

void Model::SetCursor(int y, int x)
{
    EnsureNonEmpty();
    cursorY = std::clamp(y, 0, (int)lines.size() - 1);
    cursorX = std::clamp(x, 0, (int)lines[cursorY].size());
}

void Model::InsertChar(char ch)
{
    EnsureNonEmpty();
    std::string &row = lines[cursorY];
    cursorX = std::clamp(cursorX, 0, (int)row.size());
    row.insert(row.begin() + cursorX, ch);
    cursorX++;
}

void Model::InsertNewline()
{
    EnsureNonEmpty();
    std::string &row = lines[cursorY];
    cursorX = std::clamp(cursorX, 0, (int)row.size());

    const std::string right = row.substr((size_t)cursorX);
    row.erase((size_t)cursorX);
    lines.insert(lines.begin() + cursorY + 1, right);

    cursorY++;
    cursorX = 0;
}

void Model::Backspace()
{
    EnsureNonEmpty();
    std::string &row = lines[cursorY];
    cursorX = std::clamp(cursorX, 0, (int)row.size());

    if (cursorX > 0)
    {
        row.erase((size_t)(cursorX - 1), 1);
        cursorX--;
        return;
    }

    if (cursorY == 0)
        return;

    const int prevY = cursorY - 1;
    const int prevLen = (int)lines[prevY].size();
    lines[prevY] += row;
    lines.erase(lines.begin() + cursorY);

    cursorY = prevY;
    cursorX = prevLen;
}

void Model::Delete()
{
    EnsureNonEmpty();
    std::string &row = lines[cursorY];
    cursorX = std::clamp(cursorX, 0, (int)row.size());

    if (cursorX < (int)row.size())
    {
        row.erase((size_t)cursorX, 1);
        return;
    }

    // At end-of-line: join with next line.
    if (cursorY >= (int)lines.size() - 1)
        return;

    row += lines[cursorY + 1];
    lines.erase(lines.begin() + cursorY + 1);
}

std::string Model::NormalizeLine(const std::string &in)
{
    // Keep rendering/cursor logic simple: render tabs as spaces and replace control chars.
    // (This editor assumes ASCII-ish text.)
    std::string out;
    out.reserve(in.size());

    for (unsigned char uc : in)
    {
        const char c = (char)uc;
        if (c == '\t')
        {
            out.append(4, ' ');
        }
        else if (uc < 32 || uc == 127)
        {
            out.push_back('?');
        }
        else
        {
            out.push_back(c);
        }
    }

    return out;
}
