/*
 * Copyright (C) Wolthera van Hovell tot Westerflier <griffinvalley@gmail.com>, (C) 2016
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

#include <QList>
#include <QAbstractSpinBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPointer>

#include "KoColorSpaceRegistry.h"
#include <KoColorSet.h>
#include <KoResourceServerProvider.h>
#include <KoResourceServer.h>

#include "kis_signal_compressor.h"
#include "KisViewManager.h"
#include "KoColorDisplayRendererInterface.h"

#include "kis_spinbox_color_selector.h"

#include "kis_internal_color_selector.h"
#include "ui_wdgdlginternalcolorselector.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_color_input.h"

struct KisInternalColorSelector::Private
{
    bool allowUpdates = true;
    KoColor currentColor;
    KoColor previousColor;
    KoColor sRGB = KoColor(KoColorSpaceRegistry::instance()->rgb8());
    const KoColorSpace *currentColorSpace;
    bool lockUsedCS = false;
    bool chooseAlpha = false;
    KisSignalCompressor *compressColorChanges;
    const KoColorDisplayRendererInterface *displayRenderer;
    KisHexColorInput *hexColorInput;
};

KisInternalColorSelector::KisInternalColorSelector(QWidget *parent, KoColor color, bool modal, const QString &caption, const KoColorDisplayRendererInterface *displayRenderer)
    : QDialog(parent)
     ,m_d(new Private)
{
    setModal(modal);
    this->setFocusPolicy(Qt::ClickFocus);
    m_ui = new Ui_WdgDlgInternalColorSelector();
    m_ui->setupUi(this);

    setWindowTitle(caption);

    m_d->currentColor = color;
    m_d->currentColorSpace = m_d->currentColor.colorSpace();
    m_d->displayRenderer = displayRenderer;

    m_ui->spinboxselector->slotSetColor(color);
    connect(m_ui->spinboxselector, SIGNAL(sigNewColor(KoColor)), this, SLOT(slotColorUpdated(KoColor)));

    m_ui->visualSelector->slotSetColor(color);
    m_ui->visualSelector->setDisplayRenderer(displayRenderer);
    connect(m_ui->visualSelector, SIGNAL(sigNewColor(KoColor)), this, SLOT(slotColorUpdated(KoColor)));
    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), m_ui->visualSelector, SLOT(ConfigurationChanged()));

    connect(m_ui->screenColorPicker, SIGNAL(sigNewColorPicked(KoColor)),this, SLOT(slotColorUpdated(KoColor)));
    //TODO: Add disable signal as well. Might be not necessary...?
    KisConfig cfg;
    QString paletteName = cfg.readEntry("internal_selector_active_color_set", QString());
    KoResourceServer<KoColorSet>* rServer = KoResourceServerProvider::instance()->paletteServer(false);
    KoColorSet *savedPal = rServer->resourceByName(paletteName);
    if (savedPal) {
        m_ui->paletteBox->setColorSet(savedPal);
    } else {
        savedPal = rServer->resources().first();
        if (savedPal) {
            m_ui->paletteBox->setColorSet(savedPal);
        }
    }
    connect(m_ui->paletteBox, SIGNAL(colorChanged(KoColor,bool)), this, SLOT(slotColorUpdated(KoColor)));
    m_ui->paletteBox->setDisplayRenderer(displayRenderer);

    m_ui->currentColor->setColor(m_d->currentColor);
    m_ui->currentColor->setDisplayRenderer(displayRenderer);
    m_ui->currentColor->setFrameStyle(QFrame::StyledPanel);
    m_ui->previousColor->setColor(m_d->currentColor);
    m_ui->previousColor->setDisplayRenderer(displayRenderer);
    m_ui->previousColor->setFrameStyle(QFrame::StyledPanel);
    connect(this, SIGNAL(accepted()), this, SLOT(setPreviousColor()));
    connect(m_ui->previousColor, SIGNAL(triggered(KoColorPatch*)), SLOT(slotSetColorFromPatch(KoColorPatch*)));

    m_d->sRGB.fromKoColor(m_d->currentColor);
    m_d->hexColorInput = new KisHexColorInput(this, &m_d->sRGB);
    m_d->hexColorInput->update();
    connect(m_d->hexColorInput, SIGNAL(updated()), SLOT(slotSetColorFromHex()));
    m_ui->leftPane->addWidget(m_d->hexColorInput);
    m_d->hexColorInput->setToolTip(i18n("This is a hexcode input, for webcolors. It can only get colors in the sRGB space."));

    connect(this, SIGNAL(signalForegroundColorChosen(KoColor)), this, SLOT(slotLockSelector()));
    m_d->compressColorChanges = new KisSignalCompressor(100 /* ms */, KisSignalCompressor::POSTPONE, this);
    connect(m_d->compressColorChanges, SIGNAL(timeout()), this, SLOT(endUpdateWithNewColor()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

KisInternalColorSelector::~KisInternalColorSelector()
{
    delete m_ui;
    //TODO: Does the scoped pointer also need to be deleted???
}

void KisInternalColorSelector::slotColorUpdated(KoColor newColor)
{
    //if the update did not come from this selector...
    if (m_d->allowUpdates || QObject::sender() == this->parent()) {
        if (m_d->lockUsedCS){
            newColor.convertTo(m_d->currentColorSpace);
            m_d->currentColor = newColor;
        } else {
            m_d->currentColor = newColor;
        }
        updateAllElements(QObject::sender());
    }
}

void KisInternalColorSelector::colorSpaceChanged(const KoColorSpace *cs)
{
    if (cs == m_d->currentColorSpace) {
        return;
    }

    m_d->currentColorSpace = KoColorSpaceRegistry::instance()->colorSpace(cs->colorModelId().id(), cs->colorDepthId().id(), cs->profile());
    m_ui->spinboxselector->slotSetColorSpace(m_d->currentColorSpace);
    m_ui->visualSelector->slotsetColorSpace(m_d->currentColorSpace);

}

void KisInternalColorSelector::lockUsedColorSpace(const KoColorSpace *cs)
{
    colorSpaceChanged(cs);
    m_d->lockUsedCS = true;
}

void KisInternalColorSelector::setDisplayRenderer(const KoColorDisplayRendererInterface *displayRenderer)
{
    if (displayRenderer) {
        m_d->displayRenderer = displayRenderer;
        m_ui->visualSelector->setDisplayRenderer(displayRenderer);
        m_ui->currentColor->setDisplayRenderer(displayRenderer);
        m_ui->previousColor->setDisplayRenderer(displayRenderer);
        m_ui->paletteBox->setDisplayRenderer(displayRenderer);
    } else {
        m_d->displayRenderer = KoDumbColorDisplayRenderer::instance();
    }
}

KoColor KisInternalColorSelector::getModalColorDialog(const KoColor color, bool chooseAlpha, QWidget* parent, QString caption)
{
    KisInternalColorSelector dialog(parent, color, true, caption);
    dialog.chooseAlpha(chooseAlpha);
    dialog.exec();
    return dialog.getCurrentColor();
}

KoColor KisInternalColorSelector::getCurrentColor()
{
    return m_d->currentColor;
}

void KisInternalColorSelector::chooseAlpha(bool chooseAlpha)
{
    m_d->chooseAlpha = chooseAlpha;
}

void KisInternalColorSelector::slotConfigurationChanged()
{
    //m_d->canvas->displayColorConverter()->
    //slotColorSpaceChanged(m_d->canvas->image()->colorSpace());
}

void KisInternalColorSelector::slotLockSelector()
{
    m_d->allowUpdates = false;
}

void KisInternalColorSelector::setPreviousColor()
{
    m_d->previousColor = m_d->currentColor;
    KisConfig cfg;
    if (m_ui->paletteBox->colorSet()) {
        cfg.writeEntry("internal_selector_active_color_set", m_ui->paletteBox->colorSet()->name());
    }
}

void KisInternalColorSelector::updateAllElements(QObject *source)
{
    //update everything!!!
    if (source != m_ui->spinboxselector) {
        m_ui->spinboxselector->slotSetColor(m_d->currentColor);
    }
    if (source != m_ui->visualSelector) {
        m_ui->visualSelector->slotSetColor(m_d->currentColor);
    }

    if (source != m_d->hexColorInput) {
        m_d->sRGB.fromKoColor(m_d->currentColor);
        m_d->hexColorInput->update();
    }

    m_ui->previousColor->setColor(m_d->previousColor);

    m_ui->currentColor->setColor(m_d->currentColor);

    if (source != this->parent()) {
        emit(signalForegroundColorChosen(m_d->currentColor));
        m_d->compressColorChanges->start();
    }
}


void KisInternalColorSelector::endUpdateWithNewColor()
{
    m_d->allowUpdates = true;
}

void KisInternalColorSelector::focusInEvent(QFocusEvent *e)
{
        setPreviousColor();
}

void KisInternalColorSelector::slotSetColorFromPatch(KoColorPatch* patch)
{
    slotColorUpdated(patch->color());
}

void KisInternalColorSelector::slotSetColorFromHex()
{
    slotColorUpdated(m_d->sRGB);
}
