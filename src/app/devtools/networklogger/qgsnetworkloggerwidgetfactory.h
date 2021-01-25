/***************************************************************************
    qgsnetworkloggerwidgetfactory.h
    -------------------------
    begin                : March 2020
    copyright            : (C) 2020 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSNETWORKLOGGERWIDGETFACTORY_H
#define QGSNETWORKLOGGERWIDGETFACTORY_H

#include "qgsdevtoolwidgetfactory.h"
#include "qgis_app.h"

class QgsNetworkLogger;

class APP_EXPORT QgsNetworkLoggerWidgetFactory: public QgsDevToolWidgetFactory
{
  public:

    QgsNetworkLoggerWidgetFactory( QgsNetworkLogger *logger );
    QgsDevToolWidget *createWidget( QWidget *parent = nullptr ) const override;

  private:

    QgsNetworkLogger *mLogger = nullptr;
};


#endif // QGSNETWORKLOGGERWIDGETFACTORY_H
