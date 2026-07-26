#ifndef PROJECT_METADATA_H
#define PROJECT_METADATA_H

#include <QString>
#include <QtGlobal>

// HTML 项目元数据块的提取结果。
// xmlContent 会用空白替换脚本内容并保留换行，从而保持后续 XML 诊断行号准确。
struct ProjectMetadataBlock {
    bool found = false;
    bool terminated = true;
    qsizetype tagStart = -1;
    qsizetype contentStart = -1;
    qsizetype blockEnd = -1;
    int tagLine = -1;
    int contentLine = -1;
    QString jsonContent;
    QString xmlContent;
};

class ProjectMetadata
{
public:
    static constexpr const char *CanonicalId = "algorithmviz-project-data";

    // 提取规范或旧版 AlgorithmViz JSON 元数据块。
    // 为向后兼容，读取时同时接受 algorithmviz-data。
    static ProjectMetadataBlock extract(const QString &html);
};

#endif // PROJECT_METADATA_H
