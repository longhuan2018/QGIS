/***************************************************************************
  qgsrendereditemresults.cpp
  -------------------
   begin                : August 2021
   copyright            : (C) Nyall Dawson
   email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrendereditemresults.h"
#include "qgsrendereditemdetails.h"
#include "qgsrendercontext.h"
#include "qgslogger.h"
#include "qgsrenderedannotationitemdetails.h"
#include "RTree.h"


///@cond PRIVATE
class QgsRenderedItemResultsSpatialIndex : public RTree<const QgsRenderedItemDetails *, float, 2, float>
{
  public:

    void insert( const QgsRenderedItemDetails *details, const QgsRectangle &bounds )
    {
      std::array< float, 4 > scaledBounds = scaleBounds( bounds );
      this->Insert(
      {
        scaledBounds[0], scaledBounds[ 1]
      },
      {
        scaledBounds[2], scaledBounds[3]
      },
      details );
    }

    /**
     * Performs an intersection check against the index, for data intersecting the specified \a bounds.
     *
     * The \a callback function will be called once for each matching data object encountered.
     */
    bool intersects( const QgsRectangle &bounds, const std::function< bool( const QgsRenderedItemDetails *details )> &callback ) const
    {
      std::array< float, 4 > scaledBounds = scaleBounds( bounds );
      this->Search(
      {
        scaledBounds[0], scaledBounds[ 1]
      },
      {
        scaledBounds[2], scaledBounds[3]
      },
      callback );
      return true;
    }

  private:
    std::array<float, 4> scaleBounds( const QgsRectangle &bounds ) const
    {
      return
      {
        static_cast< float >( bounds.xMinimum() ),
        static_cast< float >( bounds.yMinimum() ),
        static_cast< float >( bounds.xMaximum() ),
        static_cast< float >( bounds.yMaximum() )
      };
    }
};
///@endcond

QgsRenderedItemResults::QgsRenderedItemResults()
  : mAnnotationItemsIndex( std::make_unique< QgsRenderedItemResultsSpatialIndex >() )
{

}

QgsRenderedItemResults::~QgsRenderedItemResults() = default;

QList<QgsRenderedItemDetails *> QgsRenderedItemResults::renderedItems() const
{
  QList< QgsRenderedItemDetails * > res;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  res.reserve( static_cast< int >( mDetails.size() ) );
#else
  res.reserve( mDetails.size() );
#endif
  std::transform( mDetails.begin(), mDetails.end(), std::back_inserter( res ), []( const auto & detail ) {return detail.get();} );
  return res;
}

QList<const QgsRenderedAnnotationItemDetails *> QgsRenderedItemResults::renderedAnnotationItemsInBounds( const QgsRectangle &bounds ) const
{
  QList<const QgsRenderedAnnotationItemDetails *> res;

  mAnnotationItemsIndex->intersects( bounds, [&res]( const QgsRenderedItemDetails * details )->bool
  {
    res << qgis::down_cast< const QgsRenderedAnnotationItemDetails * >( details );
    return true;
  } );
  return res;
}

void QgsRenderedItemResults::appendResults( const QList<QgsRenderedItemDetails *> &results, const QgsRenderContext &context )
{
  const QgsCoordinateTransform layerToMapTransform = context.coordinateTransform();
  for ( QgsRenderedItemDetails *details : results )
  {
    try
    {
      const QgsRectangle transformedBounds = layerToMapTransform.transformBoundingBox( details->boundingBox() );
      details->setBoundingBox( transformedBounds );
    }
    catch ( QgsCsException & )
    {
      QgsDebugMsg( QStringLiteral( "Could not transform rendered item's bounds to map CRS" ) );
    }

    if ( QgsRenderedAnnotationItemDetails *annotationDetails = dynamic_cast< QgsRenderedAnnotationItemDetails * >( details ) )
      mAnnotationItemsIndex->insert( annotationDetails, annotationDetails->boundingBox() );

    mDetails.emplace_back( std::unique_ptr< QgsRenderedItemDetails >( details ) );
  }
}

void QgsRenderedItemResults::transferResults( QgsRenderedItemResults *other, const QStringList &layerIds )
{
  for ( auto it = other->mDetails.begin(); it != other->mDetails.end(); )
  {
    if ( layerIds.contains( ( *it )->layerId() ) )
    {
      if ( QgsRenderedAnnotationItemDetails *annotationDetails = dynamic_cast< QgsRenderedAnnotationItemDetails * >( ( *it ).get() ) )
        mAnnotationItemsIndex->insert( annotationDetails, annotationDetails->boundingBox() );

      mDetails.emplace_back( std::move( *it ) );
      other->mDetails.erase( it );
    }
    else
    {
      it++;
    }
  }
}

void QgsRenderedItemResults::transferResults( QgsRenderedItemResults *other )
{
  for ( auto it = other->mDetails.begin(); it != other->mDetails.end(); ++it )
  {
    if ( QgsRenderedAnnotationItemDetails *annotationDetails = dynamic_cast< QgsRenderedAnnotationItemDetails * >( ( *it ).get() ) )
      mAnnotationItemsIndex->insert( annotationDetails, annotationDetails->boundingBox() );

    mDetails.emplace_back( std::move( *it ) );
  }
  other->mDetails.clear();
}


