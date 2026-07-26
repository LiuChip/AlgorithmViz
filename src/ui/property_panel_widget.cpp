#include "property_panel_widget.h"

#include "../core/commands/undo_commands.h"
#include "../core/undo_manager.h"
#include "../shapes/connector/connector.h"
#include "../shapes/text_label.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QUndoCommand>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr qreal kEpsilon = 1e-6;

bool sameColor(const QColor &left, const QColor &right)
{
    return left.rgba64() == right.rgba64();
}

bool sameBorder(const Border &left, const Border &right)
{
    return std::abs(left.borderWidth - right.borderWidth) <= kEpsilon
        && sameColor(left.borderColor, right.borderColor)
        && left.borderStyle == right.borderStyle;
}

bool sameFill(const FillStyle &left, const FillStyle &right)
{
    return sameColor(left.fillColor, right.fillColor)
        && std::abs(left.fillOpacity - right.fillOpacity) <= kEpsilon;
}

bool sameText(const TextStyle &left, const TextStyle &right)
{
    return left.text == right.text && left.font == right.font
        && sameColor(left.textColor, right.textColor)
        && left.alignment == right.alignment;
}

QHBoxLayout *colorRow(QWidget *parent, QPushButton *&button, QLineEdit *&edit,
                      const char *editObjectName)
{
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    button = new QPushButton(parent);
    button->setFixedSize(34, 28);
    button->setToolTip(QStringLiteral("打开颜色选择器"));
    edit = new QLineEdit(parent);
    edit->setObjectName(QString::fromLatin1(editObjectName));
    edit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    edit->setMaxLength(9);
    layout->addWidget(button);
    layout->addWidget(edit, 1);
    return layout;
}
} // namespace

PropertyPanelWidget::PropertyPanelWidget(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("PropertyPanel"));
    setMinimumWidth(270);
    setStyleSheet(QStringLiteral(
        "QFrame#PropertyPanel { background:#fff; border:1px solid #e4e7ed; border-radius:12px; }"
        "QGroupBox { font-size:13px; font-weight:600; color:#303133; border:1px solid #ebeef5;"
        " border-radius:8px; margin-top:12px; padding:14px 10px 10px; }"
        "QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 4px; color:#409eff; }"
        "QLineEdit,QDoubleSpinBox,QSpinBox,QComboBox,QFontComboBox { border:1px solid #dcdfe6;"
        " border-radius:6px; padding:4px 6px; background:#fff; color:#303133; font-size:12px; }"
        "QLineEdit:focus,QDoubleSpinBox:focus,QSpinBox:focus,QComboBox:focus,QFontComboBox:focus"
        " { border-color:#409eff; }"
        "QPushButton,QToolButton { border:1px solid #dcdfe6; border-radius:6px; padding:5px 9px;"
        " background:#fff; color:#606266; font-size:12px; }"
        "QPushButton:hover,QToolButton:hover { background:#f5f7fa; color:#409eff; border-color:#c6e2ff; }"
        "QToolButton:checked { background:#ecf5ff; color:#409eff; border-color:#409eff; }"
        "QCheckBox { font-size:12px; color:#606266; }"));
    setupUI();
    setShape(nullptr);
}

PropertyPanelWidget::~PropertyPanelWidget() = default;

void PropertyPanelWidget::setupUI()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 16, 12, 16);
    root->setSpacing(0);
    auto *title = new QLabel(QStringLiteral("属性检查器 (Property Panel)"), this);
    title->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:#222;padding-bottom:12px;"
                                        "border-bottom:1px solid #ebeef5;"));
    root->addWidget(title);

    m_emptyStateWidget = new QWidget(this);
    m_emptyStateWidget->setObjectName(QStringLiteral("PropertyEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    auto *empty = new QLabel(QStringLiteral("未选中任何图元\n\n请在画板或图形列表中选择图形"), m_emptyStateWidget);
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet(QStringLiteral("color:#909399;font-size:13px;padding-top:40px;"));
    emptyLayout->addWidget(empty);
    emptyLayout->addStretch();
    root->addWidget(m_emptyStateWidget);

    m_formContainer = new QWidget(this);
    m_formContainer->setObjectName(QStringLiteral("PropertyFormContainer"));
    auto *formRoot = new QVBoxLayout(m_formContainer);
    formRoot->setContentsMargins(0, 4, 0, 0);
    auto *scroll = new QScrollArea(m_formContainer);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("PropertyScrollContent"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(2, 0, 2, 0);
    contentLayout->setSpacing(8);

    auto *identity = new QGroupBox(QStringLiteral("基础信息 (Identity)"), content);
    auto *identityForm = new QFormLayout(identity);
    m_idLabel = new QLabel(identity);
    m_idLabel->setStyleSheet(QStringLiteral("font-family:monospace;font-weight:600;color:#409eff;"));
    m_nameEdit = new QLineEdit(identity);
    m_nameEdit->setObjectName(QStringLiteral("PropertyShapeNameEdit"));
    m_lockCheck = new QCheckBox(QStringLiteral("锁定位置、尺寸与变换"), identity);
    identityForm->addRow(QStringLiteral("ID 编号:"), m_idLabel);
    identityForm->addRow(QStringLiteral("图元名称:"), m_nameEdit);
    identityForm->addRow(QString(), m_lockCheck);
    contentLayout->addWidget(identity);

    auto *geometry = new QGroupBox(QStringLiteral("坐标与尺寸 (Geometry)"), content);
    auto *geometryForm = new QFormLayout(geometry);
    auto makeDoubleSpin = [geometry](qreal minimum, qreal maximum, const QString &suffix) {
        auto *spin = new QDoubleSpinBox(geometry);
        spin->setRange(minimum, maximum);
        spin->setDecimals(1);
        spin->setSuffix(suffix);
        spin->setKeyboardTracking(false);
        return spin;
    };
    m_posXSpin = makeDoubleSpin(-99999, 99999, QStringLiteral(" px"));
    m_posYSpin = makeDoubleSpin(-99999, 99999, QStringLiteral(" px"));
    m_widthSpin = makeDoubleSpin(0, 99999, QStringLiteral(" px"));
    m_heightSpin = makeDoubleSpin(0, 99999, QStringLiteral(" px"));
    m_rotationSpin = makeDoubleSpin(-36000, 36000, QStringLiteral("°"));
    m_zValueSpin = makeDoubleSpin(-99999, 99999, QString());
    geometryForm->addRow(QStringLiteral("X 坐标:"), m_posXSpin);
    geometryForm->addRow(QStringLiteral("Y 坐标:"), m_posYSpin);
    geometryForm->addRow(QStringLiteral("宽度:"), m_widthSpin);
    geometryForm->addRow(QStringLiteral("高度:"), m_heightSpin);
    geometryForm->addRow(QStringLiteral("旋转:"), m_rotationSpin);
    geometryForm->addRow(QStringLiteral("图层:"), m_zValueSpin);
    contentLayout->addWidget(geometry);

    auto *appearance = new QGroupBox(QStringLiteral("外观样式 (Appearance)"), content);
    m_appearanceForm = new QFormLayout(appearance);
    auto *appearanceForm = m_appearanceForm;
    appearanceForm->addRow(QStringLiteral("边框颜色:"), colorRow(appearance, m_borderColorBtn,
                                                                  m_borderHexEdit, "BorderHexEdit"));
    m_borderWidthSpin = makeDoubleSpin(0, 100, QStringLiteral(" px"));
    m_borderStyleCombo = new QComboBox(appearance);
    m_borderStyleCombo->addItems({QStringLiteral("实线"), QStringLiteral("虚线"),
                                  QStringLiteral("点线"), QStringLiteral("无边框")});
    appearanceForm->addRow(QStringLiteral("边框宽度:"), m_borderWidthSpin);
    appearanceForm->addRow(QStringLiteral("边框样式:"), m_borderStyleCombo);
    m_endStyleCombo = new QComboBox(appearance);
    m_endStyleCombo->setObjectName(QStringLiteral("ConnectorEndStyleCombo"));
    m_endStyleCombo->addItem(QStringLiteral("无箭头"), int(Connector::EndStyle::None));
    m_endStyleCombo->addItem(QStringLiteral("单向箭头"), int(Connector::EndStyle::Arrow));
    m_endStyleCombo->addItem(QStringLiteral("双向箭头"), int(Connector::EndStyle::DualArrow));
    appearanceForm->addRow(QStringLiteral("线端样式:"), m_endStyleCombo);
    appearanceForm->addRow(QStringLiteral("填充颜色:"), colorRow(appearance, m_fillColorBtn,
                                                                  m_fillHexEdit, "FillHexEdit"));
    auto *opacityRow = new QHBoxLayout;
    m_opacitySlider = new QSlider(Qt::Horizontal, appearance);
    m_opacitySlider->setRange(0, 100);
    m_opacityLabel = new QLabel(appearance);
    m_opacityLabel->setMinimumWidth(36);
    opacityRow->addWidget(m_opacitySlider, 1);
    opacityRow->addWidget(m_opacityLabel);
    appearanceForm->addRow(QStringLiteral("填充不透明度:"), opacityRow);
    contentLayout->addWidget(appearance);

    auto *typography = new QGroupBox(QStringLiteral("文字样式 (Typography)"), content);
    m_textForm = new QFormLayout(typography);
    m_textEdit = new QLineEdit(typography);
    m_fontFamilyCombo = new QFontComboBox(typography);
    m_fontFamilyCombo->setObjectName(QStringLiteral("FontFamilyCombo"));
    m_fontFamilyCombo->setEditable(true);
    m_fontSizeSpin = new QSpinBox(typography);
    m_fontSizeSpin->setRange(1, 512);
    m_fontSizeSpin->setKeyboardTracking(false);
    auto *fontStyleRow = new QHBoxLayout;
    m_boldButton = new QToolButton(typography);
    m_boldButton->setObjectName(QStringLiteral("BoldButton"));
    m_boldButton->setText(QStringLiteral("B"));
    m_boldButton->setCheckable(true);
    m_boldButton->setToolTip(QStringLiteral("粗体"));
    m_italicButton = new QToolButton(typography);
    m_italicButton->setObjectName(QStringLiteral("ItalicButton"));
    m_italicButton->setText(QStringLiteral("I"));
    m_italicButton->setCheckable(true);
    m_italicButton->setToolTip(QStringLiteral("斜体"));
    fontStyleRow->addWidget(m_boldButton);
    fontStyleRow->addWidget(m_italicButton);
    fontStyleRow->addStretch();
    m_textAlignmentCombo = new QComboBox(typography);
    m_textAlignmentCombo->setObjectName(QStringLiteral("TextAlignmentCombo"));
    m_textAlignmentCombo->addItem(QStringLiteral("左对齐"), int(Qt::AlignLeft | Qt::AlignVCenter));
    m_textAlignmentCombo->addItem(QStringLiteral("居中"), int(Qt::AlignCenter));
    m_textAlignmentCombo->addItem(QStringLiteral("右对齐"), int(Qt::AlignRight | Qt::AlignVCenter));
    m_textAlignmentCombo->addItem(QStringLiteral("两端对齐"), int(Qt::AlignJustify | Qt::AlignVCenter));
    m_textLayoutModeCombo = new QComboBox(typography);
    m_textLayoutModeCombo->setObjectName(QStringLiteral("TextLayoutModeCombo"));
    m_textLayoutModeCombo->addItem(QStringLiteral("自动适应文字"), int(TextLabel::TextLayoutMode::AutoSize));
    m_textLayoutModeCombo->addItem(QStringLiteral("固定文本框尺寸"), int(TextLabel::TextLayoutMode::FixedSize));
    m_textForm->addRow(QStringLiteral("文字内容:"), m_textEdit);
    m_textForm->addRow(QStringLiteral("字体:"), m_fontFamilyCombo);
    m_textForm->addRow(QStringLiteral("字号:"), m_fontSizeSpin);
    m_textForm->addRow(QStringLiteral("字形:"), fontStyleRow);
    m_textForm->addRow(QStringLiteral("对齐:"), m_textAlignmentCombo);
    m_textForm->addRow(QStringLiteral("文字颜色:"), colorRow(typography, m_textColorBtn,
                                                                m_textHexEdit, "TextHexEdit"));
    m_textForm->addRow(QStringLiteral("文本框尺寸:"), m_textLayoutModeCombo);
    contentLayout->addWidget(typography);

    auto *actions = new QGroupBox(QStringLiteral("操作 (Actions)"), content);
    auto *actionsLayout = new QHBoxLayout(actions);
    auto *back = new QPushButton(QStringLiteral("置于底层"), actions);
    auto *front = new QPushButton(QStringLiteral("置于顶层"), actions);
    auto *remove = new QPushButton(QStringLiteral("删除"), actions);
    remove->setStyleSheet(QStringLiteral("color:#f56c6c;"));
    actionsLayout->addWidget(back);
    actionsLayout->addWidget(front);
    actionsLayout->addWidget(remove);
    contentLayout->addWidget(actions);
    contentLayout->addStretch();

    scroll->setWidget(content);
    formRoot->addWidget(scroll);
    root->addWidget(m_formContainer, 1);

    connect(m_nameEdit, &QLineEdit::editingFinished, this, &PropertyPanelWidget::onNameEdited);
    connect(m_lockCheck, &QCheckBox::toggled, this, &PropertyPanelWidget::onLockToggled);
    for (QDoubleSpinBox *spin : {m_posXSpin, m_posYSpin})
        connect(spin, &QDoubleSpinBox::editingFinished, this, &PropertyPanelWidget::onPositionEdited);
    for (QDoubleSpinBox *spin : {m_widthSpin, m_heightSpin})
        connect(spin, &QDoubleSpinBox::editingFinished, this, &PropertyPanelWidget::onSizeEdited);
    connect(m_rotationSpin, &QDoubleSpinBox::editingFinished, this, &PropertyPanelWidget::onRotationEdited);
    connect(m_zValueSpin, &QDoubleSpinBox::editingFinished, this, &PropertyPanelWidget::onLayerEdited);
    connect(m_borderWidthSpin, &QDoubleSpinBox::editingFinished, this, &PropertyPanelWidget::onBorderWidthEdited);
    connect(m_borderStyleCombo, &QComboBox::currentIndexChanged, this, &PropertyPanelWidget::onBorderStyleChanged);
    connect(m_endStyleCombo, &QComboBox::currentIndexChanged, this, &PropertyPanelWidget::onEndStyleChanged);
    connect(m_borderColorBtn, &QPushButton::clicked, this, &PropertyPanelWidget::onPickBorderColor);
    connect(m_borderHexEdit, &QLineEdit::editingFinished, this, &PropertyPanelWidget::onBorderHexEdited);
    connect(m_fillColorBtn, &QPushButton::clicked, this, &PropertyPanelWidget::onPickFillColor);
    connect(m_fillHexEdit, &QLineEdit::editingFinished, this, &PropertyPanelWidget::onFillHexEdited);
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityLabel->setText(QStringLiteral("%1%").arg(value));
    });
    connect(m_opacitySlider, &QSlider::sliderReleased, this, &PropertyPanelWidget::onFillOpacityReleased);
    connect(m_textEdit, &QLineEdit::editingFinished, this, &PropertyPanelWidget::onTextEdited);
    connect(m_fontFamilyCombo, &QFontComboBox::currentFontChanged, this,
            &PropertyPanelWidget::onFontFamilyChanged);
    connect(m_fontSizeSpin, &QSpinBox::editingFinished, this, &PropertyPanelWidget::onFontSizeEdited);
    connect(m_boldButton, &QToolButton::toggled, this, &PropertyPanelWidget::onBoldToggled);
    connect(m_italicButton, &QToolButton::toggled, this, &PropertyPanelWidget::onItalicToggled);
    connect(m_textAlignmentCombo, &QComboBox::currentIndexChanged,
            this, &PropertyPanelWidget::onTextAlignmentChanged);
    connect(m_textColorBtn, &QPushButton::clicked, this, &PropertyPanelWidget::onPickTextColor);
    connect(m_textHexEdit, &QLineEdit::editingFinished, this, &PropertyPanelWidget::onTextHexEdited);
    connect(m_textLayoutModeCombo, &QComboBox::currentIndexChanged,
            this, &PropertyPanelWidget::onTextLayoutModeChanged);
    connect(front, &QPushButton::clicked, this, &PropertyPanelWidget::onBringToFront);
    connect(back, &QPushButton::clicked, this, &PropertyPanelWidget::onSendToBack);
    connect(remove, &QPushButton::clicked, this, &PropertyPanelWidget::onDeleteShape);
}

void PropertyPanelWidget::connectToCanvas(Canvas *canvas)
{
    if (m_canvas == canvas)
        return;
    if (m_canvas) {
        if (m_canvas->undoManager())
            disconnect(m_canvas->undoManager(), nullptr, this, nullptr);
        disconnect(m_canvas, nullptr, this, nullptr);
    }
    m_canvas = canvas;
    if (!m_canvas) {
        setShape(nullptr);
        return;
    }
    connect(m_canvas, &Canvas::selectionChanged, this, &PropertyPanelWidget::onSelectionChanged);
    connect(m_canvas->undoManager(), &UndoManager::historyChanged,
            this, &PropertyPanelWidget::refreshFromShape);
    onSelectionChanged();
}

void PropertyPanelWidget::setShape(::Shape *shape)
{
    if (m_currentShape == shape) {
        refreshFromShape();
        return;
    }
    if (m_currentShape)
        disconnect(m_currentShape.data(), nullptr, this, nullptr);
    m_currentShape = shape;
    if (m_currentShape) {
        connect(m_currentShape.data(), &::Shape::geometryChanged, this, &PropertyPanelWidget::refreshFromShape);
        connect(m_currentShape.data(), &::Shape::lockedChanged, this, &PropertyPanelWidget::refreshFromShape);
        connect(m_currentShape.data(), &QObject::destroyed, this, [this]() { setShape(nullptr); });
    }
    refreshFromShape();
}

void PropertyPanelWidget::onSelectionChanged()
{
    ::Shape *selected = nullptr;
    if (m_canvas && m_canvas->scene()) {
        for (QGraphicsItem *item : m_canvas->scene()->selectedItems()) {
            if (auto *shape = dynamic_cast<::Shape *>(item); shape && !shape->parentItem()) {
                selected = shape;
                break;
            }
        }
    }
    setShape(selected);
}

void PropertyPanelWidget::refreshFromShape()
{
    m_isUpdatingUI = true;
    const bool hasShape = !m_currentShape.isNull();
    m_emptyStateWidget->setVisible(!hasShape);
    m_formContainer->setVisible(hasShape);
    if (!hasShape) {
        m_isUpdatingUI = false;
        return;
    }

    const bool locked = m_currentShape->isLocked();
    m_idLabel->setText(QStringLiteral("#%1").arg(m_currentShape->getID()));
    m_nameEdit->setText(m_currentShape->getName());
    m_lockCheck->setChecked(locked);
    m_posXSpin->setValue(m_currentShape->pos().x());
    m_posYSpin->setValue(m_currentShape->pos().y());
    m_posXSpin->setEnabled(m_currentShape->supportsLayoutPosition() && !locked);
    m_posYSpin->setEnabled(m_currentShape->supportsLayoutPosition() && !locked);
    m_widthSpin->setValue(m_currentShape->getSize().width());
    m_heightSpin->setValue(m_currentShape->getSize().height());
    bool sizeEnabled = m_currentShape->supportsLayoutSize() && !locked;
    if (auto *label = dynamic_cast<TextLabel *>(m_currentShape.data()))
        sizeEnabled = sizeEnabled && label->getTextLayoutMode() == TextLabel::TextLayoutMode::FixedSize;
    m_widthSpin->setEnabled(sizeEnabled);
    m_heightSpin->setEnabled(sizeEnabled);
    m_rotationSpin->setValue(m_currentShape->getRotation());
    m_rotationSpin->setEnabled(m_currentShape->supportsRotation() && !locked);
    m_zValueSpin->setValue(m_currentShape->getLayer());

    const Border border = m_currentShape->getBorderInfo();
    setColorControl(m_borderColorBtn, m_borderHexEdit, border.borderColor);
    m_borderWidthSpin->setValue(border.borderWidth);
    const int borderIndex = border.borderStyle == Qt::DashLine ? 1
        : border.borderStyle == Qt::DotLine ? 2 : border.borderStyle == Qt::NoPen ? 3 : 0;
    m_borderStyleCombo->setCurrentIndex(borderIndex);

    auto *connector = dynamic_cast<Connector *>(m_currentShape.data());
    m_appearanceForm->setRowVisible(m_endStyleCombo, connector != nullptr);
    if (connector) {
        const int endStyleIndex = m_endStyleCombo->findData(int(connector->getEndStyle()));
        m_endStyleCombo->setCurrentIndex(std::max(0, endStyleIndex));
    }

    const FillStyle fill = m_currentShape->getFillInfo();
    setColorControl(m_fillColorBtn, m_fillHexEdit, fill.fillColor);
    m_opacitySlider->setValue(qRound(std::clamp(fill.fillOpacity, qreal(0), qreal(1)) * 100));

    const TextStyle text = m_currentShape->getTextInfo();
    m_textEdit->setText(text.text);
    m_fontFamilyCombo->setCurrentFont(text.font);
    m_fontSizeSpin->setValue(qRound(text.font.pointSizeF() > 0 ? text.font.pointSizeF() : 14));
    m_boldButton->setChecked(text.font.bold());
    m_italicButton->setChecked(text.font.italic());
    const Qt::Alignment horizontal = text.alignment & Qt::AlignHorizontal_Mask;
    const int alignmentValue = horizontal.testFlag(Qt::AlignRight)
        ? int(Qt::AlignRight | Qt::AlignVCenter)
        : horizontal.testFlag(Qt::AlignJustify) ? int(Qt::AlignJustify | Qt::AlignVCenter)
        : horizontal.testFlag(Qt::AlignLeft) ? int(Qt::AlignLeft | Qt::AlignVCenter)
        : int(Qt::AlignCenter);
    m_textAlignmentCombo->setCurrentIndex(std::max(0, m_textAlignmentCombo->findData(alignmentValue)));
    setColorControl(m_textColorBtn, m_textHexEdit, text.textColor);

    const auto *label = dynamic_cast<TextLabel *>(m_currentShape.data());
    m_textForm->setRowVisible(m_textLayoutModeCombo, label != nullptr);
    if (label) {
        m_textLayoutModeCombo->setCurrentIndex(
            m_textLayoutModeCombo->findData(int(label->getTextLayoutMode())));
    }
    m_isUpdatingUI = false;
}

void PropertyPanelWidget::setColorControl(QPushButton *button, QLineEdit *edit,
                                          const QColor &color)
{
    const QString name = color.alpha() == 255 ? color.name(QColor::HexRgb)
                                               : color.name(QColor::HexArgb);
    edit->setText(name.toUpper());
    button->setStyleSheet(QStringLiteral("background:%1;border:1px solid #c0c4cc;").arg(name));
}

QColor PropertyPanelWidget::colorFromEdit(QLineEdit *edit, const QColor &fallback) const
{
    QColor color(edit->text().trimmed());
    if (!color.isValid()) {
        edit->setToolTip(QStringLiteral("请输入 #RRGGBB 或 #AARRGGBB"));
        return fallback;
    }
    edit->setToolTip(QString());
    return color;
}

void PropertyPanelWidget::executeCommand(QUndoCommand *command)
{
    if (!command)
        return;
    if (m_canvas && m_canvas->undoManager()) {
        m_canvas->undoManager()->push(command);
        return;
    }
    command->redo();
    delete command;
}

void PropertyPanelWidget::pushBorder(const Border &border, const QString &description)
{
    if (!m_currentShape)
        return;
    const Border old = m_currentShape->getBorderInfo();
    if (!sameBorder(old, border))
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::BorderStyle,
                                                old, border, description));
    refreshFromShape();
}

void PropertyPanelWidget::pushFill(const FillStyle &fill, const QString &description)
{
    if (!m_currentShape)
        return;
    const FillStyle old = m_currentShape->getFillInfo();
    if (!sameFill(old, fill))
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Fill,
                                                old, fill, description));
    refreshFromShape();
}

void PropertyPanelWidget::pushText(const TextStyle &text, const QString &description)
{
    if (!m_currentShape)
        return;
    const TextStyle old = m_currentShape->getTextInfo();
    if (!sameText(old, text))
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Text,
                                                old, text, description));
    refreshFromShape();
}

void PropertyPanelWidget::onNameEdited()
{
    if (!m_currentShape || m_isUpdatingUI)
        return;
    const QString old = m_currentShape->getName();
    const QString next = m_nameEdit->text();
    if (old != next)
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Name,
                                                old, next, QStringLiteral("Rename Shape")));
}

void PropertyPanelWidget::onLockToggled(bool locked)
{
    if (!m_currentShape || m_isUpdatingUI || locked == m_currentShape->isLocked())
        return;
    executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Locked,
                                            m_currentShape->isLocked(), locked,
                                            locked ? QStringLiteral("Lock Shape")
                                                   : QStringLiteral("Unlock Shape")));
    refreshFromShape();
}

void PropertyPanelWidget::onPositionEdited()
{
    if (!m_currentShape || m_isUpdatingUI)
        return;
    const QPointF old = m_currentShape->pos();
    const QPointF next(m_posXSpin->value(), m_posYSpin->value());
    if (old != next)
        executeCommand(new MoveItemsCommand({{m_currentShape, old, next}}));
}

void PropertyPanelWidget::onSizeEdited()
{
    if (!m_currentShape || m_isUpdatingUI)
        return;
    const QSizeF old = m_currentShape->getSize();
    const QSizeF next(m_widthSpin->value(), m_heightSpin->value());
    if (old != next)
        executeCommand(new ResizeItemCommand(m_currentShape, old, next,
                                             m_currentShape->pos(), m_currentShape->pos()));
}

void PropertyPanelWidget::onRotationEdited()
{
    if (!m_currentShape || m_isUpdatingUI || !m_currentShape->supportsRotation())
        return;
    const qreal old = m_currentShape->getRotation();
    const qreal next = m_rotationSpin->value();
    if (std::abs(old - next) > kEpsilon)
        executeCommand(new RotateItemCommand(m_currentShape, old, next));
}

void PropertyPanelWidget::onLayerEdited()
{
    if (!m_currentShape || m_isUpdatingUI)
        return;
    const int old = m_currentShape->getLayer();
    const int next = qRound(m_zValueSpin->value());
    if (old != next)
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Layer,
                                                old, next, QStringLiteral("Change Layer")));
}

void PropertyPanelWidget::onBorderWidthEdited() { if (!m_currentShape || m_isUpdatingUI) return; Border v = m_currentShape->getBorderInfo(); v.borderWidth = m_borderWidthSpin->value(); pushBorder(v, QStringLiteral("Change Border Width")); }
void PropertyPanelWidget::onBorderStyleChanged(int index) { if (m_isUpdatingUI || !m_currentShape) return; Border v = m_currentShape->getBorderInfo(); const Qt::PenStyle styles[] = {Qt::SolidLine, Qt::DashLine, Qt::DotLine, Qt::NoPen}; v.borderStyle = styles[std::clamp(index, 0, 3)]; pushBorder(v, QStringLiteral("Change Border Style")); }
void PropertyPanelWidget::onEndStyleChanged(int index) {
    if (m_isUpdatingUI || !m_currentShape || index < 0)
        return;

    auto *connector = dynamic_cast<Connector *>(m_currentShape.data());
    if (!connector)
        return;

    const QVariant data = m_endStyleCombo->itemData(index);
    if (!data.isValid())
        return;

    const auto oldStyle = connector->getEndStyle();
    const auto newStyle = static_cast<Connector::EndStyle>(data.toInt());
    if (oldStyle != newStyle) {
        executeCommand(new ShapePropertyCommand(
            connector, ShapePropertyCommand::Property::EndStyle,
            oldStyle, newStyle, QStringLiteral("Change Line End Style")));
    }
}
void PropertyPanelWidget::onPickBorderColor() { if (!m_currentShape) return; QColor c = QColorDialog::getColor(m_currentShape->getBorderInfo().borderColor, this, QStringLiteral("选择边框颜色"), QColorDialog::ShowAlphaChannel); if (c.isValid()) { Border v = m_currentShape->getBorderInfo(); v.borderColor = c; pushBorder(v, QStringLiteral("Change Border Color")); } }
void PropertyPanelWidget::onBorderHexEdited() { if (!m_currentShape || m_isUpdatingUI) return; Border v = m_currentShape->getBorderInfo(); v.borderColor = colorFromEdit(m_borderHexEdit, v.borderColor); pushBorder(v, QStringLiteral("Change Border Color")); }
void PropertyPanelWidget::onPickFillColor() { if (!m_currentShape) return; QColor c = QColorDialog::getColor(m_currentShape->getFillInfo().fillColor, this, QStringLiteral("选择填充颜色"), QColorDialog::ShowAlphaChannel); if (c.isValid()) { FillStyle v = m_currentShape->getFillInfo(); v.fillColor = c; pushFill(v, QStringLiteral("Change Fill Color")); } }
void PropertyPanelWidget::onFillHexEdited() { if (!m_currentShape || m_isUpdatingUI) return; FillStyle v = m_currentShape->getFillInfo(); v.fillColor = colorFromEdit(m_fillHexEdit, v.fillColor); pushFill(v, QStringLiteral("Change Fill Color")); }
void PropertyPanelWidget::onFillOpacityReleased() { if (!m_currentShape || m_isUpdatingUI) return; FillStyle v = m_currentShape->getFillInfo(); v.fillOpacity = m_opacitySlider->value() / 100.0; pushFill(v, QStringLiteral("Change Fill Opacity")); }
void PropertyPanelWidget::onTextEdited() { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.text = m_textEdit->text(); pushText(v, QStringLiteral("Edit Text")); }
void PropertyPanelWidget::onFontFamilyChanged(const QFont &font) { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.font.setFamily(font.family()); pushText(v, QStringLiteral("Change Font")); }
void PropertyPanelWidget::onFontSizeEdited() { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.font.setPointSize(m_fontSizeSpin->value()); pushText(v, QStringLiteral("Change Font Size")); }
void PropertyPanelWidget::onBoldToggled(bool checked) { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.font.setBold(checked); pushText(v, QStringLiteral("Toggle Bold")); }
void PropertyPanelWidget::onItalicToggled(bool checked) { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.font.setItalic(checked); pushText(v, QStringLiteral("Toggle Italic")); }
void PropertyPanelWidget::onTextAlignmentChanged(int index) { if (!m_currentShape || m_isUpdatingUI || index < 0) return; TextStyle v = m_currentShape->getTextInfo(); v.alignment = Qt::Alignment(m_textAlignmentCombo->itemData(index).toInt()); pushText(v, QStringLiteral("Change Text Alignment")); }
void PropertyPanelWidget::onPickTextColor() { if (!m_currentShape) return; QColor c = QColorDialog::getColor(m_currentShape->getTextInfo().textColor, this, QStringLiteral("选择文字颜色"), QColorDialog::ShowAlphaChannel); if (c.isValid()) { TextStyle v = m_currentShape->getTextInfo(); v.textColor = c; pushText(v, QStringLiteral("Change Text Color")); } }
void PropertyPanelWidget::onTextHexEdited() { if (!m_currentShape || m_isUpdatingUI) return; TextStyle v = m_currentShape->getTextInfo(); v.textColor = colorFromEdit(m_textHexEdit, v.textColor); pushText(v, QStringLiteral("Change Text Color")); }

void PropertyPanelWidget::onTextLayoutModeChanged(int index)
{
    if (!m_currentShape || m_isUpdatingUI || index < 0)
        return;
    auto *label = dynamic_cast<TextLabel *>(m_currentShape.data());
    if (!label)
        return;
    const auto old = label->getTextLayoutMode();
    const auto next = static_cast<TextLabel::TextLayoutMode>(m_textLayoutModeCombo->itemData(index).toInt());
    if (old != next)
        executeCommand(new ShapePropertyCommand(label, ShapePropertyCommand::Property::TextLayoutMode,
                                                old, next, QStringLiteral("Change Text Layout")));
    refreshFromShape();
}

void PropertyPanelWidget::onBringToFront()
{
    if (!m_currentShape || !m_canvas || !m_canvas->scene())
        return;
    int maximum = m_currentShape->getLayer();
    for (QGraphicsItem *item : m_canvas->scene()->items())
        if (auto *shape = dynamic_cast<::Shape *>(item); shape && !shape->parentItem())
            maximum = std::max(maximum, shape->getLayer());

    // 图层使用 int 保存；位于上界时不能再执行 +1，否则会触发有符号整数溢出。
    if (maximum == std::numeric_limits<int>::max())
        return;

    const int next = maximum + 1;
    if (next != m_currentShape->getLayer())
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Layer,
                                                m_currentShape->getLayer(), next,
                                                QStringLiteral("Bring to Front")));
}

void PropertyPanelWidget::onSendToBack()
{
    if (!m_currentShape || !m_canvas || !m_canvas->scene())
        return;
    int minimum = m_currentShape->getLayer();
    for (QGraphicsItem *item : m_canvas->scene()->items())
        if (auto *shape = dynamic_cast<::Shape *>(item); shape && !shape->parentItem())
            minimum = std::min(minimum, shape->getLayer());

    // 与置顶操作对称地保护 int 下界，保持失败时场景和撤销栈均不发生变化。
    if (minimum == std::numeric_limits<int>::min())
        return;

    const int next = minimum - 1;
    if (next != m_currentShape->getLayer())
        executeCommand(new ShapePropertyCommand(m_currentShape, ShapePropertyCommand::Property::Layer,
                                                m_currentShape->getLayer(), next,
                                                QStringLiteral("Send to Back")));
}

void PropertyPanelWidget::onDeleteShape()
{
    if (!m_currentShape || !m_canvas)
        return;
    m_currentShape->setSelected(true);
    m_canvas->deleteSelected();
    setShape(nullptr);
}
