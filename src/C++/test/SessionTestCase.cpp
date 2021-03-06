/****************************************************************************
** Copyright (c) quickfixengine.org  All rights reserved.
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 )
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <UnitTest++.h>
#include <Session.h>
#include <Responder.h>
#include <Acceptor.h>
#include <Values.h>
#include <SessionID.h>
#include <TimeRange.h>
#include <Message.h>
#include <fix42/Logon.h>
#include <fix42/Logout.h>
#include <fix42/Heartbeat.h>
#include <fix42/TestRequest.h>
#include <fix42/SequenceReset.h>
#include <fix42/ResendRequest.h>
#include <fix42/Reject.h>
#include <fix42/NewOrderSingle.h>
#include <fix42/ExecutionReport.h>

using namespace FIX;

SUITE(SessionTests)
{

class TestCallback : public Responder, public NullApplication
{
protected:
  UtcTimeOnly startTime;
  UtcTimeOnly endTime;
  FIX::Message lastResent;

public:
  TestCallback()
  : endTime( startTime ),
    toLogon( 0 ),
    toResendRequest( 0 ),
    toHeartbeat( 0 ),
    toLogout( 0 ),
    toReject( 0 ),
    toSequenceReset( 0 ),
    toBusinessMessageReject( 0 ),
    fromHeartbeat( 0 ),
    fromTestRequest( 0 ),
    fromLogout( 0 ),
    fromReject( 0 ),
    fromSequenceReset( 0 ),
    resent( 0 ),
    disconnected( 0 )
    {}

  bool send( const std::string& ) { return true; }

  void toAdmin( FIX::Message& message, const SessionID& )
  {
    MsgType msgType;
    message.getHeader().getField( msgType );
    switch ( msgType.getValue() [ 0 ] )
    {
      case 'A':
      toLogon++; sentLogon = message; break;
      case '2':
      toResendRequest++;
      sentResendRequest = message; break;
      case '0':
      toHeartbeat++; sentHeartbeat = message; break;
      case '5':
      toLogout++; break;
      case '3':
      toReject++; break;
      case '4':
      toSequenceReset++; break;
    }
  }

  void fromAdmin( const FIX::Message& message, const SessionID& )
  throw( FieldNotFound, IncorrectDataFormat, IncorrectTagValue, RejectLogon )
  {
    MsgType msgType;
    message.getHeader().getField( msgType );
    switch ( msgType.getValue() [ 0 ] )
    {
      case '0':
      fromHeartbeat++; break;
      case '1':
      fromTestRequest++; break;
      case '5':
      fromLogout++; break;
      case '3':
      fromReject++; break;
      case '4':
      fromSequenceReset++; break;
    }
  }

  void fromApp( const FIX::Message& message, const SessionID& )
  throw( FieldNotFound, IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType )
  {
    MsgType msgType;
    message.getHeader().getField( msgType );
    if ( msgType == "8" )
      throw UnsupportedMessageType();
  }

  void toApp( FIX::Message& message, const SessionID& )
  throw( DoNotSend )
  {
    PossDupFlag possDupFlag(false);
    if( message.getHeader().isSetField(possDupFlag) )
      message.getHeader().getField( possDupFlag );
    if( possDupFlag ) 
    {
      resent++;
      lastResent = message;
    }

    MsgType msgType;
    message.getHeader().getField( msgType );
    switch ( msgType.getValue() [ 0 ] )
    {
      case 'j':
      toBusinessMessageReject++; break;
    }
  }

  void disconnect() { disconnected++; }

  FIX::Message sentLogon;
  FIX::Message sentResendRequest;
  FIX::Message sentHeartbeat;
  int toLogon;
  int toResendRequest;
  int toHeartbeat;
  int toLogout;
  int toReject;
  int toSequenceReset;
  int toBusinessMessageReject;
  int fromHeartbeat;
  int fromTestRequest;
  int fromLogout;
  int fromReject;
  int fromSequenceReset;
  int resent;
  int disconnected;

  MemoryStoreFactory factory;
};

 void fillHeader( FIX::Header& header, const char* sender, const char* target, int seq )
{
  header.setField( SenderCompID( sender ) );
  header.setField( TargetCompID( target ) );
  header.setField( SendingTime() );
  header.setField( MsgSeqNum( seq ) );
}

FIX42::Logon createLogon( const char* sender, const char* target, int seq )
{
  FIX42::Logon logon;
  logon.set( EncryptMethod( 0 ) );
  logon.set( HeartBtInt( 30 ) );
  fillHeader( logon.getHeader(), sender, target, seq );
  return logon;
}

FIX42::Logout createLogout( const char* sender, const char* target, int seq )
{
  FIX42::Logout logout;
  fillHeader( logout.getHeader(), sender, target, seq );
  return logout;
}

FIX42::Heartbeat createHeartbeat( const char* sender, const char* target, int seq )
{
  FIX42::Heartbeat heartbeat;
  fillHeader( heartbeat.getHeader(), sender, target, seq );
  return heartbeat;
}

FIX42::TestRequest createTestRequest( const char* sender, const char* target, int seq, const char* id )
{
  FIX42::TestRequest testRequest;
  testRequest.set( TestReqID( id ) );
  fillHeader( testRequest.getHeader(), sender, target, seq );
  return testRequest;
}

FIX42::SequenceReset createSequenceReset( const char* sender, const char* target, int seq, int newSeq )
{
  FIX42::SequenceReset sequenceReset;
  sequenceReset.set( NewSeqNo( newSeq ) );
  fillHeader( sequenceReset.getHeader(), sender, target, seq );
  return sequenceReset;
}

FIX42::ResendRequest createResendRequest( const char* sender, const char* target, int seq, int begin, int end )
{
  FIX42::ResendRequest resendRequest;
  resendRequest.set( BeginSeqNo( begin ) );
  resendRequest.set( EndSeqNo( end ) );
  fillHeader( resendRequest.getHeader(), sender, target, seq );
  return resendRequest;
}

FIX42::Reject createReject( const char* sender, const char* target, int seq, int refSeq )
{
  FIX42::Reject reject;
  reject.set( RefSeqNum( refSeq ) );
  fillHeader( reject.getHeader(), sender, target, seq );
  return reject;
}

FIX42::NewOrderSingle createNewOrderSingle( const char* sender, const char* target, int seq )
{
  FIX42::NewOrderSingle newOrderSingle
    ( ClOrdID("ID"), HandlInst('1'), Symbol("SYMBOL"), Side(Side_BUY), TransactTime(), OrdType(OrdType_MARKET) );
  fillHeader( newOrderSingle.getHeader(), sender, target, seq );
  return newOrderSingle;
}

FIX42::ExecutionReport createExecutionReport( const char* sender, const char* target, int seq )
{
  FIX42::ExecutionReport executionReport( OrderID("ID"), ExecID("ID"), ExecTransType('0'), ExecType('0'), OrdStatus('0'), Symbol("SYMBOL"), Side(Side_BUY), LeavesQty(100), CumQty(0), AvgPx(0) );
  fillHeader( executionReport.getHeader(), sender, target, seq );
  FIX42::ExecutionReport::NoContraBrokers noContraBrokers;
  noContraBrokers.set( ContraBroker("BROKER") );
  noContraBrokers.set( ContraTrader("TRADER") );
  noContraBrokers.set( ContraTradeQty(100) );
  noContraBrokers.set( ContraTradeTime() );
  executionReport.addGroup( noContraBrokers );
  noContraBrokers.set( ContraBroker("BROKER2") );
  noContraBrokers.set( ContraTrader("TRADER2") );
  noContraBrokers.set( ContraTradeQty(100) );
  noContraBrokers.set( ContraTradeTime() );
  executionReport.addGroup( noContraBrokers );
  return executionReport;
}

struct sessionFixture : public TestCallback
{
  sessionFixture() 
  {
    object = 0;
  }

  sessionFixture( int heartBtInt )
  {
    object = 0;
    createSession( heartBtInt );
  }

  ~sessionFixture()
  {
    if( object )
      delete object;
  }

  virtual void createSession( int heartBtInt )
  {
    if( object )
      delete object;

    SessionID sessionID( BeginString( "FIX.4.2" ),
                         SenderCompID( "TW" ), TargetCompID( "ISLD" ) );
    TimeRange sessionTime( startTime, endTime );

    object = new Session( *this, factory, sessionID, DataDictionary("../spec/FIX42.xml"),
                           sessionTime, heartBtInt, 0 );
    object->setResponder( this );
  }

  Session* object;
  MemoryStoreFactory factory;
};

struct initiatorFixture : public sessionFixture
{
  initiatorFixture() : sessionFixture( 1 )
  {
  }
};

struct acceptorFixture : public sessionFixture
{
  acceptorFixture() : sessionFixture( 0 )
  {
  }
};

TEST_FIXTURE(acceptorFixture, nextLogon)
{
  // send with an incorrect SenderCompID
  object->setResponder( this );
  object->next( createLogon( "BLAH", "TW", 1 ) );
  CHECK( !object->receivedLogon() );
  CHECK_EQUAL( 0, toLogon );
  CHECK_EQUAL( 1, disconnected );

  // send with an incorrect TargetCompID
  object->setResponder( this );
  object->next( createLogon( "ISLD", "BLAH", 1 ) );
  CHECK( !object->receivedLogon() );
  CHECK_EQUAL( 0, toLogon );
  CHECK_EQUAL( 2, disconnected );

  // send a correct logon
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK( object->receivedLogon() );
  CHECK_EQUAL( 1, toLogon );
  CHECK_EQUAL( 2, disconnected );

  // check that we got a valid logon response
  SenderCompID senderCompID;
  TargetCompID targetCompID;
  HeartBtInt heartBtInt;
  EncryptMethod encryptMethod;

  sentLogon.getHeader().getField( senderCompID );
  sentLogon.getHeader().getField( targetCompID );
  sentLogon.getField( heartBtInt );
  sentLogon.getField( encryptMethod );

  CHECK_EQUAL( "TW", senderCompID );
  CHECK_EQUAL( "ISLD", targetCompID );
  CHECK_EQUAL( 30, heartBtInt );
  CHECK_EQUAL( 0, encryptMethod );
}

TEST_FIXTURE(acceptorFixture, nextLogonNoEncryptMethod)
{
  // send a correct logon
  object->setResponder( this );
  FIX42::Logon logon = createLogon( "ISLD", "TW", 1 );
  object->next( logon );
  CHECK( object->receivedLogon() );
  CHECK_EQUAL( 1, toLogon );
  CHECK_EQUAL( 0, disconnected );

  // check that we got a valid logon response
  SenderCompID senderCompID;
  TargetCompID targetCompID;
  HeartBtInt heartBtInt;
  EncryptMethod encryptMethod;
 
  sentLogon.getHeader().getField( senderCompID );
  sentLogon.getHeader().getField( targetCompID );
  sentLogon.getField( heartBtInt );
  sentLogon.getField( encryptMethod );

  CHECK_EQUAL( "TW", senderCompID );
  CHECK_EQUAL( "ISLD", targetCompID );
  CHECK_EQUAL( 30, heartBtInt );
  CHECK_EQUAL( 0, encryptMethod );
}

TEST_FIXTURE(acceptorFixture, nextLogonResetSeqNumFlag)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  object->next( createLogout( "ISLD", "TW", 2 ) );
  CHECK( !object->receivedLogon() );
  CHECK_EQUAL( 1, disconnected );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, fromLogout );
  CHECK_EQUAL( 3, object->getExpectedSenderNum() );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );

  FIX42::Logon logon = createLogon( "ISLD", "TW", 1 );
  logon.set( FIX::ResetSeqNumFlag(true) );
  object->next( logon );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );
}

TEST_FIXTURE(acceptorFixture, notifyResendRequest)
{
  object->next( createLogon( "ISLD", "TW", 5 ) );
  CHECK_EQUAL( 1, toResendRequest );
}

TEST_FIXTURE(acceptorFixture, incrMsgSeqNum)
{
  CHECK_EQUAL( 1, object->getExpectedSenderNum() );
  CHECK_EQUAL( 1, object->getExpectedTargetNum() );

  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );

  object->next( createHeartbeat( "ISLD", "TW", 2 ) );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );

  object->next( createHeartbeat( "ISLD", "TW", 3 ) );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 4, object->getExpectedTargetNum() );
}

TEST_FIXTURE(acceptorFixture, callDisconnect)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 0, disconnected );

  object->next( createHeartbeat( "ISLD", "TW", 2 ) );
  CHECK_EQUAL( 0, disconnected );
  CHECK_EQUAL( 1, fromHeartbeat );

  object->next( createHeartbeat( "ISLD", "TW", 3 ) );
  CHECK_EQUAL( 0, disconnected );
  CHECK_EQUAL( 2, fromHeartbeat );

  // message is dupicate
  FIX42::Heartbeat heartbeat = createHeartbeat( "ISLD", "TW", 2 );
  heartbeat.getHeader().setField( PossDupFlag( true ) );

  UtcTimeStamp timeStamp;
  timeStamp.setSecond( 5 );
  OrigSendingTime origSendingTime( timeStamp );
  timeStamp.setSecond( 10 );
  SendingTime sendingTime( timeStamp );

  // message is a possible dup, remain connected
  heartbeat.getHeader().setField( sendingTime );
  heartbeat.getHeader().setField( origSendingTime );
  object->next( heartbeat );
  CHECK_EQUAL( 0, disconnected );
  CHECK_EQUAL( 2, fromHeartbeat );

  // message is not a possible dup, disconnected
  heartbeat.getHeader().setField( PossDupFlag( false ) );
  object->next( heartbeat );
  CHECK_EQUAL( 1, disconnected );
  CHECK_EQUAL( 2, fromHeartbeat );
}

TEST_FIXTURE(sessionFixture, doesSessionExist)
{
  Session * pSession1 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "TW" ), TargetCompID( "ISLD" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession2 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "WT" ), TargetCompID( "ISLD" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession3 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "TW" ), TargetCompID( "DLSI" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession4 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "OREN" ), TargetCompID( "NERO" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession5 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "OREN" ), TargetCompID( "NERO" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );

  pSession1->setResponder( this );
  pSession2->setResponder( this );
  pSession3->setResponder( this );
  pSession4->setResponder( this );
  pSession5->setResponder( this );

  CHECK( Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "TW" ),
                       TargetCompID( "ISLD" ) ) ) );
  CHECK( Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "WT" ),
                       TargetCompID( "ISLD" ) ) ) );
  CHECK( Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "TW" ),
                       TargetCompID( "DLSI" ) ) ) );
  CHECK( Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "OREN" ),
                       TargetCompID( "NERO" ) ) ) );

  CHECK_EQUAL( 4, Session::numSessions() );

  delete pSession1;

  CHECK( !Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ),
                       SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK_EQUAL( 3, Session::numSessions() );

  delete pSession2;
  delete pSession3;

  CHECK_EQUAL( 1, Session::numSessions() );
  CHECK( Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "OREN" ),
                       TargetCompID( "NERO" ) ) ) );

  delete pSession4;
  CHECK_EQUAL( 0, Session::numSessions() );
  CHECK( !Session::doesSessionExist
          ( SessionID( BeginString( "FIX.4.2" ), SenderCompID( "OREN" ),
                       TargetCompID( "NERO" ) ) ) );

  delete pSession5;
}

TEST_FIXTURE(sessionFixture, lookupSession)
{
  Session* pSession1 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "TW" ), TargetCompID( "ISLD" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession2 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "WT" ), TargetCompID( "ISLD" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession3 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "TW" ), TargetCompID( "DLSI" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession4 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "OREN" ), TargetCompID( "NERO" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );
  Session* pSession5 = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                                   SenderCompID( "OREN" ), TargetCompID( "NERO" ) ), DataDictionary(),
                                   TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );

  pSession1->setResponder( this );
  pSession2->setResponder( this );
  pSession3->setResponder( this );
  pSession4->setResponder( this );
  pSession5->setResponder( this );

  CHECK_EQUAL( pSession1,
               Session::lookupSession( SessionID( BeginString( "FIX.4.2" ),
                                       SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );

  CHECK_EQUAL( pSession2,
               Session::lookupSession( SessionID( BeginString( "FIX.4.2" ),
                                       SenderCompID( "WT" ), TargetCompID( "ISLD" ) ) ) );

  CHECK_EQUAL( pSession3,
               Session::lookupSession( SessionID( BeginString( "FIX.4.2" ),
                                       SenderCompID( "TW" ), TargetCompID( "DLSI" ) ) ) );

  CHECK_EQUAL( pSession4,
               Session::lookupSession( SessionID( BeginString( "FIX.4.2" ),
                                       SenderCompID( "OREN" ), TargetCompID( "NERO" ) ) ) );

  delete pSession1; 
  delete pSession2; 
  delete pSession3;
  delete pSession4; 
  delete pSession5;
}

TEST_FIXTURE(sessionFixture, registerSession)
{
  Session* pSession = new Session
    ( *this, factory, SessionID( BeginString( "FIX.4.2" ),
                        SenderCompID( "TW" ), TargetCompID( "ISLD" ) ), DataDictionary(),
                        TimeRange(UtcTimeOnly(), UtcTimeOnly()), 0, 0 );

  CHECK_EQUAL( (Session*)0, Session::registerSession( SessionID( BeginString( "FIX.4.1" ),
                                                      SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK_EQUAL( pSession, Session::registerSession( SessionID( BeginString( "FIX.4.2" ),
                                                   SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK( Session::isSessionRegistered( SessionID( BeginString( "FIX.4.2" ),
                                       SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK_EQUAL( (Session*)0, Session::registerSession( SessionID( BeginString( "FIX.4.2" ),
                                                      SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  Session::unregisterSession( SessionID( BeginString( "FIX.4.2" ),
                                         SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) );
  CHECK( !Session::isSessionRegistered( SessionID( BeginString( "FIX.4.2" ),
                                        SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK_EQUAL( pSession, Session::registerSession( SessionID( BeginString( "FIX.4.2" ),
                                                   SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  delete pSession;
  CHECK( !Session::isSessionRegistered( SessionID( BeginString( "FIX.4.2" ),
                                        SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
  CHECK_EQUAL( (Session*)0, Session::registerSession( SessionID( BeginString( "FIX.4.2" ),
                                                      SenderCompID( "TW" ), TargetCompID( "ISLD" ) ) ) );
}

TEST_FIXTURE(acceptorFixture, nextTestRequest)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );

  object->next( createTestRequest( "ISLD", "TW", 2, "HELLO" ) );
  CHECK_EQUAL( 1, fromTestRequest );
  CHECK_EQUAL( 1, toHeartbeat );

  TestReqID testReqID;
  sentHeartbeat.getField( testReqID );
  CHECK_EQUAL( "HELLO", testReqID );
}

TEST_FIXTURE(acceptorFixture, outOfOrder)
{
  CHECK_EQUAL( 1, object->getExpectedSenderNum() );
  CHECK_EQUAL( 1, object->getExpectedTargetNum() );

  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );

  object->next( createHeartbeat( "ISLD", "TW", 3 ) );
  CHECK_EQUAL( 3, object->getExpectedSenderNum() );
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );
  CHECK_EQUAL( 0, fromHeartbeat );

  object->next( createHeartbeat( "ISLD", "TW", 2 ) );
  CHECK_EQUAL( 3, object->getExpectedSenderNum() );
  CHECK_EQUAL( 4, object->getExpectedTargetNum() );
  CHECK_EQUAL( 2, fromHeartbeat );
}

TEST_FIXTURE(acceptorFixture, nextLogout)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  object->next( createLogout( "ISLD", "TW", 2 ) );
  CHECK( !object->receivedLogon() );
  CHECK_EQUAL( 1, disconnected );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, fromLogout );
}

TEST_FIXTURE(initiatorFixture, logoutInitiator)
{
  object->setResponder( this );
  object->next();
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 2, object->getExpectedSenderNum() );
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );

  object->next( createLogout( "ISLD", "TW", 2 ) );
  CHECK_EQUAL( 3, object->getExpectedSenderNum() );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );
}

TEST_FIXTURE(acceptorFixture, badOrigSendingTime)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  UtcTimeStamp timeStamp;
  timeStamp.setSecond( 10 );
  SendingTime sendingTime( timeStamp );
  timeStamp.setSecond( 20 );
  OrigSendingTime origSendingTime( timeStamp );

  object->next( createNewOrderSingle( "ISLD", "TW", 2 ) );
  object->next( createNewOrderSingle( "ISLD", "TW", 3 ) );

  FIX42::NewOrderSingle newOrderSingle = createNewOrderSingle( "ISLD", "TW", 2 );
  newOrderSingle.getHeader().setField( sendingTime );
  newOrderSingle.getHeader().setField( origSendingTime );
  newOrderSingle.getHeader().setField( PossDupFlag( true ) );
  object->next( newOrderSingle );
  CHECK_EQUAL( 1, toReject );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 0, disconnected );

  object->next( createLogout( "ISLD", "TW", 4 ) );
  CHECK_EQUAL( 1, disconnected );
  CHECK_EQUAL( 1, toLogout );
}

TEST_FIXTURE(acceptorFixture, noOrigSendingTime)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  UtcTimeStamp timeStamp;
  timeStamp.setSecond( 10 );
  SendingTime sendingTime( timeStamp );

  object->next( createNewOrderSingle( "ISLD", "TW", 2 ) );
  object->next( createNewOrderSingle( "ISLD", "TW", 3 ) );

  FIX42::NewOrderSingle newOrderSingle = createNewOrderSingle( "ISLD", "TW", 2 );
  newOrderSingle.getHeader().setField( sendingTime );
  newOrderSingle.getHeader().setField( PossDupFlag( true ) );
  object->next( newOrderSingle );
  CHECK_EQUAL( 1, toReject );
  CHECK_EQUAL( 0, toLogout );
  CHECK_EQUAL( 0, disconnected );

  object->next( createLogout( "ISLD", "TW", 4 ) );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, disconnected );
}

TEST_FIXTURE(acceptorFixture, badCompID)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  object->next( createNewOrderSingle( "ISLD", "WT", 3 ) );
  CHECK_EQUAL( 1, toReject );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 0, disconnected );

  object->next( createLogout( "ISLD", "TW", 4 ) );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, disconnected );
}

TEST_FIXTURE(acceptorFixture, nextReject)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createTestRequest( "ISLD", "TW", 2, "HELLO" ) );

  object->next( createReject( "ISLD", "TW", 3, 2 ) );
  CHECK_EQUAL( 1, fromReject );
  CHECK_EQUAL( 0, toReject );
  CHECK_EQUAL( 0, toLogout );
  CHECK_EQUAL( 0, disconnected );
  CHECK_EQUAL( 4, object->getExpectedTargetNum() );

  object->next( createHeartbeat( "ISLD", "TW", 4 ) );
  CHECK_EQUAL( 0, toResendRequest );
}

class MsgWithBadType : public FIX42::Message
{
public:
  MsgWithBadType() : FIX42::Message( MsgType( "*" ) ) {}
};

TEST_FIXTURE(acceptorFixture, badMsgType)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  MsgWithBadType msgWithBadType;
  fillHeader( msgWithBadType.getHeader(), "ISLD", "TW", 2 );

  object->next( msgWithBadType );

  CHECK_EQUAL( 1, toReject );
  CHECK_EQUAL( 0, disconnected );
  CHECK_EQUAL( 0, toLogout );

  object->next( createLogout( "ISLD", "TW", 3 ) );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, disconnected );
}

TEST_FIXTURE(acceptorFixture, nextSequenceReset)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );

  // NewSeqNo is greater
  object->next( createSequenceReset( "ISLD", "TW", 0, 3 ) );
  CHECK_EQUAL( 1, fromSequenceReset );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );
  CHECK_EQUAL( 0, toReject );

  // NewSeqNo is equal
  object->next( createSequenceReset( "ISLD", "TW", 0, 3 ) );
  CHECK_EQUAL( 2, fromSequenceReset );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );
  CHECK_EQUAL( 0, toReject );

  // No MsgSeqNum
  FIX42::SequenceReset sequenceReset = createSequenceReset( "ISLD", "TW", 0, 3 );
  sequenceReset.getHeader().removeField( 34 );
  object->next( sequenceReset );
  CHECK_EQUAL( 3, fromSequenceReset );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );
  CHECK_EQUAL( 0, toReject );

  // NewSeqNo is less
  object->next( createSequenceReset( "ISLD", "TW", 0, 2 ) );
  CHECK_EQUAL( 4, fromSequenceReset );
  CHECK_EQUAL( 3, object->getExpectedTargetNum() );
  CHECK_EQUAL( 1, toReject );
}

TEST_FIXTURE(acceptorFixture, nextGapFill)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  // NewSeqNo is equal
  
  FIX42::SequenceReset sequenceReset = createSequenceReset( "ISLD", "TW", 2, 20 );
  sequenceReset.set( GapFillFlag( true ) );
  object->next( sequenceReset );
  CHECK_EQUAL( 1, fromSequenceReset );
  CHECK_EQUAL( 0, toResendRequest );
  CHECK_EQUAL( 20, object->getExpectedTargetNum() );

  // NewSeqNo is greater
  FIX42::SequenceReset sequenceReset2 = createSequenceReset( "ISLD", "TW", 21, 40 );
  sequenceReset2.set( GapFillFlag( true ) );
  object->next( sequenceReset2 );
  CHECK_EQUAL( 1, fromSequenceReset );
  CHECK_EQUAL( 1, toResendRequest );
  CHECK_EQUAL( 20, object->getExpectedTargetNum() );

  // NewSeqNo is less, PossDupFlag = Y
  FIX42::SequenceReset sequenceReset3 = createSequenceReset( "ISLD", "TW", 19, 20 );
  sequenceReset3.set( GapFillFlag( true ) );
  sequenceReset3.getHeader().setField( PossDupFlag( true ) );
  sequenceReset3.getHeader().setField( OrigSendingTime() );
  object->next( sequenceReset3 );
  CHECK_EQUAL( 1, fromSequenceReset );
  CHECK_EQUAL( 1, toResendRequest );
  CHECK_EQUAL( 20, object->getExpectedTargetNum() );
  CHECK_EQUAL( 0, disconnected );

  // NewSeqNo is less, PossDupFlag = N
  FIX42::SequenceReset sequenceReset4 = createSequenceReset( "ISLD", "TW", 19, 20 );
  sequenceReset4.set( GapFillFlag( true ) );
  sequenceReset4.getHeader().setField( PossDupFlag( false ) );
  object->next( sequenceReset4 );
  CHECK_EQUAL( 1, fromSequenceReset );
  CHECK_EQUAL( 1, toResendRequest );
  CHECK_EQUAL( 20, object->getExpectedTargetNum() );
  CHECK_EQUAL( 1, disconnected );
}

TEST_FIXTURE(acceptorFixture, nextResendRequest)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createTestRequest( "ISLD", "TW", 2, "HELLO" ) );
  object->next( createTestRequest( "ISLD", "TW", 3, "HELLO" ) );
  object->next( createTestRequest( "ISLD", "TW", 4, "HELLO" ) );
  object->next( createResendRequest( "ISLD", "TW", 5, 1, 4 ) );
  CHECK_EQUAL( 1, toSequenceReset );
  CHECK_EQUAL( 0, resent );

  FIX::Message message = createNewOrderSingle( "ISLD", "TW", 6 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 7 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 8 );
  CHECK( object->send( message ) );
  object->next( createResendRequest( "ISLD", "TW", 6, 5, 7 ) );
  CHECK_EQUAL( 1, toSequenceReset );
  CHECK_EQUAL( 3, resent );

  object->setNextSenderMsgSeqNum(15);
  object->next( createResendRequest( "ISLD", "TW", 7, 8, 15 ) );
  CHECK_EQUAL( 2, toSequenceReset );
  CHECK_EQUAL( 3, resent );

  object->next( createResendRequest( "ISLD", "TW", 8, 1, 15 ) );
  CHECK_EQUAL( 4, toSequenceReset );
  CHECK_EQUAL( 6, resent );

  message = createNewOrderSingle( "ISLD", "TW", 16 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 17 );
  CHECK( object->send( message ) );
  object->next( createResendRequest( "ISLD", "TW", 8, 1, 20 ) );
  CHECK_EQUAL( 6, toSequenceReset );
  CHECK_EQUAL( 11, resent );
}

TEST_FIXTURE(acceptorFixture, nextResendRequestRepeatingGroup)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  FIX::Message message = createExecutionReport( "ISLD", "TW", 2 );
  CHECK( object->send( message ) );
  object->next( createResendRequest( "ISLD", "TW", 3, 2, 2 ) );

  PossDupFlag possDupFlag;
  OrigSendingTime origSendingTime;
  SendingTime sendingTime;
  lastResent.getHeader().getField( possDupFlag );
  lastResent.getHeader().getField( origSendingTime );
  lastResent.getHeader().getField( sendingTime );
  message.getHeader().setField( possDupFlag );
  message.getHeader().setField( origSendingTime );
  message.getHeader().setField( sendingTime );
  CHECK_EQUAL( message.toString(), lastResent.toString() );
}

TEST_FIXTURE(acceptorFixture, nextResendRequestNoMessagePersist)
{
  object->setPersistMessages( false );
  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createTestRequest( "ISLD", "TW", 2, "HELLO" ) );
  object->next( createTestRequest( "ISLD", "TW", 3, "HELLO" ) );
  object->next( createTestRequest( "ISLD", "TW", 4, "HELLO" ) );
  object->next( createResendRequest( "ISLD", "TW", 5, 1, 4 ) );
  CHECK_EQUAL( 1, toSequenceReset );
  CHECK_EQUAL( 0, resent );

  FIX::Message message = createNewOrderSingle( "ISLD", "TW", 6 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 7 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 8 );
  CHECK( object->send( message ) );
  object->next( createResendRequest( "ISLD", "TW", 6, 5, 7 ) );
  CHECK_EQUAL( 2, toSequenceReset );
  CHECK_EQUAL( 0, resent );

  object->setNextSenderMsgSeqNum(15);
  object->next( createResendRequest( "ISLD", "TW", 7, 8, 15 ) );
  CHECK_EQUAL( 3, toSequenceReset );
  CHECK_EQUAL( 0, resent );

  object->next( createResendRequest( "ISLD", "TW", 8, 1, 15 ) );
  CHECK_EQUAL( 4, toSequenceReset );
  CHECK_EQUAL( 0, resent );

  message = createNewOrderSingle( "ISLD", "TW", 16 );
  CHECK( object->send( message ) );
  message = createNewOrderSingle( "ISLD", "TW", 17 );
  CHECK( object->send( message ) );
  object->next( createResendRequest( "ISLD", "TW", 8, 1, 20 ) );
  CHECK_EQUAL( 5, toSequenceReset );
  CHECK_EQUAL( 0, resent );
}

TEST_FIXTURE(acceptorFixture, badBeginString)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  FIX42::TestRequest testRequest = createTestRequest( "ISLD", "TW", 2, "HELLO" );
  testRequest.getHeader().setField( BeginString( BeginString_FIX41 ) );
  object->next( testRequest );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 0, disconnected );

  FIX42::Logout logout = createLogout( "ISLD", "TW", 3 );
  logout.getHeader().setField( BeginString( BeginString_FIX41 ) );
  object->next( logout );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, disconnected );
}

TEST_FIXTURE(acceptorFixture, unsupportedMsgType)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );

  FIX42::ExecutionReport message = createExecutionReport( "ISLD", "TW", 2 );
  object->next( message );
  CHECK_EQUAL( 1, toBusinessMessageReject );
}

struct resetOnEndTimeFixture : public acceptorFixture
{
  void createSession()
  {
    startTime.setCurrent();
    startTime.setMillisecond(0);

    endTime.setCurrent();
    endTime.setMillisecond(0);

    endTime += 2;

    acceptorFixture::createSession( 0 );
  }
};

TEST_FIXTURE(resetOnEndTimeFixture, resetOnEndTime)
{
  createSession();
  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createHeartbeat( "ISLD", "TW", 2 ) );

  CHECK_EQUAL( 1, toLogon );
  CHECK_EQUAL( 0, disconnected );
  process_sleep( 1 );
  object->next();
  CHECK_EQUAL( 0, disconnected );
  process_sleep( 2 );
  object->next();
  CHECK_EQUAL( 1, disconnected );
  CHECK_EQUAL( 1, toLogout );
  CHECK_EQUAL( 1, object->getExpectedSenderNum() );
  CHECK_EQUAL( 1, object->getExpectedTargetNum() );
}

struct disconnectBeforeStartTimeFixture : public acceptorFixture
{
  disconnectBeforeStartTimeFixture()
  {
    startTime.setCurrent();
    startTime += 100;
    endTime.setCurrent();
    endTime += 400;

    acceptorFixture::createSession( 0 );
  }
};

TEST_FIXTURE(disconnectBeforeStartTimeFixture, disconnectedBeforeStartTime)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 1, disconnected );

  process_sleep( 2 );
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 1, disconnected );
}

struct resetOnNewSessionFixture : public acceptorFixture
{
  resetOnNewSessionFixture()
  {
    startTime.setCurrent();
    endTime = startTime;
    endTime += 2;
    startTime += -2;

    acceptorFixture::createSession( 0 );
  }
};

TEST_FIXTURE(resetOnNewSessionFixture, resetOnNewSession)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 0, disconnected );

  process_sleep( 3 );
  object->next();
  CHECK_EQUAL( 1, disconnected );
}

struct logonAtLogonStartTimeFixture : public acceptorFixture
{
  logonAtLogonStartTimeFixture()
  {
    startTime.setCurrent();
    startTime.setMillisecond(0);

    endTime.setCurrent();
    endTime.setMillisecond(0);

    startTime += -10;
    endTime += 10;

    UtcTimeOnly logonStartTime;
    UtcTimeOnly logonEndTime;
    logonStartTime += 1;
    logonEndTime += 2;
    TimeRange logonTime( logonStartTime, logonEndTime );

    object->setLogonTime( logonTime );
  }
};

TEST_FIXTURE(logonAtLogonStartTimeFixture, logonAtLogonStartTime)
{
  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createHeartbeat( "ISLD", "TW", 2 ) );

  CHECK_EQUAL( 0, toLogon );
  CHECK_EQUAL( 0, toLogout );
  process_sleep( 1 );

  object->next( createLogon( "ISLD", "TW", 1 ) );
  object->next( createHeartbeat( "ISLD", "TW", 2 ) );
  object->next();
  CHECK_EQUAL( 1, toLogon );
  CHECK_EQUAL( 0, toLogout );

  process_sleep( 2 );
  object->next();
  CHECK_EQUAL( 1, toLogon );
  CHECK_EQUAL( 1, toLogout );
}

TEST_FIXTURE(acceptorFixture, processQueuedMessages)
{
  object->setResponder( this );
  object->next( createLogon( "ISLD", "TW", 1 ) );
  CHECK_EQUAL( 0, disconnected );

  for( int i = 3; i <= 5003; ++i )
  {
    object->next( createNewOrderSingle( "ISLD", "TW", i ) );
  }
  CHECK_EQUAL( 2, object->getExpectedTargetNum() );
  CHECK_EQUAL( 1, toResendRequest );
  object->next( createNewOrderSingle( "ISLD", "TW", 2 ) );
  CHECK_EQUAL( 5004, object->getExpectedTargetNum() );
  object->next( createNewOrderSingle( "ISLD", "TW", 5006 ) );
  CHECK_EQUAL( 2, toResendRequest );
}

}
