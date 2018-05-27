#include "robothelper.h"

RobotHelper::RobotHelper()
{
    this->keymap = {
        {' ', "{SPACE}"},
        {'a', "{A}"},
        {'b', "{B}"},
        {'c', "{C}"},
        {'d', "{D}"},
        {'e', "{E}"},
        {'f', "{F}"},
        {'g', "{G}"},
        {'h', "{H}"},
        {'i', "{I}"},
        {'j', "{J}"},
        {'k', "{K}"},
        {'l', "{L}"},
        {'m', "{M}"},
        {'n', "{N}"},
        {'o', "{O}"},
        {'p', "{P}"},
        {'q', "{Q}"},
        {'r', "{R}"},
        {'s', "{S}"},
        {'t', "{T}"},
        {'u', "{U}"},
        {'v', "{V}"},
        {'w', "{W}"},
        {'x', "{X}"},
        {'y', "{Y}"},
        {'z', "{Z}"},
        {'A', "{A}"},
        {'B', "{B}"},
        {'C', "{C}"},
        {'D', "{D}"},
        {'E', "{E}"},
        {'F', "{F}"},
        {'G', "{G}"},
        {'H', "{H}"},
        {'I', "{I}"},
        {'J', "{J}"},
        {'K', "{K}"},
        {'L', "{L}"},
        {'M', "{M}"},
        {'N', "{N}"},
        {'O', "{O}"},
        {'P', "{P}"},
        {'Q', "{Q}"},
        {'R', "{R}"},
        {'S', "{S}"},
        {'T', "{T}"},
        {'U', "{U}"},
        {'V', "{V}"},
        {'W', "{W}"},
        {'X', "{X}"},
        {'Y', "{Y}"},
        {'Z', "{Z}"},
        {'0', "{0}"},
        {'1', "{1}"},
        {'2', "{2}"},
        {'3', "{3}"},
        {'4', "{4}"},
        {'5', "{5}"},
        {'6', "{6}"},
        {'7', "{7}"},
        {'8', "{8}"},
        {'9', "{9}"},
        {'`', "{`}"},
        {'~', "+{`}"},
        {'-', "{-}"},
        {'_', "+{-}"},
        {'=', "{=}"},
        {'+', "+{=}"},
        {'/', "{/}"},
        {'?', "+{/}"},
        {'\\', "{\\}"},
        {'|', "+{\\}"},
        {'.', "{.}"},
        {'>', "+{.}"},
        {',', "{,}"},
        {'<', "+{,}"},
        {'[', "{[}"},
        {'{', "+{[}"},
        {']', "{]}"},
        {'}', "+{]}"},
        {'\'', "{'}"},
        {'"', "+{'}"},
        {';', "{;}"},
        {':', "+{;}"},
        {'!', "+{1}"},
        {'@', "+{2}"},
        {'#', "+{3}"},
        {'$', "+{4}"},
        {'%', "+{5}"},
        {'^', "+{6}"},
        {'&', "+{7}"},
        {'*', "+{8}"},
        {'(', "+{9}"},
        {')', "+{0}"},
        {'\n', "{RETURN}"}
    };
}

string RobotHelper::convert(string value)
{
    string result = "";

    for (int i = 0; i < value.length(); i++)
    {
        char c = value[i];
        auto search = keymap.find(c);
        if (search == keymap.end())
        {
            //strange symbol, skipping
            continue;
        } else
        {
            if (c >= 'A' && c <= 'Z')
            {
                //assuming CapsLock is off.
                result += "+";
            }
            result += search->second;
        }
    }

    return result;
}
