#ifndef MESSAGEDATAROLE_H
#define MESSAGEDATAROLE_H

#include <Qt>

enum MessageDataRole{
    Id= Qt::ItemDataRole::UserRole + 1,
    Username,
    Text,
    Time
};

#endif // MESSAGEDATAROLE_H
