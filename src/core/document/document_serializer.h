#ifndef DOCUMENT_SERIALIZER_H
#define DOCUMENT_SERIALIZER_H

#include <QString>
#include "diagram_document.h"

class DocumentSerializer {
public:
    static QString serialize(const DiagramDocument& doc);
};

#endif // DOCUMENT_SERIALIZER_H
