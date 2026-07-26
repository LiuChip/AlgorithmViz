#include <QtTest>
#include "../src/core/document/diagram_document.h"
#include "../src/core/document/document_parser.h"
#include "../src/core/document/document_serializer.h"

class ParserSerializerTest : public QObject
{
    Q_OBJECT

private slots:
    void testBasicRoundTrip()
    {
        QString testHtml = 
            "<div id=\"canvas\" class=\"viz-container\">\n"
            "    <rect id=\"1\" data-name=\"Node 1\" data-layer=\"1\" fill=\"#ff0000\" stroke=\"#000000\" stroke-width=\"2\" x=\"10\" y=\"20\" width=\"100\" height=\"50\">Hello</rect>\n"
            "    <circle id=\"2\" fill=\"rgba(0, 255, 0, 0.50)\" cx=\"200\" cy=\"45\" r=\"30\"/>\n"
            "    <connector id=\"3\" start=\"10,10\" end-target=\"1\" end-fallback=\"15,25\" end-angle=\"90\" end-style=\"arrow\">Label</connector>\n"
            "</div>\n"
            "<script type=\"application/json\" id=\"algorithmviz-project-data\">\n"
            "{\n"
            "    \"canvas\": {\n"
            "        \"height\": 800,\n"
            "        \"width\": 1200,\n"
            "        \"x\": 0,\n"
            "        \"y\": 0\n"
            "    },\n"
            "    \"version\": \"1.0\"\n"
            "}\n"
            "</script>\n";

        DiagramDocument doc;
        LoadResult result = DocumentParser::parse(testHtml, doc);
        QVERIFY(result.success);
        QCOMPARE(doc.shapes.size(), 2);
        QCOMPARE(doc.connectors.size(), 1);
        QCOMPARE(doc.shapes[0].common.id, 1);
        QCOMPARE(doc.shapes[0].common.name, QString("Node 1"));
        QCOMPARE(doc.shapes[0].common.textStyle.text, QString("Hello"));
        QCOMPARE(doc.shapes[1].common.id, 2);
        QCOMPARE(doc.connectors[0].endFreePoint, QPointF(15, 25)); // Verified fallback point loaded!

        QString serialized = DocumentSerializer::serialize(doc);
        QVERIFY(!serialized.isEmpty());
        QVERIFY(serialized.contains(QStringLiteral("id=\"algorithmviz-project-data\"")));
        QVERIFY(!serialized.contains(QStringLiteral("id=\"algorithmviz-data\"")));
        QVERIFY(serialized.contains("end-fallback=\"15,25\""));
        QVERIFY(serialized.contains("Node 1"));

        // Round-trip parse
        DiagramDocument roundTripDoc;
        LoadResult res2 = DocumentParser::parse(serialized, roundTripDoc);
        QVERIFY(res2.success);
        QCOMPARE(roundTripDoc.shapes.size(), doc.shapes.size());
        QCOMPARE(roundTripDoc.connectors.size(), doc.connectors.size());
        QCOMPARE(roundTripDoc.connectors[0].endFreePoint, QPointF(15, 25));
    }

    void testParseTwiceDoesNotAppend()
    {
        QString testHtml = 
            "<div id=\"canvas\" class=\"viz-container\">\n"
            "    <rect id=\"1\" x=\"10\" y=\"20\" width=\"100\" height=\"50\"/>\n"
            "</div>\n"
            "<script type=\"application/json\" id=\"algorithmviz-project-data\">{\"version\": \"1.0\"}</script>\n";

        DiagramDocument doc;
        LoadResult r1 = DocumentParser::parse(testHtml, doc);
        QVERIFY(r1.success);
        QCOMPARE(doc.shapes.size(), 1);

        // Second time on the same document object
        LoadResult r2 = DocumentParser::parse(testHtml, doc);
        QVERIFY(r2.success);
        QCOMPARE(doc.shapes.size(), 1); // Should remain 1 due to transactional move, not duplicate to 2!
    }

    void testMalformedJsonRejected()
    {
        QString badHtml = 
            "<div id=\"canvas\" class=\"viz-container\">\n"
            "    <rect id=\"1\" x=\"10\" y=\"20\" width=\"100\" height=\"50\"/>\n"
            "</div>\n"
            "<script type=\"application/json\" id=\"algorithmviz-project-data\">{bad json syntax}</script>\n";

        DiagramDocument doc;
        // Pre-populate doc with a shape to verify it isn't corrupted by a failed parse
        ShapeData s;
        s.common.id = 999;
        doc.shapes.append(s);

        LoadResult result = DocumentParser::parse(badHtml, doc);
        QVERIFY(!result.success);
        QVERIFY(result.diagnostics.size() > 0);
        QCOMPARE(doc.shapes.size(), 1);
        QCOMPARE(doc.shapes[0].common.id, 999); // Unchanged! Atomic transaction worked!
    }

    void testDiagnosticsContainSourceLineNumbers()
    {
        const QString malformedXml =
            QStringLiteral("<div id=\"canvas\">\n"
                           "  <rect id=\"1\" x=\"0\" y=\"0\" width=\"10\" height=\"10\">\n"
                           "</div>\n");

        DiagramDocument xmlDocument;
        const LoadResult xmlResult = DocumentParser::parse(malformedXml, xmlDocument);
        QVERIFY(!xmlResult.success);
        QVERIFY(!xmlResult.diagnostics.isEmpty());
        QCOMPARE(xmlResult.diagnostics.constLast().lineNumber, 3);

        const QString malformedJson =
            QStringLiteral("<div id=\"canvas\"/>\n"
                           "<script type=\"application/json\" id=\"algorithmviz-project-data\">\n"
                           "{\n"
                           "  \"version\": \"1.0\",\n"
                           "  broken\n"
                           "}\n"
                           "</script>\n");

        DiagramDocument jsonDocument;
        const LoadResult jsonResult = DocumentParser::parse(malformedJson, jsonDocument);
        QVERIFY(!jsonResult.success);
        QVERIFY(!jsonResult.diagnostics.isEmpty());
        QCOMPARE(jsonResult.diagnostics.constFirst().lineNumber, 5);
    }

    void testMetadataTagIsFlexibleAndLegacyCompatible()
    {
        const QString canonical = QStringLiteral(
            "<div id=\"canvas\"><rect id=\"10\" x=\"1\" y=\"2\" width=\"3\" height=\"4\"/></div>\n"
            "<SCRIPT id='algorithmviz-project-data' data-extra='kept' type='application/json'>"
            "{\"version\":\"1.0\"}</SCRIPT>\n");
        DiagramDocument canonicalDocument;
        const LoadResult canonicalResult = DocumentParser::parse(canonical, canonicalDocument);
        QVERIFY(canonicalResult.success);
        QCOMPARE(canonicalDocument.shapes.size(), 1);

        const QString legacy = QStringLiteral(
            "<div id=\"canvas\"/>\n"
            "<script id=\"algorithmviz-data\" type=\"application/json\">"
            "{\"version\":\"1.0\"}</script>\n");
        DiagramDocument legacyDocument;
        const LoadResult legacyResult = DocumentParser::parse(legacy, legacyDocument);
        QVERIFY(legacyResult.success);
    }

    void testMetadataRemovalPreservesXmlSourceLines()
    {
        const QString malformedXml = QStringLiteral(
            "<script id=\"algorithmviz-project-data\" type=\"application/json\">\n"
            "{\"version\":\"1.0\"}\n"
            "</script>\n"
            "<div id=\"canvas\">\n"
            "  <rect id=\"1\" x=\"0\" y=\"0\" width=\"10\" height=\"10\">\n"
            "</div>\n");

        DiagramDocument document;
        const LoadResult result = DocumentParser::parse(malformedXml, document);
        QVERIFY(!result.success);
        QVERIFY(!result.diagnostics.isEmpty());
        QCOMPARE(result.diagnostics.constLast().lineNumber, 6);
    }

    void testUnterminatedMetadataIsRejected()
    {
        const QString html = QStringLiteral(
            "<div id=\"canvas\"/>\n"
            "<script type=\"application/json\" id=\"algorithmviz-project-data\">\n"
            "{\"version\":\"1.0\"}\n");
        DiagramDocument document;
        const LoadResult result = DocumentParser::parse(html, document);
        QVERIFY(!result.success);
        QVERIFY(!result.diagnostics.isEmpty());
        QCOMPARE(result.diagnostics.constFirst().lineNumber, 2);
    }

};

QTEST_MAIN(ParserSerializerTest)
#include "parser_serializer_test.moc"
