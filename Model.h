#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>

// Model: owns the document state (lines + cursor) and provides primitive edit operations.
// No Observer pattern: Controller drives the view refresh explicitly.
class Model
{
public:
    Model();

    bool LoadFromFile(const std::string &filename, std::string *errMsg = nullptr);
    bool SaveToFile(const std::string &filename, std::string *errMsg = nullptr) const;

    const std::vector<std::string> &GetLines() const { return lines; }

    int GetCursorX() const { return cursorX; } // doc column
    int GetCursorY() const { return cursorY; } // doc row
    void SetCursor(int y, int x);

    // Editing primitives (all operate at the current cursor).
    void InsertChar(char ch);
    void InsertNewline();
    void Backspace();
    void Delete();

    void EnsureNonEmpty();

private:
    static std::string NormalizeLine(const std::string &in);

    std::vector<std::string> lines;
    int cursorX = 0;
    int cursorY = 0;
};

#endif
