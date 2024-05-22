#ifndef MESSAGEDATAROLE_H
#define MESSAGEDATAROLE_H

#include <Qt>

enum MessageDataRole{
    Time = Qt::ItemDataRole::UserRole + 1,
    Id,
    Username,
    Text
};

#endif // MESSAGEDATAROLE_H
