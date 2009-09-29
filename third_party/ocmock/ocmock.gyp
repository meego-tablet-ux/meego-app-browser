# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets' : [
        {
          'target_name' : 'ocmock',
          'type': '<(library)',
          'include_dirs':[ '.',],
          'direct_dependent_settings': {
            'include_dirs': [ '.', ],
          },
          'sources': [
            'OCMock/NSInvocation+OCMAdditions.h',
            'OCMock/OCMObserverRecorder.m',
            'OCMock/NSInvocation+OCMAdditions.m',
            'OCMock/NSMethodSignature+OCMAdditions.h',	
            'OCMock/NSMethodSignature+OCMAdditions.m',	
            'OCMock/NSNotificationCenter+OCMAdditions.h',	
            'OCMock/NSNotificationCenter+OCMAdditions.m',	
            'OCMock/OCClassMockObject.h',			
            'OCMock/OCClassMockObject.m',			
            'OCMock/OCMArg.h',				
            'OCMock/OCMArg.m',				
            'OCMock/OCMBoxedReturnValueProvider.h',		
            'OCMock/OCMBoxedReturnValueProvider.m',		
            'OCMock/OCMConstraint.h',				
            'OCMock/OCMConstraint.m',				
            'OCMock/OCMExceptionReturnValueProvider.h',	
            'OCMock/OCMExceptionReturnValueProvider.m',	
            'OCMock/OCMIndirectReturnValueProvider.h',	
            'OCMock/OCMIndirectReturnValueProvider.m',	
            'OCMock/OCMNotificationPoster.h',			
            'OCMock/OCMNotificationPoster.m',			
            'OCMock/OCMObserverRecorder.h',			
            'OCMock/OCMPassByRefSetter.h',
            'OCMock/OCMPassByRefSetter.m',
            'OCMock/OCMReturnValueProvider.h',
            'OCMock/OCMReturnValueProvider.m',
            'OCMock/OCMock.h',
            'OCMock/OCMockObject.h',
            'OCMock/OCMockObject.m',
            'OCMock/OCPartialMockObject.h',
            'OCMock/OCPartialMockObject.m',
            'OCMock/OCPartialMockRecorder.h',
            'OCMock/OCPartialMockRecorder.m',
            'OCMock/OCProtocolMockObject.h',
            'OCMock/OCProtocolMockObject.m',
            'OCMock/OCMockRecorder.h',
            'OCMock/OCMockRecorder.m',
            'OCMock/OCObserverMockObject.h',
            'OCMock/OCObserverMockObject.m',
          ],
        },
      ],
    }],
  ],
}
