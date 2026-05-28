#include "core/Hotkey.h"

#include <QSet>

namespace deepswitch {

static QString normalizeToken(const QString& token)
{
    const QString t = token.trimmed().toLower();
    if (t == "control") {
        return "Ctrl";
    }
    if (t == "ctrl") {
        return "Ctrl";
    }
    if (t == "alt") {
        return "Alt";
    }
    if (t == "shift") {
        return "Shift";
    }
    if (t == "super" || t == "meta" || t == "win") {
        return "Meta";
    }
    if (t == "return" || t == "enter") {
        return "Enter";
    }
    if (t == "esc" || t == "escape") {
        return "Esc";
    }
    if (t == "space") {
        return "Space";
    }
    if (t == "tab") {
        return "Tab";
    }
    if (t == "backspace" || t == "back_space") {
        return "BackSpace";
    }
    if (t == "delete" || t == "del") {
        return "Delete";
    }
    if (t == "left") {
        return "Left";
    }
    if (t == "right") {
        return "Right";
    }
    if (t == "up") {
        return "Up";
    }
    if (t == "down") {
        return "Down";
    }
    if (t == "home") {
        return "Home";
    }
    if (t == "end") {
        return "End";
    }
    if (t == "pageup" || t == "page_up" || t == "prior") {
        return "Page_Up";
    }
    if (t == "pagedown" || t == "page_down" || t == "next") {
        return "Page_Down";
    }
    if (t == "insert" || t == "ins") {
        return "Insert";
    }
    if (t.size() == 1) {
        return t.toUpper();
    }
    return token.trimmed();
}

static bool isModifier(const QString& token)
{
    return token == "Ctrl" || token == "Alt" || token == "Shift" || token == "Meta";
}

Result<Hotkey> Hotkey::parse(const QString& input)
{
    const QStringList rawTokens = input.split('+', Qt::SkipEmptyParts);
    if (rawTokens.isEmpty()) {
        return Result<Hotkey>::failure("hotkey_invalid", "Hotkey is empty.");
    }

    QStringList modifiers;
    QSet<QString> seenModifiers;
    QString key;

    for (const QString& rawToken : rawTokens) {
        const QString token = normalizeToken(rawToken);
        if (token.isEmpty()) {
            return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains an empty token.");
        }

        if (isModifier(token)) {
            if (seenModifiers.contains(token)) {
                return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains a duplicate modifier.");
            }
            seenModifiers.insert(token);
            modifiers.append(token);
            continue;
        }

        if (!key.isEmpty()) {
            return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains more than one main key.");
        }
        key = token;
    }

    if (key.isEmpty()) {
        return Result<Hotkey>::failure("hotkey_invalid", "Hotkey must contain a main key.");
    }

    const QStringList modifierOrder = { "Ctrl", "Alt", "Shift", "Meta" };
    QStringList orderedModifiers;
    for (const QString& candidate : modifierOrder) {
        if (modifiers.contains(candidate)) {
            orderedModifiers.append(candidate);
        }
    }

    Hotkey hotkey;
    QStringList sequenceParts = orderedModifiers;
    sequenceParts.append(key);
    hotkey.key = key;
    hotkey.modifiers = orderedModifiers;
    hotkey.sequence = sequenceParts.join("+");
    return Result<Hotkey>::success(hotkey);
}

}
