#ifndef SWITCHINGCOMBO_H
#define SWITCHINGCOMBO_H

#include "Global.h"

class SwitchingCombo : public QComboBox
{
    Q_OBJECT

public:
    SwitchingCombo(const SwitchingCombo&) = delete;
    SwitchingCombo& operator=(const SwitchingCombo&) = delete;
    SwitchingCombo();

public slots:
    void repopulate();
};

#endif
