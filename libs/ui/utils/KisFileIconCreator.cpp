/*
 *  Copyright (c) 2019 2020 Agata Cacko <cacko.azh@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "KisFileIconCreator.h"
#include <KoStore.h>
#include <KisMimeDatabase.h>
#include <KisDocument.h>
#include <KisPart.h>
#include <QFileInfo>

#include <kis_debug.h>

namespace
{

QIcon createIcon(const QImage &source, const QSize &iconSize, bool crop)
{
    QImage result;
    if (crop) {
        const int minSide = qMin(source.width(), source.height());
        const int minIconSize = qMin(iconSize.width(), iconSize.height());
        result = source.copy((source.width() - minSide) / 2, (source.height() - minSide) / 2, minSide, minSide)
            .scaled(minIconSize, minIconSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    } else {
        result = source.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return QIcon(QPixmap::fromImage(result));
}

}


KisFileIconCreator::KisFileIconCreator()
{
}

bool KisFileIconCreator::createFileIcon(QString path, QIcon &icon, qreal devicePixelRatioF, QSize iconSize, bool crop)
{
    iconSize *= devicePixelRatioF;
    QFileInfo fi(path);
    if (fi.exists()) {
        QString mimeType = KisMimeDatabase::mimeTypeForFile(path);
        if (mimeType == KisDocument::nativeFormatMimeType()
               || mimeType == "image/openraster") {

            QScopedPointer<KoStore> store(KoStore::createStore(path, KoStore::Read));
            if (store) {
                QString thumbnailpath;
                if (store->hasFile(QString("Thumbnails/thumbnail.png"))){
                    thumbnailpath = QString("Thumbnails/thumbnail.png");
                }
                else if (store->hasFile(QString("mergedimage.png"))) {
                    thumbnailpath = QString("mergedimage.png");
                }
                else if (store->hasFile(QString("preview.png"))) {
                    thumbnailpath = QString("preview.png");
                }
                if (!thumbnailpath.isEmpty() && store->open(thumbnailpath)) {

                    QByteArray bytes = store->read(store->size());
                    store->close();
                    QImage img;
                    img.loadFromData(bytes);
                    img.setDevicePixelRatio(devicePixelRatioF);

                    icon = createIcon(img, iconSize, crop);
                    return true;

                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else if (mimeType == "image/tiff" || mimeType == "image/x-tiff") {
            // Workaround for a bug in Qt tiff QImageIO plugin
            QScopedPointer<KisDocument> doc;
            doc.reset(KisPart::instance()->createTemporaryDocument());
            doc->setFileBatchMode(true);
            bool r = doc->openUrl(QUrl::fromLocalFile(path), KisDocument::DontAddToRecent);
            if (r) {
                KisPaintDeviceSP projection = doc->image()->projection();
                const QRect bounds = projection->exactBounds();
                const float ratio = static_cast<float>(bounds.width()) / bounds.height();
                const int maxWidth = qMax(iconSize.width(), iconSize.height());
                const int maxHeight = static_cast<int>(maxWidth * ratio);
                const QImage &thumbnail = projection->createThumbnail(maxWidth, maxHeight, bounds);
                icon = createIcon(thumbnail, iconSize, crop);
                return true;
            } else {
                return false;
            }
        } else {
            QImage img;
            img.setDevicePixelRatio(devicePixelRatioF);
            img.load(path);
            if (!img.isNull()) {
                icon = createIcon(img, iconSize, crop);
                return true;
            } else {
                return false;
            }
        }
    } else {
        return false;
    }
}
