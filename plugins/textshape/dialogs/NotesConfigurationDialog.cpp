/* This file is part of the KDE project
 * Copyright (C) 2011 Brijesh Patel <brijesh3105@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "NotesConfigurationDialog.h"

#include <KAction>

#include <QWidget>
#include <QDebug>

NotesConfigurationDialog::NotesConfigurationDialog(QTextDocument *doc, QWidget *parent)
        : QDialog(parent),
          document(doc)
{
    widget.setupUi(this);

    connect(widget.footnote,SIGNAL(toggled(bool)),this,SLOT(footnoteSetup(bool)));
    connect(widget.endnote,SIGNAL(toggled(bool)),this,SLOT(endnoteSetup(bool)));
    connect(widget.buttonBox,SIGNAL(clicked(QAbstractButton*)),this,SLOT(apply(QAbstractButton*)));
   //connect(widget.buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
}

KoOdfNotesConfiguration *NotesConfigurationDialog::notesConfiguration()
{
    return notesConfig;
}

void NotesConfigurationDialog::setStyleManager(KoStyleManager *sm)
{
    m_styleManager = sm;
}

void NotesConfigurationDialog::footnoteSetup(bool on)
{
    widget.location_footnote->setEnabled(on);
    if(on) {
        widget.numStyleCombo->setCurrentIndex(0);
        widget.paragraphCombo->setCurrentIndex(7);
        widget.pageCombo->setCurrentIndex(7);
        widget.textareaCombo->setCurrentIndex(8);
        widget.noteareaCombo->setCurrentIndex(9);
    }
}

void NotesConfigurationDialog::endnoteSetup(bool on)
{
    widget.location_endnote->setEnabled(on);
    widget.dockWidget_5->setHidden(on);
    widget.beginAtCombo->setDisabled(on);
    if(on) {
        widget.numStyleCombo->setCurrentIndex(3);
        widget.paragraphCombo->setCurrentIndex(3);
        widget.pageCombo->setCurrentIndex(8);
        widget.textareaCombo->setCurrentIndex(5);
        widget.noteareaCombo->setCurrentIndex(6);
    }
}

void NotesConfigurationDialog::apply(QAbstractButton *button)
{
    if(widget.buttonBox->standardButton(button) == widget.buttonBox->Apply) {
        qDebug()<<"inside apply notesconfiguration";
        notesConfig = new KoOdfNotesConfiguration();
        //set Note Class
        if(widget.footnote->isChecked())
            notesConfig->setNoteClass(KoOdfNotesConfiguration::Footnote);
        else notesConfig->setNoteClass(KoOdfNotesConfiguration::Endnote);
        //set Number Format
        KoOdfNumberDefinition *numFormat = new KoOdfNumberDefinition();
        switch(widget.numStyleCombo->currentIndex()) {
        case 0:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::Numeric);
            notesConfig->setNumberFormat(*numFormat);
            break;
        case 1:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::AlphabeticLowerCase);
            notesConfig->setNumberFormat(*numFormat);
            notesConfig->numberFormat().setLetterSynchronization(false);
            break;
        case 2:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::AlphabeticUpperCase);
            notesConfig->setNumberFormat(*numFormat);
            notesConfig->numberFormat().setLetterSynchronization(false);
            break;
        case 3:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::AlphabeticLowerCase);
            notesConfig->setNumberFormat(*numFormat);
            notesConfig->numberFormat().setLetterSynchronization(true);
            break;
        case 4:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::AlphabeticUpperCase);
            notesConfig->setNumberFormat(*numFormat);
            notesConfig->numberFormat().setLetterSynchronization(true);
            break;
        case 5:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::RomanLowerCase);
            notesConfig->setNumberFormat(*numFormat);
            break;
        case 6:
            numFormat->setFormatSpecification(KoOdfNumberDefinition::RomanUpperCase);
            notesConfig->setNumberFormat(*numFormat);
            break;
        };
        //set Foot notes Position
        if(notesConfig->noteClass() == KoOdfNotesConfiguration::Footnote) {
            switch(widget.location_footnote->currentIndex()) {
            case 0:
                notesConfig->setFootnotesPosition(KoOdfNotesConfiguration::Page);
            case 1:
                notesConfig->setFootnotesPosition(KoOdfNotesConfiguration::Text);
            case 2:
                notesConfig->setFootnotesPosition(KoOdfNotesConfiguration::Section);
            case 3:
                notesConfig->setFootnotesPosition(KoOdfNotesConfiguration::Document);

            }
        }
        //set start value
        notesConfig->setStartValue(widget.startAtSpinBox->value());
        //set Numbering Scheme
        switch(widget.beginAtCombo->currentIndex()) {
        case 0:
            notesConfig->setNumberingScheme(KoOdfNotesConfiguration::BeginAtDocument);
            break;
        case 1:
            notesConfig->setNumberingScheme(KoOdfNotesConfiguration::BeginAtPage);
            break;
        case 2:
            notesConfig->setNumberingScheme(KoOdfNotesConfiguration::BeginAtChapter);
            break;
        }
        //set prefix
        notesConfig->numberFormat().setPrefix(widget.prefixLineEdit->text());
        //set suffix
        notesConfig->numberFormat().setSuffix(widget.suffixLineEdit->text());
        //set footnote continuation forward
        notesConfig->setFootnoteContinuationForward(widget.endlineEdit->text());
        //set footnote continuation backward
        notesConfig->setFootnoteContinuationBackward(widget.startlineEdit->text());

        //TODO
        //set citation text style

        //set citation body text style

        //set master page

        //set note paragraph style
        this->close();
    }
    else if(widget.buttonBox->standardButton(button) == widget.buttonBox->Discard) {
        this->close();
        qDebug()<<"discarding changes";
    }

}

#include <NotesConfigurationDialog.moc>
