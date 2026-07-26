#include "canvas_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

CanvasWidget::CanvasWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("CanvasWidgetContainer");
    setStyleSheet(
        "QFrame#CanvasWidgetContainer {"
        "    background-color: #ffffff;"
        "    border: 1px solid #dcdfe6;"
        "    border-radius: 12px;"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(0);

    // Core drawing engine
    m_canvas = new Canvas(this);
    m_canvas->setStyleSheet("QGraphicsView { border: none; background-color: #fafbfc; border-radius: 10px; }");
    mainLayout->addWidget(m_canvas);

    // Floating toolbar/overlay container at bottom
    QWidget* overlayBar = new QWidget(this);
    QHBoxLayout* overlayLayout = new QHBoxLayout(overlayBar);
    overlayLayout->setContentsMargins(15, 6, 15, 8);
    overlayLayout->setSpacing(12);

    m_coordsLabel = new QLabel("X: 0, Y: 0", overlayBar);
    m_coordsLabel->setObjectName(QStringLiteral("CanvasCoordinatesLabel"));
    m_coordsLabel->setAccessibleName(QStringLiteral("画布坐标"));
    m_coordsLabel->setStyleSheet("font-size: 12px; color: #606266; font-family: monospace;");
    overlayLayout->addWidget(m_coordsLabel);
    overlayLayout->addStretch();

    // Zoom controls capsule
    QFrame* zoomCapsule = new QFrame(overlayBar);
    zoomCapsule->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e4e7ed;"
        "    border-radius: 6px;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    color: #409eff;"
        "    width: 28px;"
        "    height: 24px;"
        "}"
        "QPushButton:hover { background-color: #f2f6fc; }"
    );

    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomCapsule);
    zoomLayout->setContentsMargins(2, 2, 2, 2);
    zoomLayout->setSpacing(4);

    QPushButton* btnZoomOut = new QPushButton("－", zoomCapsule);
    btnZoomOut->setObjectName(QStringLiteral("CanvasZoomOutButton"));
    btnZoomOut->setToolTip(QStringLiteral("缩小画布"));
    btnZoomOut->setAccessibleName(QStringLiteral("缩小画布"));
    m_zoomLabel = new QLabel("100%", zoomCapsule);
    m_zoomLabel->setObjectName(QStringLiteral("CanvasZoomLabel"));
    m_zoomLabel->setAccessibleName(QStringLiteral("当前缩放比例"));
    m_zoomLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #303133; min-width: 42px; qproperty-alignment: AlignCenter; border: none;");
    QPushButton* btnZoomIn = new QPushButton("＋", zoomCapsule);
    btnZoomIn->setObjectName(QStringLiteral("CanvasZoomInButton"));
    btnZoomIn->setToolTip(QStringLiteral("放大画布"));
    btnZoomIn->setAccessibleName(QStringLiteral("放大画布"));
    QPushButton* btnReset = new QPushButton("重置", zoomCapsule);
    btnReset->setObjectName(QStringLiteral("CanvasZoomResetButton"));
    btnReset->setToolTip(QStringLiteral("重置为 100%"));
    btnReset->setAccessibleName(QStringLiteral("重置画布缩放"));
    btnReset->setStyleSheet("font-size: 12px; font-weight: normal; color: #606266; padding: 0 6px; border: none;");

    zoomLayout->addWidget(btnZoomOut);
    zoomLayout->addWidget(m_zoomLabel);
    zoomLayout->addWidget(btnZoomIn);
    zoomLayout->addWidget(btnReset);

    overlayLayout->addWidget(zoomCapsule);
    mainLayout->addWidget(overlayBar);

    // Connections
    connect(btnZoomIn, &QPushButton::clicked, m_canvas, &Canvas::zoomIn);
    connect(btnZoomOut, &QPushButton::clicked, m_canvas, &Canvas::zoomOut);
    connect(btnReset, &QPushButton::clicked, m_canvas, &Canvas::resetZoom);
    connect(m_canvas, &Canvas::zoomScaleChanged, this, &CanvasWidget::updateZoomLabel);

    connect(m_canvas, &Canvas::cursorScenePositionChanged, this, [this](QPointF pos) {
        m_coordsLabel->setText(QString("X: %1, Y: %2").arg(qRound(pos.x())).arg(qRound(pos.y())));
    });
}

CanvasWidget::~CanvasWidget() = default;

void CanvasWidget::setToolFromToolbar(ToolType tool)
{
    if (!m_canvas) return;
    switch (tool) {
        case ToolType::Select:    m_canvas->setToolMode(Canvas::ToolMode::Select); break;
        case ToolType::BoxSelect: m_canvas->setToolMode(Canvas::ToolMode::BoxSelect); break;
        case ToolType::Line:      m_canvas->setToolMode(Canvas::ToolMode::CreateLine); break;
        case ToolType::Arrow:     m_canvas->setToolMode(Canvas::ToolMode::CreateArrow); break;
        case ToolType::DualArrow: m_canvas->setToolMode(Canvas::ToolMode::CreateDualArrow); break;
        case ToolType::Rectangle: m_canvas->setToolMode(Canvas::ToolMode::CreateRect); break;
        case ToolType::Circle:    m_canvas->setToolMode(Canvas::ToolMode::CreateEllipse); break;
        case ToolType::Diamond:   m_canvas->setToolMode(Canvas::ToolMode::CreateDiamond); break;
        case ToolType::Text:      m_canvas->setToolMode(Canvas::ToolMode::CreateText); break;
    }
}

void CanvasWidget::updateZoomLabel()
{
    if (!m_canvas || !m_zoomLabel) return;
    int pct = qRound(m_canvas->zoomScale() * 100);
    m_zoomLabel->setText(QString("%1%").arg(pct));
}

