# Copyright (c) 2001-2007 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Tests for L{twisted.words.protocols.jabber.error}.
"""

from twisted.trial import unittest

from twisted.words.protocols.jabber import error
from twisted.words.xish import domish

NS_XML = 'http://www.w3.org/XML/1998/namespace'
NS_STREAMS = 'http://etherx.jabber.org/streams'
NS_XMPP_STREAMS = 'urn:ietf:params:xml:ns:xmpp-streams'
NS_XMPP_STANZAS = 'urn:ietf:params:xml:ns:xmpp-stanzas'

class BaseErrorTest(unittest.TestCase):

    def test_getElementPlain(self):
        """
        Test getting an element for a plain error.
        """
        e = error.BaseError('feature-not-implemented')
        element = e.getElement()
        self.assertIdentical(element.uri, None)
        self.assertEquals(len(element.children), 1)

    def test_getElementText(self):
        """
        Test getting an element for an error with a text.
        """
        e = error.BaseError('feature-not-implemented', 'text')
        element = e.getElement()
        self.assertEquals(len(element.children), 2)
        self.assertEquals(unicode(element.text), 'text')
        self.assertEquals(element.text.getAttribute((NS_XML, 'lang')), None)

    def test_getElementTextLang(self):
        """
        Test getting an element for an error with a text and language.
        """
        e = error.BaseError('feature-not-implemented', 'text', 'en_US')
        element = e.getElement()
        self.assertEquals(len(element.children), 2)
        self.assertEquals(unicode(element.text), 'text')
        self.assertEquals(element.text[(NS_XML, 'lang')], 'en_US')

    def test_getElementAppCondition(self):
        """
        Test getting an element for an error with an app specific condition.
        """
        ac = domish.Element(('testns', 'myerror'))
        e = error.BaseError('feature-not-implemented', appCondition=ac)
        element = e.getElement()
        self.assertEquals(len(element.children), 2)
        self.assertEquals(element.myerror, ac)

class StreamErrorTest(unittest.TestCase):

    def test_getElementPlain(self):
        """
        Test namespace of the element representation of an error.
        """
        e = error.StreamError('feature-not-implemented')
        element = e.getElement()
        self.assertEquals(element.uri, NS_STREAMS)

    def test_getElementConditionNamespace(self):
        """
        Test that the error condition element has the correct namespace.
        """
        e = error.StreamError('feature-not-implemented')
        element = e.getElement()
        self.assertEquals(NS_XMPP_STREAMS, getattr(element, 'feature-not-implemented').uri)

    def test_getElementTextNamespace(self):
        """
        Test that the error text element has the correct namespace.
        """
        e = error.StreamError('feature-not-implemented', 'text')
        element = e.getElement()
        self.assertEquals(NS_XMPP_STREAMS, element.text.uri)

class StanzaErrorTest(unittest.TestCase):

    def test_getElementPlain(self):
        """
        Test getting an element for a plain stanza error.
        """
        e = error.StanzaError('feature-not-implemented')
        element = e.getElement()
        self.assertEquals(element.uri, None)
        self.assertEquals(element['type'], 'cancel')
        self.assertEquals(element['code'], '501')

    def test_getElementType(self):
        """
        Test getting an element for a stanza error with a given type.
        """
        e = error.StanzaError('feature-not-implemented', 'auth')
        element = e.getElement()
        self.assertEquals(element.uri, None)
        self.assertEquals(element['type'], 'auth')
        self.assertEquals(element['code'], '501')

    def test_getElementConditionNamespace(self):
        """
        Test that the error condition element has the correct namespace.
        """
        e = error.StanzaError('feature-not-implemented')
        element = e.getElement()
        self.assertEquals(NS_XMPP_STANZAS, getattr(element, 'feature-not-implemented').uri)

    def test_getElementTextNamespace(self):
        """
        Test that the error text element has the correct namespace.
        """
        e = error.StanzaError('feature-not-implemented', text='text')
        element = e.getElement()
        self.assertEquals(NS_XMPP_STANZAS, element.text.uri)

    def test_toResponse(self):
        """
        Test an error response is generated from a stanza.

        The addressing on the (new) response stanza should be reversed, an
        error child (with proper properties) added and the type set to
        C{'error'}.
        """
        stanza = domish.Element(('jabber:client', 'message'))
        stanza['type'] = 'chat'
        stanza['to'] = 'user1@example.com'
        stanza['from'] = 'user2@example.com/resource'
        e = error.StanzaError('service-unavailable')
        response = e.toResponse(stanza)
        self.assertNotIdentical(response, stanza)
        self.assertEqual(response['from'], 'user1@example.com')
        self.assertEqual(response['to'], 'user2@example.com/resource')
        self.assertEqual(response['type'], 'error')
        self.assertEqual(response.error.children[0].name,
                         'service-unavailable')
        self.assertEqual(response.error['type'], 'cancel')
        self.assertNotEqual(stanza.children, response.children)

class ParseErrorTest(unittest.TestCase):

    def setUp(self):
        self.error = domish.Element((None, 'error'))

    def test_empty(self):
        """
        Test parsing of the empty error element.
        """
        result = error._parseError(self.error, 'errorns')
        self.assertEqual({'condition': None,
                          'text': None,
                          'textLang': None,
                          'appCondition': None}, result)

    def test_condition(self):
        """
        Test parsing of an error element with a condition.
        """
        self.error.addElement(('errorns', 'bad-request'))
        result = error._parseError(self.error, 'errorns')
        self.assertEqual('bad-request', result['condition'])

    def test_text(self):
        """
        Test parsing of an error element with a text.
        """
        text = self.error.addElement(('errorns', 'text'))
        text.addContent('test')
        result = error._parseError(self.error, 'errorns')
        self.assertEqual('test', result['text'])
        self.assertEqual(None, result['textLang'])

    def test_textLang(self):
        """
        Test parsing of an error element with a text with a defined language.
        """
        text = self.error.addElement(('errorns', 'text'))
        text[NS_XML, 'lang'] = 'en_US'
        text.addContent('test')
        result = error._parseError(self.error, 'errorns')
        self.assertEqual('en_US', result['textLang'])

    def test_textLangInherited(self):
        """
        Test parsing of an error element with a text with inherited language.
        """
        text = self.error.addElement(('errorns', 'text'))
        self.error[NS_XML, 'lang'] = 'en_US'
        text.addContent('test')
        result = error._parseError(self.error, 'errorns')
        self.assertEqual('en_US', result['textLang'])
    test_textLangInherited.todo = "xml:lang inheritance not implemented"

    def test_appCondition(self):
        """
        Test parsing of an error element with an app specific condition.
        """
        condition = self.error.addElement(('testns', 'condition'))
        result = error._parseError(self.error, 'errorns')
        self.assertEqual(condition, result['appCondition'])

    def test_appConditionMultiple(self):
        """
        Test parsing of an error element with multiple app specific conditions.
        """
        condition = self.error.addElement(('testns', 'condition'))
        condition2 = self.error.addElement(('testns', 'condition2'))
        result = error._parseError(self.error, 'errorns')
        self.assertEqual(condition2, result['appCondition'])

class ExceptionFromStanzaTest(unittest.TestCase):

    def test_basic(self):
        """
        Test basic operations of exceptionFromStanza.

        Given a realistic stanza, check if a sane exception is returned.

        Using this stanza::

          <iq type='error'
              from='pubsub.shakespeare.lit'
              to='francisco@denmark.lit/barracks'
              id='subscriptions1'>
            <pubsub xmlns='http://jabber.org/protocol/pubsub'>
              <subscriptions/>
            </pubsub>
            <error type='cancel'>
              <feature-not-implemented
                xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>
              <unsupported xmlns='http://jabber.org/protocol/pubsub#errors'
                           feature='retrieve-subscriptions'/>
            </error>
          </iq>
        """

        stanza = domish.Element((None, 'stanza'))
        p = stanza.addElement(('http://jabber.org/protocol/pubsub', 'pubsub'))
        p.addElement('subscriptions')
        e = stanza.addElement('error')
        e['type'] = 'cancel'
        e.addElement((NS_XMPP_STANZAS, 'feature-not-implemented'))
        uc = e.addElement(('http://jabber.org/protocol/pubsub#errors',
                           'unsupported'))
        uc['feature'] = 'retrieve-subscriptions'

        result = error.exceptionFromStanza(stanza)
        self.assert_(isinstance(result, error.StanzaError))
        self.assertEquals('feature-not-implemented', result.condition)
        self.assertEquals('cancel', result.type)
        self.assertEquals(uc, result.appCondition)
        self.assertEquals([p], result.children)

    def test_legacy(self):
        """
        Test legacy operations of exceptionFromStanza.

        Given a realistic stanza with only legacy (pre-XMPP) error information,
        check if a sane exception is returned.

        Using this stanza::

          <message type='error'
                   to='piers@pipetree.com/Home'
                   from='qmacro@jaber.org'>
            <body>Are you there?</body>
            <error code='502'>Unable to resolve hostname.</error>
          </message>
        """
        stanza = domish.Element((None, 'stanza'))
        p = stanza.addElement('body', content='Are you there?')
        e = stanza.addElement('error', content='Unable to resolve hostname.')
        e['code'] = '502'

        result = error.exceptionFromStanza(stanza)
        self.assert_(isinstance(result, error.StanzaError))
        self.assertEquals('service-unavailable', result.condition)
        self.assertEquals('wait', result.type)
        self.assertEquals('Unable to resolve hostname.', result.text)
        self.assertEquals([p], result.children)

class ExceptionFromStreamErrorTest(unittest.TestCase):

    def test_basic(self):
        """
        Test basic operations of exceptionFromStreamError.

        Given a realistic stream error, check if a sane exception is returned.

        Using this error::

          <stream:error xmlns:stream='http://etherx.jabber.org/streams'>
            <xml-not-well-formed xmlns='urn:ietf:params:xml:ns:xmpp-streams'/>
          </stream:error>
        """

        e = domish.Element(('http://etherx.jabber.org/streams', 'error'))
        e.addElement((NS_XMPP_STREAMS, 'xml-not-well-formed'))

        result = error.exceptionFromStreamError(e)
        self.assert_(isinstance(result, error.StreamError))
        self.assertEquals('xml-not-well-formed', result.condition)
