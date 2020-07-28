#ifndef SLOTRULER_H
#define SLOTRULER_H

#include <QStringList>
#include <QWidget>

#include "applicationsettings.h"
#include "reactivelist.hpp"
#include "coloredlabel.h"
#include "colorprovider.h"
#include "dataproviderwidget.h"

class SlotsWidget;
class SlotRuler : public DataProviderWidget,
                  public ReactiveList<ColoredLabel>,
                  public ColorProvider {
Q_OBJECT
public:
    explicit SlotRuler(QWidget *parent = nullptr);

    void reloadStrategy();

private:
    using ColorGetter = QColor (*)();

    int calculateLabelWidth();
    bool isIntegerHourAtIndex(int index);

    ColorGetter labelColorGetterAtIndex(int index);

    QString labelAtIndex(int index);

    // ReactiveList
    int numberOfItems() override;
    QVBoxLayout *listLayout() override;
    void reuseItemAtIndex(int index, ColoredLabel *itemWidget) override;
    ColoredLabel *makeNewItemAtIndex(int index) override;

    void paintEvent(QPaintEvent *) override;

    static const int bigFontHeight = 10;
    static const int smallFontHeight = 9;
};

#endif // SLOTRULER_H
