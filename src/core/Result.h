#pragma once

#include <QString>
#include <utility>

namespace deepswitch {

template <typename T>
struct Result {
    bool ok = false;
    T value {};
    QString errorCode;
    QString message;

    static Result success(T resultValue)
    {
        Result result;
        result.ok = true;
        result.value = std::move(resultValue);
        return result;
    }

    static Result failure(QString code, QString failureMessage)
    {
        Result result;
        result.ok = false;
        result.errorCode = std::move(code);
        result.message = std::move(failureMessage);
        return result;
    }
};

struct VoidResult {
    bool ok = false;
    QString errorCode;
    QString message;

    static VoidResult success()
    {
        return { true, {}, {} };
    }

    static VoidResult failure(QString code, QString failureMessage)
    {
        return { false, std::move(code), std::move(failureMessage) };
    }
};

}
