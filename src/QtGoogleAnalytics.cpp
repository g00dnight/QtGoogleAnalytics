/*
 * Copyright (c) 2014 Thomas Daehling <doc@methedrine.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "QtGoogleAnalytics.h"

#include <QtGlobal>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegExp>
#include <QUrlQuery>

using namespace QtGoogleAnalytics;

const QUrl Tracker::NormalEndpoint( "http://www.google-analytics.com/collect" );
const QUrl Tracker::SecureEndpoint( "https://ssl.google-analytics.com/collect" );
const QString Tracker::UserAgent( "QtGoogleAnalyticsTracker/1.0" );
const QString Tracker::DefaultClientID( "QtGoogleAnalytics" );
const QString Tracker::ProtocolVersion( "1" );

Tracker::Tracker( QObject *parent )
    : QObject( parent ), m_nam( new QNetworkAccessManager( this ) ), m_userAgent( UserAgent ),
      m_endpoint( NormalEndpoint ), m_clientID( DefaultClientID ),
      m_operation( QNetworkAccessManager::PostOperation ), m_cacheBusting( false )
{
    connectSignals();
}

/*!
 * \brief QtGoogleAnalyticsTracker::setNetworkAccessManager sets the QNetworkAccessManager that should be used by this tracker.
 *
 * Since a tracker requires a valid QNetworkAccessManager instance it will by default construct one of its own. However,
 * for some applications this may not be desirable, and as such the network manager to use can overriden using this method.
 *
 * \note If a nullptr is passed to this function the previously set QNetworkAccessManager will still be used.
 *
 * \sa networkAccessManager()
 */
void Tracker::setNetworkAccessManager( QNetworkAccessManager *nam )
{
    if ( ! nam )
    {
        return;
    }

    if ( m_nam && m_nam->parent() == this )
    {
        delete m_nam;
    }
    m_nam = nam;
    connectSignals();
}

QNetworkAccessManager* Tracker::networkAccessManager() const
{
    return m_nam;
}

void Tracker::track( const Tracker::ParameterList& parameters )
{
    QUrlQuery query;
    query.setQueryItems( parameters );
    query.addQueryItem( QString( "v" ), ProtocolVersion );
    query.addQueryItem( QString( "tid" ), m_trackingID );
    query.addQueryItem( QString( "cid" ), m_clientID );
    track( query );
}

void Tracker::track( const QUrlQuery& query )
{
    QNetworkRequest req;
    req.setHeader( QNetworkRequest::UserAgentHeader, m_userAgent );

    if ( m_operation == QNetworkAccessManager::PostOperation )
    {
        QByteArray data = query.toString( QUrl::FullyEncoded ).toLatin1();
        if ( data.size() > 8192 )
        {
            qWarning( "%d exceeds 8192 byte payload size limit for POST operations.", data.size() );
        }
        req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        req.setUrl( m_endpoint );

        m_replies.insert( m_nam->post( req, data ) );
    }
    else if ( m_operation == QNetworkAccessManager::GetOperation )
    {
        QUrl url = m_endpoint;
        QUrlQuery q = query;

        if ( m_cacheBusting )
        {
            q.addQueryItem( "z", QString::number( qrand() % 100000000 ) );
        }

        url.setQuery( q );
        req.setUrl( url );

        int size = url.toString( QUrl::FullyEncoded ).size();
        if ( size > 2000 )
        {
            qWarning( "%d exceeds 2000 byte payload size limit for GET operations.", size );
        }

        m_replies.insert( m_nam->get( req ) );
    }
}

void Tracker::connectSignals()
{
    connect( m_nam, SIGNAL( finished( QNetworkReply* ) ), this, SLOT( onFinished( QNetworkReply* ) ) );
}

void Tracker::onFinished( QNetworkReply *reply )
{
    if ( ! m_replies.contains( reply ) )
    {
        return;
    }

    if ( reply->error() != QNetworkReply::NoError )
    {
        qWarning( "Network reply finished with error: %s", qPrintable( reply->errorString() ) );
    }
    reply->deleteLater();
    m_replies.remove( reply );
    emit tracked();
}

void Tracker::setTrackingID( const QString& trackingID )
{
    m_trackingID.clear();
    QRegExp validTrackingID( "\\b(UA|YT|MO)-\\d+-\\d+\\b" );
    validTrackingID.setCaseSensitivity( Qt::CaseInsensitive );
    if ( validTrackingID.exactMatch( trackingID ) )
    {
        m_trackingID = trackingID;
    }
}

QString Tracker::trackingID() const
{
    return m_trackingID;
}

void Tracker::setUserAgent( const QString& userAgent )
{
    if ( ! userAgent.isEmpty() )
    {
        m_userAgent = userAgent;
    }
}

QString Tracker::userAgent() const
{
    return m_userAgent;
}

void Tracker::setEndpoint( const QUrl& endpoint )
{
    if ( endpoint.isValid() )
    {
        m_endpoint = endpoint;
    }
}

QUrl Tracker::endpoint() const
{
    return m_endpoint;
}

void Tracker::setClientID( const QString& clientID )
{
    // NOTE Google Analytics says that clientID SHOULD be a Uuidv4 but it accepts all texts
    if ( ! clientID.isEmpty() )
    {
        m_clientID = clientID;
    }
}

QString Tracker::clientID() const
{
    return m_clientID;
}

void Tracker::setOperation( QNetworkAccessManager::Operation op )
{
    switch( op )
    {
        case QNetworkAccessManager::PostOperation:
        case QNetworkAccessManager::GetOperation:
            m_operation = op;
            break;
        default:
            return;
    }
}

QNetworkAccessManager::Operation Tracker::operation() const
{
    return m_operation;
}

void Tracker::setCacheBusting( bool enabled )
{
    m_cacheBusting = enabled;
}

bool Tracker::cacheBusting() const
{
    return m_cacheBusting;
}