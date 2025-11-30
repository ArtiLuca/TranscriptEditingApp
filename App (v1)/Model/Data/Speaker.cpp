#include "Speaker.h"

#include <QRandomGenerator>

namespace Model {
namespace Data {

Speaker::Speaker(const QString& id,
                 const QString& displayName,
                 const QColor& color)
    : id(id),
    displayName(displayName),
    color(color)
{}

bool Speaker::isValid() const {

    return (!id.isEmpty() && !displayName.isEmpty());
}

QString Speaker::toLabel() const {

    return displayName + ":";
}

void Speaker::assignRandomColor() {

    color = QColor::fromRgb(
        QRandomGenerator::global()->bounded(256),
        QRandomGenerator::global()->bounded(256),
        QRandomGenerator::global()->bounded(256));
}

}
}
