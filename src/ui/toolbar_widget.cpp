#include "toolbar_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QFont>
#include <QtMath>
#include <QTimer>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>
#include <QDialog>
#include <QFrame>
#include <QPointer>

void drawToolIcon(QPainter& painter, const QRectF& rect, ToolType type, const QColor& iconColor)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(iconColor);
    pen.setWidthF(2.8);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    QPointF c = rect.center();

    switch (type) {
        case ToolType::Select: {
            painter.setBrush(iconColor);
            QPolygonF cursorPoly;
            cursorPoly << QPointF(c.x() - 6, c.y() - 10)
                       << QPointF(c.x() - 6, c.y() + 10)
                       << QPointF(c.x() - 1, c.y() + 4)
                       << QPointF(c.x() + 5, c.y() + 12)
                       << QPointF(c.x() + 8, c.y() + 10)
                       << QPointF(c.x() + 2, c.y() + 2)
                       << QPointF(c.x() + 9, c.y() + 2);
            painter.drawPolygon(cursorPoly);
            break;
        }
        case ToolType::BoxSelect: {
            painter.setBrush(Qt::NoBrush);
            QPen boxPen = pen;
            boxPen.setStyle(Qt::DashLine);
            boxPen.setWidthF(1.8);
            painter.setPen(boxPen);
            painter.drawRect(QRectF(c.x() - 9, c.y() - 9, 15, 14));

            QPen crossPen(iconColor);
            crossPen.setStyle(Qt::SolidLine);
            crossPen.setWidthF(2.5);
            crossPen.setCapStyle(Qt::SquareCap);
            painter.setPen(crossPen);
            // 实线小十字
            painter.drawLine(QPointF(c.x() + 3, c.y() + 7), QPointF(c.x() + 11, c.y() + 7));
            painter.drawLine(QPointF(c.x() + 7, c.y() + 3), QPointF(c.x() + 7, c.y() + 11));
            break;
        }
        case ToolType::Line: {
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPointF(c.x() - 11, c.y() + 11),
                             QPointF(c.x() + 11, c.y() - 11));
            break;
        }
        case ToolType::Arrow: {
            painter.setBrush(iconColor);
            painter.drawLine(QPointF(c.x() - 11, c.y() + 11),
                             QPointF(c.x() + 6, c.y() - 6));
            QPolygonF arrowHead;
            arrowHead << QPointF(c.x() + 12, c.y() - 12)
                      << QPointF(c.x() + 2, c.y() - 8)
                      << QPointF(c.x() + 8, c.y() - 2);
            painter.drawPolygon(arrowHead);
            break;
        }
        case ToolType::DualArrow: {
            painter.setBrush(iconColor);
            painter.drawLine(QPointF(c.x() - 6, c.y() + 6),
                             QPointF(c.x() + 6, c.y() - 6));
            QPolygonF arrowTopRight;
            arrowTopRight << QPointF(c.x() + 12, c.y() - 12)
                          << QPointF(c.x() + 2, c.y() - 8)
                          << QPointF(c.x() + 8, c.y() - 2);
            painter.drawPolygon(arrowTopRight);
            QPolygonF arrowBottomLeft;
            arrowBottomLeft << QPointF(c.x() - 12, c.y() + 12)
                            << QPointF(c.x() - 2, c.y() + 8)
                            << QPointF(c.x() - 8, c.y() + 2);
            painter.drawPolygon(arrowBottomLeft);
            break;
        }
        case ToolType::Rectangle: {
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(c.x() - 10, c.y() - 9, 20, 18));
            break;
        }
        case ToolType::Circle: {
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, 10, 10);
            break;
        }
        case ToolType::Diamond: {
            painter.setBrush(Qt::NoBrush);
            QPolygonF diamondPoly;
            diamondPoly << QPointF(c.x(), c.y() - 11)
                        << QPointF(c.x() + 11, c.y())
                        << QPointF(c.x(), c.y() + 11)
                        << QPointF(c.x() - 11, c.y());
            painter.drawPolygon(diamondPoly);
            break;
        }
        case ToolType::Text: {
            QFont f("Times New Roman", 22, QFont::Bold);
            if (!f.exactMatch()) {
                f = QFont("Times", 22, QFont::Bold);
            }
            f.setStyleHint(QFont::Serif);
            painter.setFont(f);
            painter.drawText(rect, Qt::AlignCenter, "T");
            break;
        }
    }
    painter.restore();
}

// -----------------------------------------------------------------------------
// SubtoolButton for the popup menu
// -----------------------------------------------------------------------------
class SubtoolButton : public QWidget
{
public:
    SubtoolButton(ToolType type, QWidget* parent = nullptr)
        : QWidget(parent), m_type(type)
    {
        setFixedSize(52, 52);
        const QString name = ToolBarWidget::toolName(m_type);
        setToolTip(name);
        setAccessibleName(name);
    }

    ToolType toolType() const { return m_type; }
    bool isHighlighted() const { return m_highlighted; }

    void setHighlighted(bool highlighted) {
        if (m_highlighted != highlighted) {
            m_highlighted = highlighted;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRectF boxRect = QRectF(rect()).adjusted(2, 2, -2, -2);

        if (m_highlighted) {
            painter.setBrush(QColor("#ffffff"));
            QPen pen(QColor("#009dff"), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(pen);
            painter.drawRoundedRect(boxRect, 12, 12);
        } else {
            painter.setBrush(QColor("#ffffff"));
            QPen pen(QColor("#e4e7ed"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(pen);
            painter.drawRoundedRect(boxRect, 12, 12);
        }

        QColor iconColor = m_highlighted ? QColor("#001a33") : QColor("#222222");
        drawToolIcon(painter, boxRect, m_type, iconColor);
    }

private:
    ToolType m_type;
    bool m_highlighted = false;
};

// -----------------------------------------------------------------------------
// SubtoolPopup Window (Flyout menu with light external drop shadow)
// -----------------------------------------------------------------------------
class SubtoolPopup : public QDialog
{
    Q_OBJECT
public:
    SubtoolPopup(const QList<ToolType>& subTools, ToolType currentSelected, QWidget* parent = nullptr)
        : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(false); // Non-modal so mouse grab forwarding from button works seamlessly

        QVBoxLayout* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(15, 15, 15, 15);

        QFrame* capsule = new QFrame(this);
        capsule->setObjectName("Capsule");
        capsule->setStyleSheet(
            "QFrame#Capsule {"
            "    background-color: #ffffff;"
            "    border: 1.5px solid #dcdfe6;"
            "    border-radius: 20px;"
            "}"
        );

        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(25);
        shadow->setColor(QColor(0, 0, 0, 40));
        shadow->setOffset(0, 4);
        capsule->setGraphicsEffect(shadow);

        QHBoxLayout* capLayout = new QHBoxLayout(capsule);
        capLayout->setContentsMargins(8, 8, 8, 8);
        capLayout->setSpacing(8);

        for (ToolType t : subTools) {
            SubtoolButton* btn = new SubtoolButton(t, capsule);
            if (t == currentSelected) {
                btn->setHighlighted(true);
            }
            m_buttons.append(btn);
            capLayout->addWidget(btn);
        }

        rootLayout->addWidget(capsule);
    }

    void startInteraction() {
        show();
        grabMouse();
    }

    void updateHighlightByGlobalPos(const QPoint& globPos) {
        SubtoolButton* best = nullptr;
        int minDistance = 999999;
        
        for (SubtoolButton* btn : m_buttons) {
            QPoint btnCenter = btn->mapToGlobal(btn->rect().center());
            int dist = (globPos - btnCenter).manhattanLength();
            if (dist < 150 && dist < minDistance) { // Wide intuitive magnetic snap
                minDistance = dist;
                best = btn;
            }
        }

        if (best) {
            for (SubtoolButton* btn : m_buttons) {
                btn->setHighlighted(btn == best);
            }
        }
    }

    void confirmSelectionAndClose() {
        releaseMouse();
        SubtoolButton* selectedBtn = nullptr;
        for (SubtoolButton* btn : m_buttons) {
            if (btn->isHighlighted()) {
                selectedBtn = btn;
                break;
            }
        }
        
        if (selectedBtn) {
            emit toolSelected(selectedBtn->toolType());
        }
        close();
    }

signals:
    void toolSelected(ToolType type);

protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        updateHighlightByGlobalPos(event->globalPosition().toPoint());
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        Q_UNUSED(event);
        confirmSelectionAndClose();
    }

private:
    QList<SubtoolButton*> m_buttons;
};

// -----------------------------------------------------------------------------
// ToolButton for main toolbar
// -----------------------------------------------------------------------------
class ToolButton : public QPushButton
{
    Q_OBJECT
public:
    ToolButton(ToolType type, bool hasSubtools, QWidget* parent = nullptr)
        : QPushButton(parent), m_type(type), m_hasSubtools(hasSubtools)
    {
        setCheckable(true);
        setFixedSize(50, 50);
        setCursor(Qt::PointingHandCursor);
        updateMetadata();

        m_longPressTimer = new QTimer(this);
        m_longPressTimer->setSingleShot(true);
        connect(m_longPressTimer, &QTimer::timeout, this, [this]() {
            m_longPressTriggered = true;
            emit longPressTriggered();
        });
    }

    ToolType toolType() const { return m_type; }
    void setToolType(ToolType type) {
        if (m_type != type) {
            m_type = type;
            updateMetadata();
            update();
        }
    }

    void setActivePopup(SubtoolPopup* popup) {
        if (m_activePopup && m_activePopup != popup)
            m_activePopup->close();
        // QPointer 会在 WA_DeleteOnClose 销毁弹窗后自动清空。
        m_activePopup = popup;
    }

signals:
    void longPressTriggered();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_hasSubtools) {
            m_pressPos = event->pos();
            m_longPressTriggered = false;
            m_longPressTimer->start(250); // 250ms threshold
        }
        QPushButton::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_hasSubtools && isDown()) {
            if (!m_longPressTriggered) {
                if ((event->pos() - m_pressPos).manhattanLength() > 12) {
                    m_longPressTimer->stop();
                    m_longPressTriggered = true;
                    emit longPressTriggered();
                }
            }
            if (m_longPressTriggered && m_activePopup) {
                // Forward movement while holding mouse down to popup so highlighting tracks smoothly!
                m_activePopup->updateHighlightByGlobalPos(event->globalPosition().toPoint());
                return;
            }
        }
        QPushButton::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_hasSubtools) {
            m_longPressTimer->stop();
            if (m_longPressTriggered) {
                // User held button down, moved mouse to pick subtool, and just let go!
                // Confirm whatever is currently highlighted in the popup immediately!
                if (m_activePopup) {
                    m_activePopup->confirmSelectionAndClose();
                    m_activePopup = nullptr;
                }
                setDown(false);
                return;
            }
        }
        QPushButton::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event);

        QPainter painter(this);
        QColor iconColor = isChecked() ? QColor("#001a33") : QColor("#222222");
        drawToolIcon(painter, QRectF(rect()), m_type, iconColor);

        if (m_hasSubtools) {
            painter.setBrush(iconColor);
            painter.setPen(Qt::NoPen);
            QPolygonF cornerPoly;
            QPointF br(width() - 8, height() - 8);
            cornerPoly << br << QPointF(br.x() - 6, br.y()) << QPointF(br.x(), br.y() - 6);
            painter.drawPolygon(cornerPoly);
        }
    }

private:
    void updateMetadata()
    {
        const QString name = ToolBarWidget::toolName(m_type);
        setObjectName(QStringLiteral("ToolButton_%1").arg(int(m_type)));
        setAccessibleName(name);
        setToolTip(m_hasSubtools
            ? QStringLiteral("%1（按住或拖动可选择子工具）").arg(name)
            : name);
    }

    ToolType m_type;
    bool m_hasSubtools;
    QTimer* m_longPressTimer = nullptr;
    QPoint m_pressPos;
    bool m_longPressTriggered = false;
    QPointer<SubtoolPopup> m_activePopup;
};

// =============================================================================
// ToolBarWidget Implementation
// =============================================================================

ToolBarWidget::ToolBarWidget(QWidget *parent)
    : QWidget(parent), m_buttonGroup(new QButtonGroup(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(4, 12, 4, 12);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    setStyleSheet(
        "QPushButton {"
        "    background-color: #ffffff;"
        "    border: 2px solid #e4e7ed;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #f7f9fc;"
        "    border-color: #d1d5db;"
        "}"
        "QPushButton:checked {"
        "    background-color: #ffffff;"
        "    border: 2.5px solid #009dff;"
        "}"
    );

    m_buttonGroup->setExclusive(true);

    struct ItemDef {
        ToolType defaultType;
        bool hasSubtools;
        QList<ToolType> subtools;
    } items[] = {
        { ToolType::Select, true, { ToolType::Select, ToolType::BoxSelect } },
        { ToolType::Line, true, { ToolType::Line, ToolType::Arrow, ToolType::DualArrow } },
        { ToolType::Rectangle, false, {} },
        { ToolType::Circle, false, {} },
        { ToolType::Diamond, false, {} },
        { ToolType::Text, false, {} }
    };

    for (const ItemDef& item : items) {
        ToolButton *btn = new ToolButton(item.defaultType, item.hasSubtools, this);
        layout->addWidget(btn);
        m_buttonGroup->addButton(btn, static_cast<int>(item.defaultType));

        if (item.hasSubtools) {
            if (item.defaultType == ToolType::Select) m_selectGroupButton = btn;
            if (item.defaultType == ToolType::Line) m_lineGroupButton = btn;
            QList<ToolType> groupSubtools = item.subtools;

            connect(btn, &ToolButton::longPressTriggered, this, [this, btn, groupSubtools]() {
                ToolType currentSub = (btn == m_selectGroupButton) ? m_currentSelectSubTool : m_currentLineSubTool;
                SubtoolPopup* popup = new SubtoolPopup(groupSubtools, currentSub, this);
                btn->setActivePopup(popup);
                
                QPoint globalBtnPos = btn->mapToGlobal(QPoint(0, 0));
                QPoint popupPos(globalBtnPos.x() - 23, globalBtnPos.y() - 23);
                popup->move(popupPos);

                connect(popup, &SubtoolPopup::toolSelected, this, [this, btn](ToolType pickedTool) {
                    if (btn == m_selectGroupButton) m_currentSelectSubTool = pickedTool;
                    if (btn == m_lineGroupButton) m_currentLineSubTool = pickedTool;

                    btn->setToolType(pickedTool);
                    
                    // 分组按钮始终保留自己的稳定 ID（Line/Select）。
                    // 当前子工具由 m_currentLineSubTool / m_currentSelectSubTool 管理，
                    // 不要动态修改 QButtonGroup 的 ID，否则后续通过 button(id)
                    // 查找其它工具时会出现状态漂移。
                    btn->setChecked(true);
                    
                    m_currentTool = pickedTool;
                    emit toolClicked(m_currentTool);
                    emit toolChanged(m_currentTool);
                });

                popup->startInteraction();
            });
        }

        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            ToolType effectiveTool = btn->toolType();
            if (btn == m_lineGroupButton) {
                effectiveTool = m_currentLineSubTool;
            } else if (btn == m_selectGroupButton) {
                effectiveTool = m_currentSelectSubTool;
            }
            m_currentTool = effectiveTool;
            emit toolClicked(m_currentTool);
            emit toolChanged(m_currentTool);
        });

        if (item.defaultType == ToolType::Select) {
            btn->setChecked(true);
        }
    }

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

ToolBarWidget::~ToolBarWidget() = default;

ToolType ToolBarWidget::currentTool() const
{
    return m_currentTool;
}

void ToolBarWidget::setCurrentTool(ToolType tool)
{
    if (m_currentTool != tool) {
        m_currentTool = tool;
        if (tool == ToolType::Line || tool == ToolType::Arrow || tool == ToolType::DualArrow) {
            if (m_lineGroupButton) {
                m_currentLineSubTool = tool;
                m_lineGroupButton->setToolType(tool);
                m_lineGroupButton->setChecked(true);
            }
        } else if (tool == ToolType::Select || tool == ToolType::BoxSelect) {
            if (m_selectGroupButton) {
                m_currentSelectSubTool = tool;
                m_selectGroupButton->setToolType(tool);
                m_selectGroupButton->setChecked(true);
            }
        } else {
            if (QAbstractButton *btn = m_buttonGroup->button(static_cast<int>(tool))) {
                btn->setChecked(true);
            }
        }
        emit toolChanged(m_currentTool);
    }
}

QString ToolBarWidget::toolName(ToolType tool)
{
    switch (tool) {
        case ToolType::Select: return "Select (Pointer/Pan)";
        case ToolType::BoxSelect: return "Box Select (Marquee)";
        case ToolType::Line: return "Line";
        case ToolType::Arrow: return "Single Arrow";
        case ToolType::DualArrow: return "Dual Arrow";
        case ToolType::Rectangle: return "Rectangle";
        case ToolType::Circle: return "Circle";
        case ToolType::Diamond: return "Diamond";
        case ToolType::Text: return "Text";
        default: return "Unknown";
    }
}

#include "toolbar_widget.moc"
