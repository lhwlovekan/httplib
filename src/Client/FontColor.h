#pragma once
#include <iostream>
using namespace std;

// 字符串分割
static void StrSplit(string str, const char split, vector<string> &result) {
    stringstream input(str);
    string temp;
    while (getline(input, temp, split)) {
        if (!temp.empty()) {
            result.push_back(temp);
        }
    }
}

enum class FontColor {
    Black = 30,
    Red,
    Green,
    Yellow,
    Blue,
    Purple,
    Cyan,
    White,
    // 颜色变亮
    LBlack = 90,
    LRed,
    LGreen,
    LYellow,
    LBlue,
    LPurple,
    LCyan,
    LWhite
};

enum class FontStyle { Default = 0, Bold, Thin, Italic, Underline, Blink, Flash, Invert, Blackout, Prohibit };

// 输出没有换行
static void PrintColor(string content, FontColor fontcolor) {
    cout << "\033[" + to_string(static_cast<int>(fontcolor)) + "m" + content + "\033[0m";
}

// 输出带有换行
static void PrintColorLine(string content, FontColor fontcolor) {
    cout << "\033[" + to_string(static_cast<int>(fontcolor)) + "m" + content + "\n\033[0m";
}

// 重载，输出带有属性，不换行
static void PrintColor(string content, FontColor fontcolor, FontStyle fontstyle) {
    cout << "\033[" + to_string(static_cast<int>(fontstyle)) + ";" + to_string(static_cast<int>(fontcolor)) + "m" + content + "\033[0m";
}

// 重载，输出带有属性，换行
static void PrintColorLine(string content, FontColor fontcolor, FontStyle fontstyle) {
    cout << "\033[" + to_string(static_cast<int>(fontstyle)) + ";" + to_string(static_cast<int>(fontcolor)) + "m" + content + "\n\033[0m";
}

// 不输出，为字符串添加属性
static string AddColorStyle(string content, FontStyle fontstyle) {
    return "\001\033[" + to_string(static_cast<int>(fontstyle)) + "m\002" + content + "\001\033[0m\002";
}

// 不输出，为字符串添加颜色和属性
static string AddColorStyle(string content, FontColor fontcolor, FontStyle fontstyle) {
    return "\001\033[" + to_string(static_cast<int>(fontstyle)) + ";" + to_string(static_cast<int>(fontcolor)) + "m\002" + content +
           "\001\033[0m\002";
}
