#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H

#include <QString>
#include "diagram_document.h"

class DocumentParser {
public:
    static LoadResult parse(const QString& html, DiagramDocument& doc);
};

#endif // DOCUMENT_PARSER_H
