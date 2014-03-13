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
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include <gtest/gtest.h>

#include "../src/QtGoogleAnalytics.h"

#include "testnetworkaccessmanager.h"

TEST(Tracker, setNetworkAccessManager)
{
    // Tests that we can a network manager to use
    TestNetworkAccessManager nam;
    QtGoogleAnalyticsTracker tracker;
    // 1. network access manager cannot be nulled
    tracker.setNetworkAccessManager( nullptr );
    EXPECT_NE( nullptr, tracker.networkAccessManager() );
    // 2. network access manager can be set to valid values
    tracker.setNetworkAccessManager( &nam );
    EXPECT_EQ( &nam, tracker.networkAccessManager() );
}

TEST(Tracker, trackingID)
{
    QtGoogleAnalyticsTracker tracker;
    QString empty;

    // 1. Proper initialization
    EXPECT_EQ( empty, tracker.trackingID() );

    // 2. a bunch of invalid tracking IDs
    tracker.setTrackingID( QString( "WT-1234-56" ) );
    EXPECT_EQ( empty, tracker.trackingID() );
    tracker.setTrackingID( QString( "-1234-56" ) );
    EXPECT_EQ( empty, tracker.trackingID() );
    tracker.setTrackingID( QString( "UA--12" ) );
    EXPECT_EQ( empty, tracker.trackingID() );
    tracker.setTrackingID( QString( "YT-1-" ) );
    EXPECT_EQ( empty, tracker.trackingID() );
    tracker.setTrackingID( QString( "-123-" ) );
    EXPECT_EQ( empty, tracker.trackingID() );
    tracker.setTrackingID( QString( "--1234" ) );

    // 3. a bunch of valid tracking IDs
    QStringList expectedTrackingIDs;
    expectedTrackingIDs << "UA-1234-12" \
                        << "YT-1234-12" \
                        << "MO-1-1" \
                        << "UA-1234-1" \
                        << "ua-1-1";
    Q_FOREACH( QString expected, expectedTrackingIDs )
    {
        tracker.setTrackingID( expected );
        EXPECT_EQ( expected, tracker.trackingID() );
    }
}

TEST(Tracker, track)
{
    // test that we can actually track stuff using QtGoogleAnalytics
    TestNetworkAccessManager nam;
    QNetworkRequest expectedRequest;
    QtGoogleAnalyticsTracker tracker;
    QSignalSpy spy( &tracker, SIGNAL( tracked() ) );

    expectedRequest.setUrl( QtGoogleAnalyticsTracker::NormalEndpoint );
    nam.setExpectedRequest( &expectedRequest );

    tracker.setNetworkAccessManager( &nam );
    tracker.track();

    spy.wait();

    EXPECT_EQ( 1, spy.count() );
    EXPECT_FALSE( nam.failed() );
}

int main(int argc, char** argv)
{
    QCoreApplication app( argc, argv );
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
