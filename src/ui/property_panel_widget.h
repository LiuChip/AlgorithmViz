#ifndef PROPERTY_PANEL_WIDGET_H
#define PROPERTY_PANEL_WIDGET_H

#include <QFrame>
#include <QPointer>

#include "../core/canvas.h"
#include "../shapes/shape.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QToolButton;
class QUndoCommand;
class QWidget;

// 当前选中图形的属性检查器。所有用户修改均通过 Canvas 的 UndoManager 提交。
class PropertyPanelWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PropertyPanelWidget(QWidget *parent = nullptr);
    ~PropertyPanelWidget() override;

    void setShape(::Shape *shape);
    void connectToCanvas(Canvas *canvas);

private slots:
    void refreshFromShape();
    void onSelectionChanged();
    void onNameEdited();
    void onLockToggled(bool locked);
    void onPositionEdited();
    void onSizeEdited();
    void onRotationEdited();
    void onLayerEdited();
    void onBorderWidthEdited();
    void onBorderStyleChanged(int index);
    void onEndStyleChanged(int index);
    void onPickBorderColor();
    void onBorderHexEdited();
    void onPickFillColor();
    void onFillHexEdited();
    void onFillOpacityReleased();
    void onTextEdited();
    void onFontFamilyChanged(const QFont &font);
    void onFontSizeEdited();
    void onBoldToggled(bool checked);
    void onItalicToggled(bool checked);
    void onTextAlignmentChanged(int index);
    void onPickTextColor();
    void onTextHexEdited();
    void onTextLayoutModeChanged(int index);
    void onBringToFront();
    void onSendToBack();
    void onDeleteShape();

private:
    void setupUI();
    void setColorControl(QPushButton *button, QLineEdit *edit, const QColor &color);
    QColor colorFromEdit(QLineEdit *edit, const QColor &fallback) const;
    void executeCommand(QUndoCommand *command);
    void pushBorder(const Border &border, const QString &description);
    void pushFill(const FillStyle &fill, const QString &description);
    void pushText(const TextStyle &text, const QString &description);

    QPointer<::Shape> m_currentShape;
    QPointer<Canvas> m_canvas;
    bool m_isUpdatingUI = false;

    QWidget *m_emptyStateWidget = nullptr;
    QWidget *m_formContainer = nullptr;
    QLabel *m_idLabel = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QCheckBox *m_lockCheck = nullptr;
    QDoubleSpinBox *m_posXSpin = nullptr;
    QDoubleSpinBox *m_posYSpin = nullptr;
    QDoubleSpinBox *m_widthSpin = nullptr;
    QDoubleSpinBox *m_heightSpin = nullptr;
    QDoubleSpinBox *m_rotationSpin = nullptr;
    QDoubleSpinBox *m_zValueSpin = nullptr;
    QPushButton *m_borderColorBtn = nullptr;
    QLineEdit *m_borderHexEdit = nullptr;
    QDoubleSpinBox *m_borderWidthSpin = nullptr;
    QComboBox *m_borderStyleCombo = nullptr;
    QComboBox *m_endStyleCombo = nullptr;
    QPushButton *m_fillColorBtn = nullptr;
    QLineEdit *m_fillHexEdit = nullptr;
    QSlider *m_opacitySlider = nullptr;
    QLabel *m_opacityLabel = nullptr;
    QLineEdit *m_textEdit = nullptr;
    QFontComboBox *m_fontFamilyCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QToolButton *m_boldButton = nullptr;
    QToolButton *m_italicButton = nullptr;
    QComboBox *m_textAlignmentCombo = nullptr;
    QPushButton *m_textColorBtn = nullptr;
    QLineEdit *m_textHexEdit = nullptr;
    QComboBox *m_textLayoutModeCombo = nullptr;
    QFormLayout *m_textForm = nullptr;
    QFormLayout *m_appearanceForm = nullptr;
};

#endif // PROPERTY_PANEL_WIDGET_H
