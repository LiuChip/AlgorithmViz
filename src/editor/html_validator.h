#ifndef HTML_VALIDATOR_H
#define HTML_VALIDATOR_H

#include <QString>
#include <QList>
#include "editor/diagnostics_widget.h"

class HtmlValidator
{
public:
    static QList<DiagnosticMessage> validateHtmlCode(const QString &html);
};

#endif // HTML_VALIDATOR_H
