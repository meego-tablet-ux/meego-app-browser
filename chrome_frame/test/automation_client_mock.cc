// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome_frame/test/automation_client_mock.h"

#include "base/callback.h"
#include "net/base/net_errors.h"
#include "chrome_frame/test/chrome_frame_test_utils.h"

#define GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
#include "testing/gmock_mutant.h"

using testing::_;
using testing::CreateFunctor;
using testing::Return;

template <> struct RunnableMethodTraits<ProxyFactory::LaunchDelegate> {
  void RetainCallee(ProxyFactory::LaunchDelegate* obj) {}
  void ReleaseCallee(ProxyFactory::LaunchDelegate* obj) {}
};

template <> struct RunnableMethodTraits<ChromeFrameAutomationClient> {
  void RetainCallee(ChromeFrameAutomationClient* obj) {}
  void ReleaseCallee(ChromeFrameAutomationClient* obj) {}
};

template <> struct RunnableMethodTraits<chrome_frame_test::TimedMsgLoop> {
  void RetainCallee(chrome_frame_test::TimedMsgLoop* obj) {}
  void ReleaseCallee(chrome_frame_test::TimedMsgLoop* obj) {}
};

void MockProxyFactory::GetServerImpl(ChromeFrameAutomationProxy* pxy,
                                     void* proxy_id,
                                     AutomationLaunchResult result,
                                     LaunchDelegate* d,
                                     const ChromeFrameLaunchParams& params,
                                     void** automation_server_id) {
  *automation_server_id = proxy_id;
  Task* task = NewRunnableMethod(d,
      &ProxyFactory::LaunchDelegate::LaunchComplete, pxy, result);
  loop_->PostDelayedTask(FROM_HERE, task,
                         params.automation_server_launch_timeout/2);
}

void CFACMockTest::SetAutomationServerOk(int times) {
  EXPECT_CALL(factory_, GetAutomationServer(testing::NotNull(),
              testing::Field(&ChromeFrameLaunchParams::profile_name,
                  testing::StrEq(profile_path_.BaseName().ToWStringHack())),
              testing::NotNull()))
    .Times(times)
    .WillRepeatedly(testing::Invoke(CreateFunctor(&factory_,
        &MockProxyFactory::GetServerImpl, get_proxy(), id_,
        AUTOMATION_SUCCESS)));

  EXPECT_CALL(factory_, ReleaseAutomationServer(testing::Eq(id_))).Times(times);
}

void CFACMockTest::Set_CFD_LaunchFailed(AutomationLaunchResult result) {
  EXPECT_CALL(cfd_, OnAutomationServerLaunchFailed(testing::Eq(result),
                                                   testing::_))
      .Times(1)
      .WillOnce(QUIT_LOOP(loop_));
}

MATCHER_P(MsgType, msg_type, "IPC::Message::type()") {
  const IPC::Message& m = arg;
  return (m.type() == msg_type);
}

MATCHER_P(EqNavigationInfoUrl, url, "IPC::NavigationInfo matcher") {
  if (url.is_valid() && url != arg.url)
    return false;
  // TODO(stevet): other members
  return true;
}

// Could be implemented as MockAutomationProxy member (we have WithArgs<>!)
ACTION_P3(HandleCreateTab, tab_handle, external_tab_container, tab_wnd) {
  // arg0 - message
  // arg1 - callback
  // arg2 - key
  CallbackRunner<Tuple3<HWND, HWND, int> >* c =
      reinterpret_cast<CallbackRunner<Tuple3<HWND, HWND, int> >*>(arg1);
  c->Run(external_tab_container, tab_wnd, tab_handle);
  delete c;
  delete arg0;
}

// We mock ChromeFrameDelegate only. The rest is with real AutomationProxy
TEST(CFACWithChrome, CreateTooFast) {
  MockCFDelegate cfd;
  chrome_frame_test::TimedMsgLoop loop;
  int timeout = 0;  // Chrome cannot send Hello message so fast.
  const FilePath profile_path(
      chrome_frame_test::GetProfilePath(L"Adam.N.Epilinter"));

  scoped_refptr<ChromeFrameAutomationClient> client;
  client = new ChromeFrameAutomationClient();

  EXPECT_CALL(cfd, OnAutomationServerLaunchFailed(AUTOMATION_TIMEOUT, _))
      .Times(1)
      .WillOnce(QUIT_LOOP(loop));

  ChromeFrameLaunchParams clp = {
    timeout,
    GURL(),
    GURL(),
    profile_path,
    profile_path.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client->Initialize(&cfd, clp));
  loop.RunFor(10);
  client->Uninitialize();
}

// This test may fail if Chrome take more that 10 seconds (timeout var) to
// launch. In this case GMock shall print something like "unexpected call to
// OnAutomationServerLaunchFailed". I'm yet to find out how to specify
// that this is an unexpected call, and still to execute an action.
TEST(CFACWithChrome, CreateNotSoFast) {
  MockCFDelegate cfd;
  chrome_frame_test::TimedMsgLoop loop;
  const FilePath profile_path(
      chrome_frame_test::GetProfilePath(L"Adam.N.Epilinter"));
  int timeout = 10000;

  scoped_refptr<ChromeFrameAutomationClient> client;
  client = new ChromeFrameAutomationClient;

  EXPECT_CALL(cfd, OnAutomationServerReady())
      .Times(1)
      .WillOnce(QUIT_LOOP(loop));

  EXPECT_CALL(cfd, OnAutomationServerLaunchFailed(_, _))
      .Times(0);

  ChromeFrameLaunchParams clp = {
    timeout,
    GURL(),
    GURL(),
    profile_path,
    profile_path.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client->Initialize(&cfd, clp));

  loop.RunFor(11);
  client->Uninitialize();
  client = NULL;
}

TEST(CFACWithChrome, NavigateOk) {
  MockCFDelegate cfd;
  chrome_frame_test::TimedMsgLoop loop;
  const std::string url = "about:version";
  const FilePath profile_path(
      chrome_frame_test::GetProfilePath(L"Adam.N.Epilinter"));
  int timeout = 10000;

  scoped_refptr<ChromeFrameAutomationClient> client;
  client = new ChromeFrameAutomationClient;

  EXPECT_CALL(cfd, OnAutomationServerReady())
      .WillOnce(testing::IgnoreResult(testing::InvokeWithoutArgs(CreateFunctor(
          client.get(), &ChromeFrameAutomationClient::InitiateNavigation,
          url, std::string(), false))));

  EXPECT_CALL(cfd, GetBounds(_)).Times(testing::AnyNumber());

  EXPECT_CALL(cfd, OnNavigationStateChanged(_, _))
      .Times(testing::AnyNumber());

  {
    testing::InSequence s;

    EXPECT_CALL(cfd, OnDidNavigate(_, EqNavigationInfoUrl(GURL())))
        .Times(1);

    EXPECT_CALL(cfd, OnUpdateTargetUrl(_, _)).Times(testing::AtMost(1));

    EXPECT_CALL(cfd, OnLoad(_, _))
        .Times(1)
        .WillOnce(QUIT_LOOP(loop));
  }

  ChromeFrameLaunchParams clp = {
    timeout,
    GURL(),
    GURL(),
    profile_path,
    profile_path.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client->Initialize(&cfd, clp));
  loop.RunFor(10);
  client->Uninitialize();
  client = NULL;
}

TEST(CFACWithChrome, NavigateFailed) {
  MockCFDelegate cfd;
  chrome_frame_test::TimedMsgLoop loop;
  const FilePath profile_path(
      chrome_frame_test::GetProfilePath(L"Adam.N.Epilinter"));
  const std::string url = "http://127.0.0.3:65412/";
  const URLRequestStatus connection_failed(URLRequestStatus::FAILED,
                                           net::ERR_INVALID_URL);

  scoped_refptr<ChromeFrameAutomationClient> client;
  client = new ChromeFrameAutomationClient;
  cfd.SetRequestDelegate(client);

  EXPECT_CALL(cfd, OnAutomationServerReady())
      .WillOnce(testing::IgnoreResult(testing::InvokeWithoutArgs(CreateFunctor(
          client.get(), &ChromeFrameAutomationClient::InitiateNavigation,
          url, std::string(), false))));

  EXPECT_CALL(cfd, GetBounds(_)).Times(testing::AnyNumber());
  EXPECT_CALL(cfd, OnNavigationStateChanged(_, _)).Times(testing::AnyNumber());

  EXPECT_CALL(cfd, OnRequestStart(_, _, _))
      // Often there's another request for the error page
      .Times(testing::Between(1, 2))
      .WillRepeatedly(testing::WithArgs<1>(testing::Invoke(CreateFunctor(&cfd,
          &MockCFDelegate::Reply, connection_failed))));

  EXPECT_CALL(cfd, OnUpdateTargetUrl(_, _)).Times(testing::AnyNumber());
  EXPECT_CALL(cfd, OnLoad(_, _)).Times(testing::AtMost(1));

  EXPECT_CALL(cfd, OnNavigationFailed(_, _, GURL(url)))
      .Times(1)
      .WillOnce(QUIT_LOOP_SOON(loop, 2));

  ChromeFrameLaunchParams clp = {
    10000,
    GURL(),
    GURL(),
    profile_path,
    profile_path.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client->Initialize(&cfd, clp));

  loop.RunFor(10);
  client->Uninitialize();
  client = NULL;
}

TEST_F(CFACMockTest, MockedCreateTabOk) {
  int timeout = 500;
  CreateTab();
  SetAutomationServerOk(1);

  EXPECT_CALL(mock_proxy_, server_version()).Times(testing::AnyNumber())
      .WillRepeatedly(Return(""));

  // We need some valid HWNDs, when responding to CreateExternalTab
  HWND h1 = ::GetDesktopWindow();
  HWND h2 = ::GetDesktopWindow();
  EXPECT_CALL(mock_proxy_, SendAsAsync(testing::Property(
      &IPC::SyncMessage::type, AutomationMsg_CreateExternalTab__ID),
      testing::NotNull(), _))
          .Times(1).WillOnce(HandleCreateTab(tab_handle_, h1, h2));

  EXPECT_CALL(mock_proxy_, CreateTabProxy(testing::Eq(tab_handle_)))
      .WillOnce(Return(tab_));

  EXPECT_CALL(cfd_, OnAutomationServerReady())
      .WillOnce(QUIT_LOOP(loop_));

  EXPECT_CALL(mock_proxy_, CancelAsync(_)).Times(testing::AnyNumber());

  // Here we go!
  ChromeFrameLaunchParams clp = {
    timeout,
    GURL(),
    GURL(),
    profile_path_,
    profile_path_.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client_->Initialize(&cfd_, clp));
  loop_.RunFor(10);

  EXPECT_CALL(mock_proxy_, ReleaseTabProxy(testing::Eq(tab_handle_))).Times(1);
  client_->Uninitialize();
}

TEST_F(CFACMockTest, MockedCreateTabFailed) {
  HWND null_wnd = NULL;
  SetAutomationServerOk(1);

  EXPECT_CALL(mock_proxy_, server_version()).Times(testing::AnyNumber())
      .WillRepeatedly(Return(""));

  EXPECT_CALL(mock_proxy_, SendAsAsync(testing::Property(
      &IPC::SyncMessage::type, AutomationMsg_CreateExternalTab__ID),
      testing::NotNull(), _))
          .Times(1).WillOnce(HandleCreateTab(tab_handle_, null_wnd, null_wnd));

  EXPECT_CALL(mock_proxy_, CreateTabProxy(_)).Times(0);

  EXPECT_CALL(mock_proxy_, CancelAsync(_)).Times(testing::AnyNumber());

  Set_CFD_LaunchFailed(AUTOMATION_CREATE_TAB_FAILED);

  // Here we go!
  ChromeFrameLaunchParams clp = {
    timeout_,
    GURL(),
    GURL(),
    profile_path_,
    profile_path_.BaseName().value(),
    L"",
    false,
    false,
    false
  };
  EXPECT_TRUE(client_->Initialize(&cfd_, clp));
  loop_.RunFor(4);
  client_->Uninitialize();
}

class TestChromeFrameAutomationProxyImpl
    : public ChromeFrameAutomationProxyImpl {
 public:
  TestChromeFrameAutomationProxyImpl()
      : ChromeFrameAutomationProxyImpl(1) {  // 1 is an unneeded timeout.
  }
  MOCK_METHOD3(SendAsAsync, void(IPC::SyncMessage* msg, void* callback,
                                 void* key));
  void FakeChannelError() {
    reinterpret_cast<IPC::ChannelProxy::MessageFilter*>(message_filter_.get())->
        OnChannelError();
  }
};

TEST_F(CFACMockTest, OnChannelErrorEmpty) {
  TestChromeFrameAutomationProxyImpl proxy;

  // No tabs should do nothing yet still not fail either.
  proxy.FakeChannelError();
}

TEST_F(CFACMockTest, OnChannelError) {
  TestChromeFrameAutomationProxyImpl proxy;
  returned_proxy_ = &proxy;

  ChromeFrameLaunchParams clp = {
    1,  // Unneeded timeout, but can't be 0.
    GURL(),
    GURL(),
    profile_path_,
    profile_path_.BaseName().value(),
    L"",
    false,
    false,
    false
  };

  HWND h1 = ::GetDesktopWindow();
  HWND h2 = ::GetDesktopWindow();
  EXPECT_CALL(proxy, SendAsAsync(testing::Property(
    &IPC::SyncMessage::type, AutomationMsg_CreateExternalTab__ID),
    testing::NotNull(), _)).Times(3)
        .WillOnce(HandleCreateTab(tab_handle_, h1, h2))
        .WillOnce(HandleCreateTab(tab_handle_ * 2, h1, h2))
        .WillOnce(HandleCreateTab(tab_handle_ * 3, h1, h2));

  SetAutomationServerOk(3);

  // First, try a single tab and make sure the notification find its way to the
  // Chrome Frame Delegate.
  StrictMock<MockCFDelegate> cfd1;
  scoped_refptr<ChromeFrameAutomationClient> client1;
  client1 = new ChromeFrameAutomationClient;
  client1->set_proxy_factory(&factory_);

  EXPECT_CALL(cfd1, OnAutomationServerReady()).WillOnce(QUIT_LOOP(loop_));
  EXPECT_TRUE(client1->Initialize(&cfd1, clp));
  // Wait for OnAutomationServerReady to be called in the UI thread.
  loop_.RunFor(11);

  proxy.FakeChannelError();
  EXPECT_CALL(cfd1, OnChannelError()).WillOnce(QUIT_LOOP(loop_));
  // Wait for OnChannelError to be propagated to delegate from the UI thread.
  loop_.RunFor(11);

  // Add a second tab using a different delegate.
  StrictMock<MockCFDelegate> cfd2;
  scoped_refptr<ChromeFrameAutomationClient> client2;
  client2 = new ChromeFrameAutomationClient;
  client2->set_proxy_factory(&factory_);

  EXPECT_CALL(cfd2, OnAutomationServerReady()).WillOnce(QUIT_LOOP(loop_));
  EXPECT_TRUE(client2->Initialize(&cfd2, clp));
  // Wait for OnAutomationServerReady to be called in the UI thread.
  loop_.RunFor(11);

  EXPECT_CALL(cfd1, OnChannelError()).Times(1);
  EXPECT_CALL(cfd2, OnChannelError()).WillOnce(QUIT_LOOP(loop_));
  proxy.FakeChannelError();
  // Wait for OnChannelError to be propagated to delegate from the UI thread.
  loop_.RunFor(11);

  // And now a 3rd tab using the first delegate.
  scoped_refptr<ChromeFrameAutomationClient> client3;
  client3 = new ChromeFrameAutomationClient;
  client3->set_proxy_factory(&factory_);

  EXPECT_CALL(cfd1, OnAutomationServerReady()).WillOnce(QUIT_LOOP(loop_));
  EXPECT_TRUE(client3->Initialize(&cfd1, clp));
  // Wait for OnAutomationServerReady to be called in the UI thread.
  loop_.RunFor(11);

  EXPECT_CALL(cfd2, OnChannelError()).Times(1);
  EXPECT_CALL(cfd1, OnChannelError()).Times(2).WillOnce(Return())
      .WillOnce(QUIT_LOOP(loop_));
  proxy.FakeChannelError();
  // Wait for OnChannelError to be propagated to delegate from the UI thread.
  loop_.RunFor(11);

  // Cleanup.
  client1->Uninitialize();
  client2->Uninitialize();
  client3->Uninitialize();
  client1 = NULL;
  client2 = NULL;
  client3 = NULL;
}
